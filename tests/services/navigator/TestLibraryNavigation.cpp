/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrlQuery>

#include "Connection.hpp"
#include "Library.hpp"
#include "NavigatorFactory.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/ImageJson.hpp"

namespace photobook::navigator
{

namespace
{

using connection::Connection;
using fakes::FakeServer;
using library::Library;
using server_client::HttpRequest;

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString imagesUrl = serverUrl + QStringLiteral("/api/v1/images");
const QString countUrl = serverUrl + QStringLiteral("/api/v1/images/count");
constexpr int resultCount = 7;

// Never asked: library images are not in a folder.
class NoFolders : public folder_scanner::FolderScanner
{
public:
    QStringList listImages(const QString& /*folderPath*/, const folder_scanner::Sorting& /*sorting*/) const override
    {
        return {};
    }
};

}  // namespace

// Stepping through the library's search results. The fake server finds seven images, in pages of three:
// rows 0-2, 3-5 and 6.
class TestLibraryNavigation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void stepsThroughTheResultsInTheLibrarysOrder();
    void countsAgainstTheTotalOfTheResults();
    void loadsTheNextPageAhead();
    void waitsForThePageWhenSteppingPastTheLoadedResults();
    void stopsAtTheLastResult();
    void wrapsAroundAtTheEndOfAllResults();
    void wrapsBackOnlyWhenAllResultsAreLoaded();
    void stopsWhenTheResultsChange();
    void leavesTheLibraryForAFile();

private:
    void logInAndWaitForTheFirstPage();
    void loadAllResults();
    QList<int> requestedRows(const QSignalSpy& spy) const;

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<Library> m_library;
    std::unique_ptr<Navigator> m_navigator;
};

void TestLibraryNavigation::init()
{
    m_navigator.reset();  // before the library it browses
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("photobookrc")));
    m_settings->setServerUrl(QUrl(serverUrl));

    m_server = std::make_shared<FakeServer>();
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/auth/me"),
                     fakes::jsonResponse(200,
                                         {{QStringLiteral("id"), 2},
                                          {QStringLiteral("username"), QStringLiteral("kid")},
                                          {QStringLiteral("role"), QStringLiteral("user")},
                                          {QStringLiteral("restricted_tags"), QJsonArray()}}));
    m_server->answerWith("GET", imagesUrl, [](const HttpRequest& request) {
        const QString cursor = QUrlQuery(request.url).queryItemValue(QStringLiteral("cursor"));
        if (cursor == QLatin1String("after_3"))
        {
            return fakes::imagePageJson({4, 5, 6}, QStringLiteral("after_6"));
        }

        if (cursor == QLatin1String("after_6"))
        {
            return fakes::imagePageJson({7}, QString());
        }

        return fakes::imagePageJson({1, 2, 3}, QStringLiteral("after_3"));
    });
    m_server->answer("GET",
                     countUrl,
                     fakes::jsonResponse(200,
                                         {{QStringLiteral("count"), resultCount},
                                          {QStringLiteral("effective_query"), QStringLiteral("-rating:nsfw")}}));

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_library = std::make_unique<Library>(m_connection.get());
    m_navigator = createNavigator(std::make_unique<NoFolders>());
}

void TestLibraryNavigation::logInAndWaitForTheFirstPage()
{
    m_connection->restoreLogin();
    QTRY_COMPARE(m_library->count(), 3);
}

void TestLibraryNavigation::loadAllResults()
{
    while (m_library->hasMoreToLoad())
    {
        const int loaded = m_library->count();
        m_library->loadMore();
        QTRY_VERIFY(m_library->count() > loaded);
    }
}

QList<int> TestLibraryNavigation::requestedRows(const QSignalSpy& spy) const
{
    QList<int> rows;
    for (const QList<QVariant>& arguments : spy)
    {
        rows.append(arguments.first().toInt());
    }

    return rows;
}

void TestLibraryNavigation::stepsThroughTheResultsInTheLibrarysOrder()
{
    logInAndWaitForTheFirstPage();
    QSignalSpy requested(m_navigator.get(), &Navigator::libraryImageRequested);
    QSignalSpy filesRequested(m_navigator.get(), &Navigator::imageRequested);

    m_navigator->openLibraryImage(m_library.get(), 1);
    m_navigator->goToNext();
    m_navigator->goToPrevious();
    m_navigator->goToPrevious();

    QCOMPARE(requestedRows(requested), QList<int>({1, 2, 1, 0}));
    QCOMPARE(filesRequested.count(), 0);
    QVERIFY(m_navigator->isBrowsingLibrary());
    QCOMPARE(m_navigator->currentIndex(), 0);
    QVERIFY(!m_navigator->canGoToPrevious());  // the first result, and no wrap-around
}

