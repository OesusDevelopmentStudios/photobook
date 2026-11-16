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

}  // namespace pictorio::library_view

QTEST_GUILESS_MAIN(pictorio::library_view::TestLibraryView)

#include "TestLibraryView.moc"
