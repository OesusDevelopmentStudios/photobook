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
    QCOMPARE(settings->albumSort(), std::nullopt);
    QCOMPARE(settings->showAlbumsWithHiddenImages(), std::nullopt);
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
        settings->setAlbumSort(QStringLiteral("size"));
        settings->setShowAlbumsWithHiddenImages(true);
    }

    const auto settingsAfterRestart = createSettings(configFilePath());

    QCOMPARE(settingsAfterRestart->slideshowIntervalSeconds(), 12);
    QCOMPARE(settingsAfterRestart->slideshowOrder(), QStringLiteral("date"));
    QCOMPARE(settingsAfterRestart->serverUrl(), QUrl(QStringLiteral("https://photos.example.org")));
    QCOMPARE(settingsAfterRestart->serverUsername(), QStringLiteral("restrictedUser"));
    QCOMPARE(settingsAfterRestart->librarySidebarVisible(), false);
    QCOMPARE(settingsAfterRestart->libraryTileSize(), 14);
    QCOMPARE(settingsAfterRestart->libraryTab(), QStringLiteral("albums"));
    QCOMPARE(settingsAfterRestart->albumSort(), QStringLiteral("size"));
    QCOMPARE(settingsAfterRestart->showAlbumsWithHiddenImages(), true);
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
    settings->setAlbumSort(QStringLiteral("name"));
    settings->setShowAlbumsWithHiddenImages(true);

    const KConfig configFile(configFilePath(), KConfig::SimpleConfig);
    const KConfigGroup slideshowGroup = configFile.group(QStringLiteral("Slideshow"));
    const KConfigGroup serverGroup = configFile.group(QStringLiteral("Server"));
    const KConfigGroup libraryGroup = configFile.group(QStringLiteral("Library"));
    const KConfigGroup albumsGroup = configFile.group(QStringLiteral("Albums"));

    QCOMPARE(slideshowGroup.readEntry("IntervalSeconds", 0), 7);
    QCOMPARE(slideshowGroup.readEntry("Order", QString()), QStringLiteral("random"));
    QCOMPARE(serverGroup.readEntry("Url", QString()), QStringLiteral("http://raspberrypi.local:8737"));
    QCOMPARE(serverGroup.readEntry("Username", QString()), QStringLiteral("restrictedUser"));
    QCOMPARE(libraryGroup.readEntry("SidebarVisible", QString()), QStringLiteral("false"));
    QCOMPARE(libraryGroup.readEntry("TileSize", 0), 12);
    QCOMPARE(libraryGroup.readEntry("Tab", QString()), QStringLiteral("albums"));
    QCOMPARE(albumsGroup.readEntry("Sort", QString()), QStringLiteral("name"));
    QCOMPARE(albumsGroup.readEntry("ShowWithHiddenImages", QString()), QStringLiteral("true"));
    QVERIFY(!serverGroup.keyList().contains(QStringLiteral("Token")));  // the token belongs in the keyring
}

QString TestSettings::configFilePath() const
{
    return m_folder->filePath(QStringLiteral("photobookrc"));
}

}  // namespace photobook::settings

QTEST_GUILESS_MAIN(photobook::settings::TestSettings)

#include "TestSettings.moc"
