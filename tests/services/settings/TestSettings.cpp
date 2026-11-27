/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KConfig>
#include <KConfigGroup>
#include <QTemporaryDir>
#include <QTest>

#include "Settings.hpp"
#include "SettingsFactory.hpp"

namespace photobook::settings
{

// Each test uses its own temporary config file, never the user's real settings.
class TestSettings : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void returnsNothingForValuesNeverStored();
    void keepsValuesAfterRestart();
    void writesReadableConfigFile();

private:
    QString configFilePath() const;

    std::unique_ptr<QTemporaryDir> m_folder;
};

void TestSettings::init()
{
    m_folder = std::make_unique<QTemporaryDir>();
    QVERIFY(m_folder->isValid());
}

void TestSettings::returnsNothingForValuesNeverStored()
{
    const auto settings = createSettings(configFilePath());

    QCOMPARE(settings->slideshowIntervalSeconds(), std::nullopt);
    QCOMPARE(settings->slideshowOrder(), std::nullopt);
    QCOMPARE(settings->serverUrl(), std::nullopt);
    QCOMPARE(settings->serverUsername(), std::nullopt);
    QCOMPARE(settings->librarySidebarVisible(), std::nullopt);
    QCOMPARE(settings->libraryTileSize(), std::nullopt);
    QCOMPARE(settings->libraryTab(), std::nullopt);
    QCOMPARE(settings->libraryTabShown(), std::nullopt);
    QCOMPARE(settings->albumsTabShown(), std::nullopt);
    QCOMPARE(settings->tagPaneOpen(), std::nullopt);
    QCOMPARE(settings->foldedTagNamespaces(), std::nullopt);
    QCOMPARE(settings->albumSort(), std::nullopt);
    QCOMPARE(settings->showAlbumsWithHiddenImages(), std::nullopt);
    QCOMPARE(settings->videoVolume(), std::nullopt);
    QCOMPARE(settings->videoSpeed(), std::nullopt);
    QCOMPARE(settings->startVideosMuted(), std::nullopt);
}

void TestSettings::keepsValuesAfterRestart()
{
    {
        const auto settings = createSettings(configFilePath());
        settings->setSlideshowIntervalSeconds(12);
        settings->setSlideshowOrder(QStringLiteral("date"));
        settings->setServerUrl(QUrl(QStringLiteral("https://photos.example.org")));
        settings->setServerUsername(QStringLiteral("restrictedUser"));
        settings->setLibrarySidebarVisible(false);
        settings->setLibraryTileSize(14);
        settings->setLibraryTab(QStringLiteral("albums"));
        settings->setLibraryTabShown(false);
        settings->setAlbumsTabShown(true);
        settings->setTagPaneOpen(false);
        settings->setFoldedTagNamespaces({QStringLiteral("general"), QStringLiteral("meta")});
        settings->setAlbumSort(QStringLiteral("size"));
        settings->setShowAlbumsWithHiddenImages(true);
        settings->setVideoVolume(0.35);
        settings->setVideoSpeed(1.5);
        settings->setStartVideosMuted(true);
    }

    const auto settingsAfterRestart = createSettings(configFilePath());

    QCOMPARE(settingsAfterRestart->slideshowIntervalSeconds(), 12);
    QCOMPARE(settingsAfterRestart->slideshowOrder(), QStringLiteral("date"));
    QCOMPARE(settingsAfterRestart->serverUrl(), QUrl(QStringLiteral("https://photos.example.org")));
    QCOMPARE(settingsAfterRestart->serverUsername(), QStringLiteral("restrictedUser"));
    QCOMPARE(settingsAfterRestart->librarySidebarVisible(), false);
    QCOMPARE(settingsAfterRestart->libraryTileSize(), 14);
    QCOMPARE(settingsAfterRestart->libraryTab(), QStringLiteral("albums"));
    QCOMPARE(settingsAfterRestart->libraryTabShown(), false);
    QCOMPARE(settingsAfterRestart->albumsTabShown(), true);
    QCOMPARE(settingsAfterRestart->tagPaneOpen(), false);
    QCOMPARE(settingsAfterRestart->foldedTagNamespaces(),
             QStringList({QStringLiteral("general"), QStringLiteral("meta")}));
    QCOMPARE(settingsAfterRestart->albumSort(), QStringLiteral("size"));
    QCOMPARE(settingsAfterRestart->showAlbumsWithHiddenImages(), true);
    QCOMPARE(settingsAfterRestart->videoVolume(), 0.35);
    QCOMPARE(settingsAfterRestart->videoSpeed(), 1.5);
    QCOMPARE(settingsAfterRestart->startVideosMuted(), true);
}

