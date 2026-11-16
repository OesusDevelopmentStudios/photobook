/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrlQuery>

#include "Connection.hpp"
#include "Library.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/ImageJson.hpp"

namespace pictorio::library
{

namespace
{

using connection::Connection;
using fakes::FakeServer;
using fakes::imagePageJson;
using fakes::jsonResponse;
using server_client::HttpRequest;

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString imagesUrl = serverUrl + QStringLiteral("/api/v1/images");
const QString countUrl = serverUrl + QStringLiteral("/api/v1/images/count");

QString parameter(const HttpRequest& request, const QString& name)
{
    return QUrlQuery(request.url).queryItemValue(name, QUrl::FullyDecoded);
}

// The cursor of the second page of the request's search, e.g. "page2:taken_at:summer".
QString cursorFor(const HttpRequest& request)
{
    return QStringLiteral("page2:%1:%2")
        .arg(parameter(request, QStringLiteral("sort")), parameter(request, QStringLiteral("q")));
}

}  // namespace

class TestLibrary : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void loadsTheFirstPageOnceLoggedIn();
    void loadsTheNextPageWhenTheGridAsks();
    void searchesAndStartsOver();
    void keepsARandomOrderWhilePaging();
    void ignoresTheAnswerToAnOlderSearch();
    void neverMixesANewOrderWithTheOldOrdersPages();
    void showsWhyASearchFailed();
    void emptiesWhenLoggedOut();
    void givesTheGridWhatItShows();
    void knowsTheTotalOfTheSearch();
    void ignoresTheTotalOfAnOlderSearch();
    void hasNoTotalIfTheServerCannotCount();
    void replacesAnImageAfterItChanged();

private:
    void logIn();
    QList<HttpRequest> searches() const;

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<Library> m_library;
};

// Logged in from the last run: the kept token is checked at start. All images: two pages (1, 2 and 3).
void TestLibrary::init()
{
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("pictoriorc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    const QJsonObject kid{{QStringLiteral("id"), 2},
                          {QStringLiteral("username"), QStringLiteral("kid")},
                          {QStringLiteral("role"), QStringLiteral("user")},
                          {QStringLiteral("restricted_tags"), QJsonArray()}};
    m_server->answer("GET", serverUrl + QStringLiteral("/api/v1/auth/me"), jsonResponse(200, kid));
    m_server->answerWith("GET", imagesUrl, [](const HttpRequest& request) {
        if (parameter(request, QStringLiteral("q")) == QLatin1String("category:photo"))
        {
            return imagePageJson({7}, QString());
        }

        // Like a real cursor, this one belongs to its search: the sort and the text.
        const QString nextPage = cursorFor(request);
        if (parameter(request, QStringLiteral("cursor")) == nextPage)
        {
            return imagePageJson({3}, QString());
        }

        return imagePageJson({1, 2}, nextPage);
    });
    m_server->answerWith("GET", countUrl, [](const HttpRequest& request) {
        const bool photos = parameter(request, QStringLiteral("q")) == QLatin1String("category:photo");
        return jsonResponse(200,
                            {{QStringLiteral("count"), photos ? 1 : 3},
                             {QStringLiteral("effective_query"), QStringLiteral("-rating:nsfw")}});
    });

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_library = std::make_unique<Library>(m_connection.get());
}

void TestLibrary::logIn()
{
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
}

QList<HttpRequest> TestLibrary::searches() const
{
    return m_server->requestsTo("GET", imagesUrl);
}

void TestLibrary::loadsTheFirstPageOnceLoggedIn()
{
    QCOMPARE(m_library->count(), 0);  // not logged in yet

    logIn();

    QTRY_COMPARE(m_library->count(), 2);
    const HttpRequest request = searches().first();
    QCOMPARE(parameter(request, QStringLiteral("limit")), QStringLiteral("100"));
    QCOMPARE(parameter(request, QStringLiteral("sort")), QStringLiteral("taken_at"));
    QCOMPARE(parameter(request, QStringLiteral("order")), QStringLiteral("desc"));
    QCOMPARE(m_library->effectiveQuery(), QStringLiteral("-rating:nsfw"));
    QVERIFY(m_library->canFetchMore({}));
    QVERIFY(m_library->hasMoreToLoad());
}

void TestLibrary::loadsTheNextPageWhenTheGridAsks()
{
    logIn();
    QTRY_COMPARE(m_library->count(), 2);

    m_library->loadMore();

    QTRY_COMPARE(m_library->count(), 3);
    QCOMPARE(parameter(searches().last(), QStringLiteral("cursor")), QStringLiteral("page2:taken_at:"));
    QVERIFY(!m_library->canFetchMore({}));  // that was the last page
    QVERIFY(!m_library->hasMoreToLoad());
}

