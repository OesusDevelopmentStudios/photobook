/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "LibraryView.hpp"
#include "LibraryViewFactory.hpp"
#include "SettingsFactory.hpp"

namespace pictorio::library_view
{

// The settings are in a temporary config file, never the user's real one.
class TestLibraryView : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void showsSidebarAtFirstStart();
    void remembersHiddenSidebar();
    void announcesOnlyChanges();

    void startsAtTheDefaultTileSize();
    void limitsTileSize();
    void remembersTileSize();
    void limitsRememberedTileSize();
    void resizesTilesByWholeSteps();
    void addsUpPartialSteps();
    void respondsAtOnceAfterALimit();
    void usesTheLargeThumbnailAboveTheSmallOnesSize();

private:
    // A new library view reads the settings, like one created when the application starts.
    void recreateLibraryView();

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<LibraryView> m_libraryView;
};

void TestLibraryView::init()
{
    m_libraryView.reset();  // before the settings it uses
    m_folder = std::make_unique<QTemporaryDir>();
    QVERIFY(m_folder->isValid());
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("pictoriorc")));
    recreateLibraryView();
}

void TestLibraryView::recreateLibraryView()
{
    m_libraryView = createLibraryView(m_settings.get());
}

void TestLibraryView::showsSidebarAtFirstStart()
{
    QVERIFY(m_libraryView->isSidebarVisible());
}

void TestLibraryView::remembersHiddenSidebar()
{
    m_libraryView->setSidebarVisible(false);
    recreateLibraryView();

    QVERIFY(!m_libraryView->isSidebarVisible());
}

void TestLibraryView::announcesOnlyChanges()
{
    QSignalSpy sidebarVisibleChanged(m_libraryView.get(), &LibraryView::sidebarVisibleChanged);

    m_libraryView->setSidebarVisible(true);
    QCOMPARE(sidebarVisibleChanged.count(), 0);

    m_libraryView->setSidebarVisible(false);
    QCOMPARE(sidebarVisibleChanged.count(), 1);
}

void TestLibraryView::startsAtTheDefaultTileSize()
{
    QCOMPARE(m_libraryView->tileSize(), LibraryView::defaultTileSize);
}

void TestLibraryView::limitsTileSize()
{
    m_libraryView->setTileSize(0);
    QCOMPARE(m_libraryView->tileSize(), LibraryView::minimumTileSize);

    m_libraryView->setTileSize(1000);
    QCOMPARE(m_libraryView->tileSize(), LibraryView::maximumTileSize);
}

void TestLibraryView::remembersTileSize()
{
    m_libraryView->setTileSize(14);
    recreateLibraryView();

    QCOMPARE(m_libraryView->tileSize(), 14);
}

// E.g. a config file edited by hand
void TestLibraryView::limitsRememberedTileSize()
{
    m_settings->setLibraryTileSize(1000);
    recreateLibraryView();

    QCOMPARE(m_libraryView->tileSize(), LibraryView::maximumTileSize);
}

void TestLibraryView::resizesTilesByWholeSteps()
{
    QSignalSpy tileSizeChanged(m_libraryView.get(), &LibraryView::tileSizeChanged);

    m_libraryView->resizeTilesBySteps(1);
    QCOMPARE(m_libraryView->tileSize(), LibraryView::defaultTileSize + 1);

    m_libraryView->resizeTilesBySteps(-3);
    QCOMPARE(m_libraryView->tileSize(), LibraryView::defaultTileSize - 2);
    QCOMPARE(tileSizeChanged.count(), 2);
}

// A touchpad sends many small parts of a step.
void TestLibraryView::addsUpPartialSteps()
{
    m_libraryView->resizeTilesBySteps(0.4);
    m_libraryView->resizeTilesBySteps(0.4);
    QCOMPARE(m_libraryView->tileSize(), LibraryView::defaultTileSize);

    m_libraryView->resizeTilesBySteps(0.4);
    QCOMPARE(m_libraryView->tileSize(), LibraryView::defaultTileSize + 1);
}

// Scrolling on past the limit must not have to be scrolled back before the tiles shrink again.
void TestLibraryView::respondsAtOnceAfterALimit()
{
    m_libraryView->setTileSize(LibraryView::maximumTileSize);

    m_libraryView->resizeTilesBySteps(5.5);
    m_libraryView->resizeTilesBySteps(-1);

    QCOMPARE(m_libraryView->tileSize(), LibraryView::maximumTileSize - 1);
}

void TestLibraryView::usesTheLargeThumbnailAboveTheSmallOnesSize()
{
    QVERIFY(!m_libraryView->needsLargeThumbnail(256));
    QVERIFY(m_libraryView->needsLargeThumbnail(256.5));
}

}  // namespace pictorio::library_view

QTEST_GUILESS_MAIN(pictorio::library_view::TestLibraryView)

#include "TestLibraryView.moc"
