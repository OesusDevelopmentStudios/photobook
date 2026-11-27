/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "VideoPlayer.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/ImageJson.hpp"

namespace photobook::video_player
{

namespace
{

using connection::Connection;
using fakes::FakeServer;
using fakes::jsonResponse;
using server_client::HttpRequest;

const QString serverUrl = QStringLiteral("https://photos.example.org");

QString streamRequestUrl(int imageId)
{
    return serverUrl + QStringLiteral("/api/v1/media/%1/stream").arg(imageId);
}

// What the player plays a library video from: the file's URL with the signature of the nth answer
QUrl signedUrl(int imageId, int answer)
{
    return QUrl(serverUrl +
                QStringLiteral("/api/v1/media/%1/file?t=3&exp=1795826657&sig=s%2").arg(imageId).arg(answer));
}

}  // namespace

class TestVideoPlayer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void streamsALibraryVideoFromASignedUrl();
    void asksForANewUrlOnceAfterARefusedOne();
    void ignoresTheUrlOfAVideoLeftBehind();
    void saysWhyTheServerGaveNoUrl();
    void playsALocalFileAsItIs();
    void remembersVolumeAndSpeed();
    void eachVideoStartsAsStartMutedSays();
    void closingIgnoresALateUrl();

private:
    void logIn();
    void answerStreamRequests(int imageId);

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<VideoPlayer> m_player;
    QMap<int, int> m_answers;  // stream URLs given so far, per video
};