void TestLibrary::searchesAndStartsOver()
{
    logIn();
    QTRY_COMPARE(m_library->count(), 2);

    m_library->search(QStringLiteral("  category:photo "));

    QTRY_COMPARE(m_library->count(), 1);
    QCOMPARE(m_library->searchText(), QStringLiteral("category:photo"));
    QCOMPARE(parameter(searches().last(), QStringLiteral("q")), QStringLiteral("category:photo"));
    QCOMPARE(m_library->data(m_library->index(0), Library::ImageIdRole).toInt(), 7);
}

void TestLibrary::keepsARandomOrderWhilePaging()
{
    logIn();

    m_library->setOrder(Library::Order::Random);
    QTRY_COMPARE(m_library->count(), 2);
    m_library->fetchMore({});
    QTRY_COMPARE(m_library->count(), 3);

    const QList<HttpRequest> randomSearches = searches().mid(searches().size() - 2);
    QCOMPARE(parameter(randomSearches.first(), QStringLiteral("sort")), QStringLiteral("random"));
    const QString seed = parameter(randomSearches.first(), QStringLiteral("seed"));
    QVERIFY(!seed.isEmpty());
    QCOMPARE(parameter(randomSearches.last(), QStringLiteral("seed")), seed);  // the second page fits the first
}

void TestLibrary::ignoresTheAnswerToAnOlderSearch()
{
    logIn();
    QTRY_COMPARE(m_library->count(), 2);
    m_server->holdAnswers = true;

    m_library->search(QStringLiteral("summer"));          // all images: 1 and 2
    m_library->search(QStringLiteral("category:photo"));  // image 7
    m_server->releaseAnswer(2);                           // the newer search's page is answered first…
    m_server->releaseAnswers();                           // …and the older one after it

    QTRY_COMPARE(m_library->count(), 1);
    QTest::qWait(50);  // give the older answer time to arrive, if it were not ignored
    QCOMPARE(m_library->count(), 1);
    QCOMPARE(m_library->data(m_library->index(0), Library::ImageIdRole).toInt(), 7);
}

// The grid asks for more whenever it changes: when the list is emptied for the new search, when the change
// is announced, as images arrive, as the busy indicator goes. The old search's next page must never be asked
// for in the new one, which the server refuses, and no page twice.
void TestLibrary::neverMixesANewOrderWithTheOldOrdersPages()
{
    logIn();
    QTRY_COMPARE(m_library->count(), 2);  // a next page is waiting
    const auto askForMore = [this]() { m_library->loadMore(); };
    connect(m_library.get(), &Library::modelReset, this, askForMore);
    connect(m_library.get(), &Library::countChanged, this, askForMore);
    connect(m_library.get(), &Library::searchChanged, this, askForMore);
    connect(m_library.get(), &Library::loadingChanged, this, askForMore);

    m_library->setOrder(Library::Order::Random);
    m_library->search(QStringLiteral("summer"));

    QSet<QUrl> asked;
    for (const HttpRequest& request : searches())  // every next page belongs to the search it is asked for in
    {
        const QString cursor = parameter(request, QStringLiteral("cursor"));
        QVERIFY2(cursor.isEmpty() || cursor == cursorFor(request), qPrintable(request.url.toString()));
        if (parameter(request, QStringLiteral("sort")) != QLatin1String("random"))  // a new seed each time
        {
            QVERIFY2(!asked.contains(request.url), qPrintable(request.url.toString()));
            asked.insert(request.url);
        }
    }

    QCOMPARE(m_library->count(), 3);  // both pages of the last search, once
}

void TestLibrary::showsWhyASearchFailed()
{
    m_server->answerWith("GET", imagesUrl, [](const HttpRequest&) {
        return jsonResponse(400,
                            {{QStringLiteral("type"), QStringLiteral("t")},
                             {QStringLiteral("title"), QStringLiteral("Invalid search")},
                             {QStringLiteral("status"), 400},
                             {QStringLiteral("code"), QStringLiteral("invalid_query")},
                             {QStringLiteral("detail"), QStringLiteral("There is no namespace weather.")}});
    });

    logIn();

    QTRY_COMPARE(m_library->errorMessage(), QStringLiteral("Invalid search. There is no namespace weather."));
    QVERIFY(!m_library->canFetchMore({}));  // no endless retries; a new search tries again
}