void TestLibraryNavigation::countsAgainstTheTotalOfTheResults()
{
    logInAndWaitForTheFirstPage();
    QTRY_COMPARE(m_library->totalCount(), resultCount);

    m_navigator->openLibraryImage(m_library.get(), 1);

    QCOMPARE(m_navigator->imageCount(), resultCount);  // not only the loaded ones
}

// Close to the end of the loaded results, the next page is asked for before anyone steps there.
void TestLibraryNavigation::loadsTheNextPageAhead()
{
    logInAndWaitForTheFirstPage();

    m_navigator->openLibraryImage(m_library.get(), 0);

    QTRY_COMPARE(m_library->count(), resultCount);
}

void TestLibraryNavigation::waitsForThePageWhenSteppingPastTheLoadedResults()
{
    logInAndWaitForTheFirstPage();
    m_server->holdAnswers = true;
    m_navigator->openLibraryImage(m_library.get(), 2);  // the last loaded one; the next page is on its way
    QSignalSpy requested(m_navigator.get(), &Navigator::libraryImageRequested);

    QVERIFY(m_navigator->canGoToNext());
    m_navigator->goToNext();
    QCOMPARE(requested.count(), 0);  // not loaded yet

    m_server->releaseAnswers();

    QTRY_COMPARE(requestedRows(requested), QList<int>({3}));
    QCOMPARE(m_navigator->currentIndex(), 3);
}

void TestLibraryNavigation::stopsAtTheLastResult()
{
    logInAndWaitForTheFirstPage();
    loadAllResults();

    m_navigator->openLibraryImage(m_library.get(), resultCount - 1);

    QVERIFY(!m_navigator->canGoToNext());
    QVERIFY(m_navigator->canGoToPrevious());
}

// As a slideshow does
void TestLibraryNavigation::wrapsAroundAtTheEndOfAllResults()
{
    logInAndWaitForTheFirstPage();
    loadAllResults();
    m_navigator->setWrapAround(true);
    m_navigator->openLibraryImage(m_library.get(), resultCount - 1);
    QSignalSpy requested(m_navigator.get(), &Navigator::libraryImageRequested);

    m_navigator->goToNext();
    m_navigator->goToPrevious();

    QCOMPARE(requestedRows(requested), QList<int>({0, resultCount - 1}));
}

// Before the last page is in, the last result is not known.
void TestLibraryNavigation::wrapsBackOnlyWhenAllResultsAreLoaded()
{
    logInAndWaitForTheFirstPage();
    m_server->holdAnswers = true;
    m_navigator->setWrapAround(true);

    m_navigator->openLibraryImage(m_library.get(), 0);

    QVERIFY(!m_navigator->canGoToPrevious());
    QVERIFY(m_navigator->canGoToNext());
}

void TestLibraryNavigation::stopsWhenTheResultsChange()
{
    logInAndWaitForTheFirstPage();
    m_navigator->openLibraryImage(m_library.get(), 1);
    QSignalSpy requested(m_navigator.get(), &Navigator::libraryImageRequested);
    QSignalSpy positionChanged(m_navigator.get(), &Navigator::positionChanged);

    m_library->search(QStringLiteral("summer"));
    QTRY_COMPARE(m_library->count(), 3);

    QVERIFY(!m_navigator->isBrowsingLibrary());
    QVERIFY(positionChanged.count() > 0);
    QCOMPARE(m_navigator->currentIndex(), -1);
    QVERIFY(!m_navigator->canGoToNext());
    QVERIFY(!m_navigator->canGoToPrevious());
    QCOMPARE(requested.count(), 0);  // the image on screen stays
}

void TestLibraryNavigation::leavesTheLibraryForAFile()
{
    logInAndWaitForTheFirstPage();
    m_server->holdAnswers = true;
    m_navigator->openLibraryImage(m_library.get(), 2);  // the next page is on its way
    m_navigator->goToNext();                            // and awaited
    QSignalSpy requested(m_navigator.get(), &Navigator::libraryImageRequested);

    m_navigator->openImage(QStringLiteral("/photos/beach.jpg"));
    m_server->releaseAnswers();

    QTRY_COMPARE(m_library->count(), 6);
    QVERIFY(!m_navigator->isBrowsingLibrary());
    QCOMPARE(requested.count(), 0);  // the file stays on screen
}

}  // namespace photobook::navigator

QTEST_GUILESS_MAIN(photobook::navigator::TestLibraryNavigation)

#include "TestLibraryNavigation.moc"
