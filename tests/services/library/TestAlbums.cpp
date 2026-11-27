/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrlQuery>

#include "Albums.hpp"
#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/ImageJson.hpp"

namespace photobook::library
{

namespace
{

using connection::Connection;
using fakes::FakeServer;
using fakes::jsonResponse;
using server_client::HttpRequest;

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString albumsUrl = serverUrl + QStringLiteral("/api/v1/albums");

QJsonObject albumJson(const QString& tag, int imageCount, int coverId, int coverRevision, const QString& lastChanged)
{
    return {
        {QStringLiteral("tag"), tag},
        {QStringLiteral("media_count"), imageCount},
        {QStringLiteral("cover"),
         QJsonObject{{QStringLiteral("id"), coverId}, {QStringLiteral("revision"), coverRevision}}},
        {QStringLiteral("last_changed_at"), lastChanged},
    };
}

bool includesHidden(const HttpRequest& request)
{
    return QUrlQuery(request.url).queryItemValue(QStringLiteral("include_hidden")) == QLatin1String("true");
}

// Three albums; with the hidden ones, a fourth whose images a hidden filter matches. The server sends them most
// recently changed first.
server_client::HttpResponse albumsAnswer(const HttpRequest& request)
{
    QJsonArray items;
    if (includesHidden(request))
    {
        items.append(albumJson(QStringLiteral("album:private_trip"), 5, 30, 0, QStringLiteral("2026-11-23T08:00:00Z")));
    }

    items.append(albumJson(QStringLiteral("album:garden"), 3, 7, 0, QStringLiteral("2026-11-22T10:00:00Z")));
    items.append(albumJson(QStringLiteral("album:vacation_2015"), 84, 1234, 2, QStringLiteral("2026-11-20T18:03:11Z")));
    items.append(albumJson(QStringLiteral("album:birthday_party"), 12, 9, 0, QStringLiteral("2026-11-01T12:00:00Z")));
    return jsonResponse(200, {{QStringLiteral("items"), items}});
}

}  // namespace

class TestAlbums : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void loadsOnceLoggedIn();
    void givesTheGridWhatItShows();
    void sortsByNameAndSize();
    void filtersByName();
    void listsAlbumsWithHiddenImagesWhenAsked();
    void remembersTheSortAndTheHiddenChoice();
    void ignoresTheAnswerToAnOlderLoad();
    void keepsTheGridWhenNothingChanged();
    void showsWhyLoadingFailed();
    void emptiesWhenLoggedOut();

private:
    void logIn();
    QStringList shownNames() const;

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<Albums> m_albums;
};

void TestAlbums::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
}

// Logged in from the last run: the kept token is checked at start.
void TestAlbums::init()
{
    m_albums.reset();
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("photobookrc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/auth/me"),
                     jsonResponse(200,
                                  {{QStringLiteral("id"), 2},
                                   {QStringLiteral("username"), QStringLiteral("restrictedUser")},
                                   {QStringLiteral("role"), QStringLiteral("user")},
                                   {QStringLiteral("restricted_tags"), QJsonArray()}}));
    m_server->answerWith("GET", albumsUrl, albumsAnswer);

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_albums = std::make_unique<Albums>(m_connection.get(), m_settings.get());
}

void TestAlbums::logIn()
{
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
}

QStringList TestAlbums::shownNames() const
{
    QStringList names;
    for (int row = 0; row < m_albums->rowCount(); ++row)
    {
        names.append(m_albums->data(m_albums->index(row), Albums::NameRole).toString());
    }

    return names;
}

void TestAlbums::loadsOnceLoggedIn()
{
    QCOMPARE(m_albums->count(), 0);  // not logged in yet

    logIn();

    QTRY_COMPARE(m_albums->count(), 3);
    QVERIFY(!m_albums->isLoading());
    QVERIFY(!includesHidden(m_server->requestsTo("GET", albumsUrl).last()));
    QCOMPARE(m_albums->sort(), Albums::Sort::RecentlyChanged);
    QCOMPARE(
        shownNames(),
        QStringList({QStringLiteral("garden"), QStringLiteral("vacation 2015"), QStringLiteral("birthday party")}));
}

void TestAlbums::givesTheGridWhatItShows()
{
    logIn();
    QTRY_COMPARE(m_albums->count(), 3);

    const QModelIndex vacation = m_albums->index(1);

    QCOMPARE(m_albums->data(vacation, Albums::TagRole).toString(), QStringLiteral("album:vacation_2015"));
    QCOMPARE(m_albums->data(vacation, Albums::NameRole).toString(), QStringLiteral("vacation 2015"));
    QCOMPARE(m_albums->data(vacation, Albums::ImageCountRole).toInt(), 84);
    QCOMPARE(m_albums->data(vacation, Albums::LastChangedAtRole).toDateTime(),
             QDateTime(QDate(2026, 11, 20), QTime(18, 3, 11), QTimeZone::UTC));
    // The cover's thumbnail, at its revision, so a turned cover is not shown from an old cache
    const QUrl cover = m_albums->data(vacation, Albums::CoverUrlRole).toUrl();
    QCOMPARE(cover.path(), QStringLiteral("/api/v1/media/1234/thumbnail"));
    QCOMPARE(QUrlQuery(cover).queryItemValue(QStringLiteral("size")), QStringLiteral("256"));
    QCOMPARE(QUrlQuery(cover).queryItemValue(QStringLiteral("rev")), QStringLiteral("2"));
    const QUrl largeCover = m_albums->data(vacation, Albums::LargeCoverUrlRole).toUrl();
    QCOMPARE(QUrlQuery(largeCover).queryItemValue(QStringLiteral("size")), QStringLiteral("1024"));
    QCOMPARE(m_albums->albumAt(1)->coverImageId, 1234);
    QCOMPARE(m_albums->albumAt(3), std::nullopt);
    QCOMPARE(m_albums->roleNames().value(Albums::CoverUrlRole), QByteArray("coverUrl"));
}