// Group and key names must not change, or users lose their settings with an update.
void TestSettings::writesReadableConfigFile()
{
    const auto settings = createSettings(configFilePath());
    settings->setSlideshowIntervalSeconds(7);
    settings->setSlideshowOrder(QStringLiteral("random"));
    settings->setServerUrl(QUrl(QStringLiteral("http://raspberrypi.local:8737")));
    settings->setServerUsername(QStringLiteral("restrictedUser"));
    settings->setLibrarySidebarVisible(false);
    settings->setLibraryTileSize(12);
    settings->setLibraryTab(QStringLiteral("albums"));
    settings->setAlbumsTabShown(false);
    settings->setTagPaneOpen(false);
    settings->setFoldedTagNamespaces({QStringLiteral("general")});
    settings->setAlbumSort(QStringLiteral("name"));
    settings->setShowAlbumsWithHiddenImages(true);
    settings->setVideoVolume(0.5);
    settings->setVideoSpeed(2);
    settings->setStartVideosMuted(true);

    const KConfig configFile(configFilePath(), KConfig::SimpleConfig);
    const KConfigGroup slideshowGroup = configFile.group(QStringLiteral("Slideshow"));
    const KConfigGroup serverGroup = configFile.group(QStringLiteral("Server"));
    const KConfigGroup libraryGroup = configFile.group(QStringLiteral("Library"));
    const KConfigGroup albumsGroup = configFile.group(QStringLiteral("Albums"));
    const KConfigGroup tagPaneGroup = configFile.group(QStringLiteral("TagPane"));
    const KConfigGroup videoGroup = configFile.group(QStringLiteral("Video"));

    QCOMPARE(slideshowGroup.readEntry("IntervalSeconds", 0), 7);
    QCOMPARE(slideshowGroup.readEntry("Order", QString()), QStringLiteral("random"));
    QCOMPARE(serverGroup.readEntry("Url", QString()), QStringLiteral("http://raspberrypi.local:8737"));
    QCOMPARE(serverGroup.readEntry("Username", QString()), QStringLiteral("restrictedUser"));
    QCOMPARE(libraryGroup.readEntry("SidebarVisible", QString()), QStringLiteral("false"));
    QCOMPARE(libraryGroup.readEntry("TileSize", 0), 12);
    QCOMPARE(libraryGroup.readEntry("Tab", QString()), QStringLiteral("albums"));
    QCOMPARE(libraryGroup.readEntry("ShowAlbumsTab", QString()), QStringLiteral("false"));
    QCOMPARE(albumsGroup.readEntry("Sort", QString()), QStringLiteral("name"));
    QCOMPARE(tagPaneGroup.readEntry("Open", QString()), QStringLiteral("false"));
    QCOMPARE(tagPaneGroup.readEntry("FoldedNamespaces", QString()), QStringLiteral("general"));
    QCOMPARE(albumsGroup.readEntry("ShowWithHiddenImages", QString()), QStringLiteral("true"));
    QCOMPARE(videoGroup.readEntry("Volume", QString()), QStringLiteral("0.5"));
    QCOMPARE(videoGroup.readEntry("Speed", QString()), QStringLiteral("2"));
    QCOMPARE(videoGroup.readEntry("StartMuted", QString()), QStringLiteral("true"));
    QVERIFY(!serverGroup.keyList().contains(QStringLiteral("Token")));  // the token belongs in the keyring
}

QString TestSettings::configFilePath() const
{
    return m_folder->filePath(QStringLiteral("photobookrc"));
}

}  // namespace photobook::settings

QTEST_GUILESS_MAIN(photobook::settings::TestSettings)

#include "TestSettings.moc"
