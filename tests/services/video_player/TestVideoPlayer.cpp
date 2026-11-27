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
    void findsThePreviewNearestAPosition_data();
    void findsThePreviewNearestAPosition();
    void hasNoPreviewsWhenTheServerCannotMakeThem();
    void previewsGoWithTheirVideo();
    void ignoresTheStoryboardOfAVideoLeftBehind();

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
    // 25 frames a second apart, 160×90, ten to a row; none for video 6
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/media/5/storyboard"),
                     jsonResponse(200,
                                  {{QStringLiteral("interval_ms"), 1000},
                                   {QStringLiteral("frame_count"), 25},
                                   {QStringLiteral("frame_width"), 160},
                                   {QStringLiteral("frame_height"), 90},
                                   {QStringLiteral("columns"), 10},
                                   {QStringLiteral("sprite"), QStringLiteral("/api/v1/media/5/storyboard/sprite")}}));
    m_server->answer(
        "GET", serverUrl + QStringLiteral("/api/v1/media/6/storyboard"), fakes::problem(503, "storyboard_unavailable"));

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

    // Held: 5's URL and storyboard, then 6's. The video on screen gets its URL first.
    m_server->releaseAnswer(2);
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

void TestVideoPlayer::findsThePreviewNearestAPosition_data()
{
    QTest::addColumn<qint64>("positionMs");
    QTest::addColumn<QPoint>("offset");

    QTest::newRow("the start") << qint64(0) << QPoint(0, 0);
    QTest::newRow("nearer the first frame") << qint64(1400) << QPoint(160, 0);
    QTest::newRow("nearer the second") << qint64(1600) << QPoint(320, 0);
    QTest::newRow("the second row") << qint64(10000) << QPoint(0, 90);
    QTest::newRow("the last frame") << qint64(24400) << QPoint(640, 180);
    QTest::newRow("past the last frame") << qint64(99000) << QPoint(640, 180);
    QTest::newRow("before the start") << qint64(-500) << QPoint(0, 0);
}

void TestVideoPlayer::findsThePreviewNearestAPosition()
{
    QFETCH(qint64, positionMs);
    QFETCH(QPoint, offset);
    logIn();
    m_player->openLibraryVideo(5);
    QTRY_VERIFY(m_player->hasPreviews());

    QCOMPARE(m_player->previewSprite(), QUrl(serverUrl + QStringLiteral("/api/v1/media/5/storyboard/sprite")));
    QCOMPARE(m_player->previewFrameSize(), QSize(160, 90));
    QCOMPARE(m_player->previewFrameOffset(positionMs), offset);
}

// The scrub bar works without them: no message.
void TestVideoPlayer::hasNoPreviewsWhenTheServerCannotMakeThem()
{
    logIn();

    m_player->openLibraryVideo(6);

    QTRY_COMPARE(m_player->source(), signedUrl(6, 1));
    QVERIFY(!m_player->hasPreviews());
    QCOMPARE(m_player->errorMessage(), QString());
    QCOMPARE(m_player->previewFrameOffset(5000), QPoint(0, 0));
}

void TestVideoPlayer::previewsGoWithTheirVideo()
{
    logIn();
    m_player->openLibraryVideo(5);
    QTRY_VERIFY(m_player->hasPreviews());

    m_player->openLocalVideo(QStringLiteral("/home/apo/Videos/waves.mp4"));
    QVERIFY(!m_player->hasPreviews());

    m_player->openLibraryVideo(5);
    QTRY_VERIFY(m_player->hasPreviews());
    m_player->close();
    QVERIFY(!m_player->hasPreviews());
    QCOMPARE(m_player->previewSprite(), QUrl());
}

void TestVideoPlayer::ignoresTheStoryboardOfAVideoLeftBehind()
{
    logIn();
    m_server->holdAnswers = true;
    m_player->openLibraryVideo(5);
    m_player->openLibraryVideo(6);
    QTRY_COMPARE(m_server->requestsTo("GET", serverUrl + QStringLiteral("/api/v1/media/6/storyboard")).size(), 1);

    m_server->releaseAnswers();
    QTRY_COMPARE(m_player->source(), signedUrl(6, 1));

    QVERIFY(!m_player->hasPreviews());  // video 5's frames, not this one's
}

}  // namespace photobook::video_player

QTEST_GUILESS_MAIN(photobook::video_player::TestVideoPlayer)
#include "TestVideoPlayer.moc"