void TestLibrary::emptiesWhenLoggedOut()
{
    logIn();
    QTRY_COMPARE(m_library->count(), 2);

    m_connection->logOut();

    QCOMPARE(m_library->count(), 0);
    QVERIFY(!m_library->canFetchMore({}));
}

void TestLibrary::givesTheGridWhatItShows()
{
    logIn();
    QTRY_COMPARE(m_library->count(), 2);

    const QModelIndex first = m_library->index(0);

    QCOMPARE(m_library->data(first, Library::ImageIdRole).toInt(), 1);
    QCOMPARE(m_library->data(first, Library::FileNameRole).toString(), QStringLiteral("photo_1.jpg"));
    QCOMPARE(m_library->data(first, Library::ThumbnailUrlRole).toUrl(),
             QUrl(serverUrl + QStringLiteral("/api/v1/images/1/thumbnail?size=256")));
    QCOMPARE(m_library->data(first, Library::LargeThumbnailUrlRole).toUrl(),
             QUrl(serverUrl + QStringLiteral("/api/v1/images/1/thumbnail?size=1024")));
    QCOMPARE(m_library->data(first, Library::TakenAtRole).toDateTime().date(), QDate(2025, 7, 14));
    QCOMPARE(m_library->roleNames().value(Library::ThumbnailUrlRole), QByteArray("thumbnailUrl"));

    QCOMPARE(m_library->imageAt(1)->id, 2);  // all of an image, for the viewer
    QCOMPARE(m_library->imageAt(2), std::nullopt);
    QCOMPARE(m_library->imageAt(-1), std::nullopt);
}

void TestLibrary::knowsTheTotalOfTheSearch()
{
    QCOMPARE(m_library->totalCount(), -1);  // not logged in yet
    logIn();
    QTRY_COMPARE(m_library->totalCount(), 3);
    QCOMPARE(m_library->count(), 2);  // more than the first page holds

    QSignalSpy totalCountChanged(m_library.get(), &Library::totalCountChanged);
    m_library->search(QStringLiteral("category:photo"));

    QTRY_COMPARE(m_library->totalCount(), 1);
    QCOMPARE(totalCountChanged.count(), 2);  // unknown while the new search runs, then its total
    const HttpRequest countRequest = m_server->requestsTo("GET", countUrl).last();
    QCOMPARE(parameter(countRequest, QStringLiteral("q")), QStringLiteral("category:photo"));
    QCOMPARE(parameter(countRequest, QStringLiteral("apply_hidden_filters")), QStringLiteral("true"));
}

void TestLibrary::ignoresTheTotalOfAnOlderSearch()
{
    logIn();
    QTRY_COMPARE(m_library->totalCount(), 3);
    m_server->holdAnswers = true;

    m_library->search(QStringLiteral("summer"));          // a total of 3
    m_library->search(QStringLiteral("category:photo"));  // a total of 1
    m_server->releaseAnswer(3);                           // the newer search's total arrives first…
    m_server->releaseAnswers();                           // …and the older one after it

    QTRY_COMPARE(m_library->totalCount(), 1);
    QTest::qWait(50);
    QCOMPARE(m_library->totalCount(), 1);
}

void TestLibrary::hasNoTotalIfTheServerCannotCount()
{
    m_server->answerWith("GET", countUrl, [](const HttpRequest&) {
        return jsonResponse(500, {{QStringLiteral("code"), QStringLiteral("internal")}});
    });

    logIn();
    QTRY_COMPARE(m_library->count(), 2);

    QCOMPARE(m_library->totalCount(), -1);
    QVERIFY(m_library->errorMessage().isEmpty());  // the grid still works
}

void TestLibrary::replacesAnImageAfterItChanged()
{
    logIn();
    QTRY_COMPARE(m_library->count(), 2);
    QSignalSpy dataChanged(m_library.get(), &Library::dataChanged);
    server_client::Image retagged = *m_library->imageAt(1);
    retagged.tags = {QStringLiteral("general:sunset")};

    m_library->replaceImage(retagged);

    QCOMPARE(m_library->imageAt(1)->tags, QStringList{QStringLiteral("general:sunset")});
    QCOMPARE(dataChanged.count(), 1);
    QCOMPARE(dataChanged.first().first().toModelIndex().row(), 1);

    server_client::Image notShown = retagged;
    notShown.id = 99;
    m_library->replaceImage(notShown);
    QCOMPARE(dataChanged.count(), 1);  // not in the results: nothing changes
}

}  // namespace pictorio::library

QTEST_GUILESS_MAIN(pictorio::library::TestLibrary)
#include "TestLibrary.moc"
