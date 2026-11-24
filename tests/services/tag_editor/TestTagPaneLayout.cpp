/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "SettingsFactory.hpp"
#include "TagEditorFactory.hpp"

namespace photobook::tag_editor
{

class TestTagPaneLayout : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void isOpenAtFirst();
    void remembersAClosedPane();
    void foldsAndUnfoldsANamespace();
    void remembersTheFoldedNamespaces();

private:
    // A new layout reads the settings, as one made when the application starts.
    void recreateLayout();

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<TagPaneLayout> m_layout;
};

void TestTagPaneLayout::init()
{
    m_layout.reset();
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("photobookrc")));
    recreateLayout();
}

void TestTagPaneLayout::recreateLayout()
{
    m_layout = createTagPaneLayout(m_settings.get());
}

void TestTagPaneLayout::isOpenAtFirst()
{
    QVERIFY(m_layout->isOpen());
    QVERIFY(m_layout->foldedNamespaces().isEmpty());
}

void TestTagPaneLayout::remembersAClosedPane()
{
    QSignalSpy changed(m_layout.get(), &TagPaneLayout::openChanged);

    m_layout->setOpen(false);
    m_layout->setOpen(false);

    QCOMPARE(changed.size(), 1);
    recreateLayout();
    QVERIFY(!m_layout->isOpen());
}

void TestTagPaneLayout::foldsAndUnfoldsANamespace()
{
    QSignalSpy changed(m_layout.get(), &TagPaneLayout::foldedNamespacesChanged);

    m_layout->setFolded(QStringLiteral("general"), true);
    m_layout->setFolded(QStringLiteral("general"), true);

    QVERIFY(m_layout->isFolded(QStringLiteral("general")));
    QVERIFY(!m_layout->isFolded(QStringLiteral("meta")));
    QCOMPARE(changed.size(), 1);

    m_layout->setFolded(QStringLiteral("general"), false);

    QVERIFY(!m_layout->isFolded(QStringLiteral("general")));
    QCOMPARE(changed.size(), 2);
}

void TestTagPaneLayout::remembersTheFoldedNamespaces()
{
    m_layout->setFolded(QStringLiteral("general"), true);
    m_layout->setFolded(QStringLiteral("meta"), true);

    recreateLayout();

    QCOMPARE(m_layout->foldedNamespaces(), QStringList({QStringLiteral("general"), QStringLiteral("meta")}));
}

}  // namespace photobook::tag_editor

QTEST_GUILESS_MAIN(photobook::tag_editor::TestTagPaneLayout)

#include "TestTagPaneLayout.moc"