void TestVideoPlayer::init()
{
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("photobookrc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    m_answers.clear();
    const QJsonObject user{{QStringLiteral("id"), 2},
                           {QStringLiteral("username"), QStringLiteral("restrictedUser")},
                           {QStringLiteral("role"), QStringLiteral("user")},
                           {QStringLiteral("restricted_tags"), QJsonArray()}};
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/auth/me"), jsonResponse(200, user));
    answerStreamRequests(5);
    answerStreamRequests(6);

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_player = std::make_unique<VideoPlayer>(m_connection.get(), m_settings.get());
}

void TestVideoPlayer::logIn()
{
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
}

// Each answer a URL with a signature of its own, as a new one from the server would have
void TestVideoPlayer::answerStreamRequests(int imageId)
{
    m_server->answerWith("POST", streamRequestUrl(imageId), [this, imageId](const HttpRequest&) {
        const int answer = ++m_answers[imageId];
        return jsonResponse(
            200,
            {{QStringLiteral("url"), signedUrl(imageId, answer).toString(QUrl::RemoveScheme | QUrl::RemoveAuthority)},
             {QStringLiteral("expires_at"), QStringLiteral("2026-11-28T00:44:17Z")}});
    });
}

void TestVideoPlayer::streamsALibraryVideoFromASignedUrl()
{
    logIn();
    m_server->holdAnswers = true;

    m_player->openLibraryVideo(5);

    QVERIFY(m_player->isWaitingForSource());
    QCOMPARE(m_player->source(), QUrl());
    QTRY_COMPARE(m_server->requestsTo("POST", streamRequestUrl(5)).size(), 1);
    m_server->releaseAnswers();
    QTRY_COMPARE(m_player->source(), signedUrl(5, 1));
    QVERIFY(!m_player->isWaitingForSource());
    QCOMPARE(m_player->startPositionMs(), 0);
    QCOMPARE(m_player->errorMessage(), QString());
}

// A URL expires, and ends when the login does: the player gets one new URL and goes on where it stopped. A second
// refusal is a real one.
void TestVideoPlayer::asksForANewUrlOnceAfterARefusedOne()
{
    logIn();
    m_player->openLibraryVideo(5);
    QTRY_COMPARE(m_player->source(), signedUrl(5, 1));

    m_player->reportPlaybackError(QStringLiteral("Could not open file"), 42000);

    QTRY_COMPARE(m_player->source(), signedUrl(5, 2));
    QCOMPARE(m_player->startPositionMs(), 42000);
    QCOMPARE(m_player->errorMessage(), QString());

    m_player->reportPlaybackError(QStringLiteral("Could not open file"), 42500);

    QCOMPARE(m_player->source(), QUrl());
    QCOMPARE(m_player->errorMessage(), QStringLiteral("Cannot play the video: Could not open file"));
    QCOMPARE(m_server->requestsTo("POST", streamRequestUrl(5)).size(), 2);
}

void TestVideoPlayer::ignoresTheUrlOfAVideoLeftBehind()
{
    logIn();
    m_server->holdAnswers = true;
    m_player->openLibraryVideo(5);
    m_player->openLibraryVideo(6);
    QTRY_COMPARE(m_server->requestsTo("POST", streamRequestUrl(6)).size(), 1);

    m_server->releaseAnswer(1);  // the video on screen first
    QTRY_COMPARE(m_player->source(), signedUrl(6, 1));
    m_server->releaseAnswers();
    QTest::qWait(50);

    QCOMPARE(m_player->source(), signedUrl(6, 1));
}

void TestVideoPlayer::saysWhyTheServerGaveNoUrl()
{
    m_server->answer("POST",
                     streamRequestUrl(7),
                     fakes::response(404, R"({"type":"t","title":"Not a video","status":404,"code":"not_found",
                                              "detail":"This is an image; only videos have this."})"));
    logIn();

    m_player->openLibraryVideo(7);

    QTRY_COMPARE(m_player->errorMessage(),
                 QStringLiteral("Cannot play the video: This is an image; only videos have this."));
    QCOMPARE(m_player->source(), QUrl());
    QVERIFY(!m_player->isWaitingForSource());
}

void TestVideoPlayer::playsALocalFileAsItIs()
{
    m_player->openLocalVideo(QStringLiteral("/home/apo/Videos/waves.mp4"));

    QCOMPARE(m_player->source(), QUrl::fromLocalFile(QStringLiteral("/home/apo/Videos/waves.mp4")));
    QVERIFY(!m_player->isWaitingForSource());

    m_player->reportPlaybackError(QStringLiteral("Unsupported codec"), 0);

    QCOMPARE(m_player->errorMessage(), QStringLiteral("Cannot play the video: Unsupported codec"));
    QVERIFY(m_server->requests.isEmpty());  // a file on this computer needs nothing from the server
}

void TestVideoPlayer::remembersVolumeAndSpeed()
{
    QCOMPARE(m_player->volume(), 1.0);
    QCOMPARE(m_player->speed(), 1.0);
    QCOMPARE(m_player->speeds(), QList<double>({0.25, 0.5, 1.0, 1.25, 1.5, 2.0}));

    m_player->setVolume(0.4);
    m_player->setSpeed(1.5);
    m_player->setSpeed(3.0);   // not one of the speeds
    m_player->setVolume(1.7);  // above full

    QCOMPARE(m_player->speed(), 1.5);
    QCOMPARE(m_player->volume(), 1.0);
    m_player->setVolume(0.4);
    const VideoPlayer afterRestart(m_connection.get(), m_settings.get());
    QCOMPARE(afterRestart.volume(), 0.4);
    QCOMPARE(afterRestart.speed(), 1.5);
}

void TestVideoPlayer::eachVideoStartsAsStartMutedSays()
{
    m_player->setStartMuted(true);
    m_player->openLocalVideo(QStringLiteral("/home/apo/Videos/waves.mp4"));
    QVERIFY(m_player->isMuted());

    m_player->setMuted(false);  // for this video only
    m_player->openLocalVideo(QStringLiteral("/home/apo/Videos/beach.mp4"));

    QVERIFY(m_player->isMuted());
    QCOMPARE(m_settings->startVideosMuted(), true);
    m_player->setStartMuted(false);
    m_player->openLocalVideo(QStringLiteral("/home/apo/Videos/waves.mp4"));
    QVERIFY(!m_player->isMuted());
}

// E.g. the viewer moved on to a photo while the server was still answering
void TestVideoPlayer::closingIgnoresALateUrl()
{
    logIn();
    m_server->holdAnswers = true;
    m_player->openLibraryVideo(5);
    QTRY_COMPARE(m_server->requestsTo("POST", streamRequestUrl(5)).size(), 1);

    m_player->close();
    m_server->releaseAnswers();
    QTest::qWait(50);

    QCOMPARE(m_player->source(), QUrl());
    QVERIFY(!m_player->isWaitingForSource());
}

}  // namespace photobook::video_player

QTEST_GUILESS_MAIN(photobook::video_player::TestVideoPlayer)
#include "TestVideoPlayer.moc"