void TestAlbums::sortsByNameAndSize()
{
    logIn();
    QTRY_COMPARE(m_albums->count(), 3);

    m_albums->setSort(Albums::Sort::Name);
    QCOMPARE(
        shownNames(),
        QStringList({QStringLiteral("birthday party"), QStringLiteral("garden"), QStringLiteral("vacation 2015")}));

    m_albums->setSort(Albums::Sort::Size);
    QCOMPARE(
        shownNames(),
        QStringList({QStringLiteral("vacation 2015"), QStringLiteral("birthday party"), QStringLiteral("garden")}));
    QCOMPARE(m_server->requestsTo("GET", albumsUrl).size(), 1);  // sorted here, not asked again
}

void TestAlbums::filtersByName()
{
    logIn();
    QTRY_COMPARE(m_albums->count(), 3);

    m_albums->setFilterText(QStringLiteral("Party"));
    QCOMPARE(shownNames(), QStringList({QStringLiteral("birthday party")}));

    m_albums->setFilterText(QStringLiteral("birthday_p"));  // as the tag is written
    QCOMPARE(shownNames(), QStringList({QStringLiteral("birthday party")}));

    m_albums->setFilterText(QStringLiteral("a"));
    QCOMPARE(
        shownNames(),
        QStringList({QStringLiteral("garden"), QStringLiteral("vacation 2015"), QStringLiteral("birthday party")}));

    m_albums->setFilterText(QStringLiteral("nothing like it"));
    QCOMPARE(m_albums->count(), 0);
    QCOMPARE(m_albums->totalCount(), 3);  // "No album matches", not "No albums yet"
}

void TestAlbums::listsAlbumsWithHiddenImagesWhenAsked()
{
    logIn();
    QTRY_COMPARE(m_albums->count(), 3);

    m_albums->setIncludeHidden(true);

    QTRY_COMPARE(m_albums->count(), 4);
    QVERIFY(includesHidden(m_server->requestsTo("GET", albumsUrl).last()));
    QCOMPARE(shownNames().first(), QStringLiteral("private trip"));
}

// Chosen in one run, there in the next
void TestAlbums::remembersTheSortAndTheHiddenChoice()
{
    m_albums->setSort(Albums::Sort::Size);
    m_albums->setIncludeHidden(true);
    m_albums->setFilterText(QStringLiteral("garden"));

    const Albums nextRun(m_connection.get(), m_settings.get());
    logIn();

    QCOMPARE(nextRun.sort(), Albums::Sort::Size);
    QVERIFY(nextRun.includesHidden());
    QVERIFY(nextRun.filterText().isEmpty());
    QTRY_COMPARE(nextRun.count(), 4);
    QCOMPARE(nextRun.albumAt(0)->tag, QStringLiteral("album:vacation_2015"));  // the largest
}

void TestAlbums::ignoresTheAnswerToAnOlderLoad()
{
    logIn();
    QTRY_COMPARE(m_albums->count(), 3);
    m_server->holdAnswers = true;
    m_albums->reload();
    m_albums->setIncludeHidden(true);
    QTRY_COMPARE(m_server->requestsTo("GET", albumsUrl).size(), 3);

    m_server->releaseAnswer(1);  // the newer answer first…
    m_server->releaseAnswers();  // …and the older one after it

    QTRY_COMPARE(m_albums->count(), 4);
    QTest::qWait(50);
    QCOMPARE(m_albums->count(), 4);
    QVERIFY(!m_albums->isLoading());
}

// The tab reloads whenever it is shown, e.g. back from the viewer: the grid keeps its place.
void TestAlbums::keepsTheGridWhenNothingChanged()
{
    logIn();
    QTRY_COMPARE(m_albums->count(), 3);
    QSignalSpy reset(m_albums.get(), &Albums::modelReset);

    m_albums->reload();
    QTRY_VERIFY(!m_albums->isLoading());

    QCOMPARE(reset.size(), 0);
    QCOMPARE(m_server->requestsTo("GET", albumsUrl).size(), 2);
}

void TestAlbums::showsWhyLoadingFailed()
{
    logIn();
    QTRY_COMPARE(m_albums->count(), 3);
    m_server->answerWith("GET", albumsUrl, [](const HttpRequest&) {
        return jsonResponse(500,
                            {{QStringLiteral("type"), QStringLiteral("t")},
                             {QStringLiteral("title"), QStringLiteral("Internal error")},
                             {QStringLiteral("status"), 500},
                             {QStringLiteral("code"), QStringLiteral("internal")},
                             {QStringLiteral("detail"), QStringLiteral("The database is locked.")}});
    });

    m_albums->reload();

    QTRY_COMPARE(m_albums->errorMessage(), QStringLiteral("Internal error. The database is locked."));
    QCOMPARE(m_albums->count(), 3);  // the albums shown before stay
    QVERIFY(!m_albums->isLoading());

    m_server->answerWith("GET", albumsUrl, albumsAnswer);
    m_albums->reload();

    QVERIFY(m_albums->errorMessage().isEmpty());
}

void TestAlbums::emptiesWhenLoggedOut()
{
    logIn();
    QTRY_COMPARE(m_albums->count(), 3);

    m_connection->logOut();

    QCOMPARE(m_albums->count(), 0);
    QCOMPARE(m_albums->totalCount(), 0);
}

}  // namespace photobook::library

QTEST_GUILESS_MAIN(photobook::library::TestAlbums)

#include "TestAlbums.moc"
