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
}

void TestSettings::keepsValuesAfterRestart()
{
    {
        const auto settings = createSettings(configFilePath());
        settings->setSlideshowIntervalSeconds(12);
        settings->setSlideshowOrder(QStringLiteral("date"));
        settings->setServerUrl(QUrl(QStringLiteral("https://photos.example.org")));
        settings->setServerUsername(QStringLiteral("kid"));
        settings->setLibrarySidebarVisible(false);
        settings->setLibraryTileSize(14);
    }

    const auto settingsAfterRestart = createSettings(configFilePath());

    QCOMPARE(settingsAfterRestart->slideshowIntervalSeconds(), 12);
    QCOMPARE(settingsAfterRestart->slideshowOrder(), QStringLiteral("date"));
    QCOMPARE(settingsAfterRestart->serverUrl(), QUrl(QStringLiteral("https://photos.example.org")));
    QCOMPARE(settingsAfterRestart->serverUsername(), QStringLiteral("kid"));
    QCOMPARE(settingsAfterRestart->librarySidebarVisible(), false);
    QCOMPARE(settingsAfterRestart->libraryTileSize(), 14);
}

// Group and key names must not change, or users lose their settings with an update.
void TestSettings::writesReadableConfigFile()
{
    const auto settings = createSettings(configFilePath());
    settings->setSlideshowIntervalSeconds(7);
    settings->setSlideshowOrder(QStringLiteral("random"));
    settings->setServerUrl(QUrl(QStringLiteral("http://raspberrypi.local:8737")));
    settings->setServerUsername(QStringLiteral("kid"));
    settings->setLibrarySidebarVisible(false);
    settings->setLibraryTileSize(12);

    const KConfig configFile(configFilePath(), KConfig::SimpleConfig);
    const KConfigGroup slideshowGroup = configFile.group(QStringLiteral("Slideshow"));
    const KConfigGroup serverGroup = configFile.group(QStringLiteral("Server"));
    const KConfigGroup libraryGroup = configFile.group(QStringLiteral("Library"));

    QCOMPARE(slideshowGroup.readEntry("IntervalSeconds", 0), 7);
    QCOMPARE(slideshowGroup.readEntry("Order", QString()), QStringLiteral("random"));
    QCOMPARE(serverGroup.readEntry("Url", QString()), QStringLiteral("http://raspberrypi.local:8737"));
    QCOMPARE(serverGroup.readEntry("Username", QString()), QStringLiteral("kid"));
    QCOMPARE(libraryGroup.readEntry("SidebarVisible", QString()), QStringLiteral("false"));
    QCOMPARE(libraryGroup.readEntry("TileSize", 0), 12);
    QVERIFY(!serverGroup.keyList().contains(QStringLiteral("Token")));  // the token belongs in the keyring
}

QString TestSettings::configFilePath() const
{
    return m_folder->filePath(QStringLiteral("photobookrc"));
}

}  // namespace photobook::settings

QTEST_GUILESS_MAIN(photobook::settings::TestSettings)

#include "TestSettings.moc"
