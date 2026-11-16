/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <cstdlib>

#include <KAboutData>
#include <KLocalizedQmlContext>
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QQmlEngineExtensionPlugin>
#include <QQuickStyle>

#include "app_manager/AppManager.hpp"
#include "utils/AppIdentity.hpp"
#include "utils/CommandLine.hpp"

// The user interface's QML module, a static library (see pictorio_ui in src/CMakeLists.txt)
Q_IMPORT_QML_PLUGIN(io_github_oesusdevelopmentstudios_pictorioPlugin)

namespace
{

// Uses the KDE look for buttons, menus etc., also outside Plasma.
// A style the user picked with the QT_QUICK_CONTROLS_STYLE environment variable takes precedence.
void selectQuickControlsStyle()
{
    const bool userChoseStyle = !qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE");
    if (!userChoseStyle)
    {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);

    pictorio::utils::registerApplicationIdentity();
    selectQuickControlsStyle();

    QCommandLineParser commandLineParser;
    pictorio::utils::addCommandLineOptions(commandLineParser);
    commandLineParser.process(application);  // exits on --help, --version and errors
    KAboutData::applicationData().processCommandLine(&commandLineParser);
    const pictorio::utils::CommandLineOptions commandLineOptions =
        pictorio::utils::readCommandLineOptions(commandLineParser, QDir::currentPath());

    pictorio::app_manager::AppManager appManager;
    const bool openedAtStart = !commandLineOptions.imagePaths.isEmpty();
    if (openedAtStart)
    {
        const QString& path = commandLineOptions.imagePaths.first();
        if (QFileInfo(path).isDir())
        {
            appManager.navigator()->openFolder(path);  // e.g. pictorio ~/Pictures/holiday
        }
        else
        {
            appManager.navigator()->openImage(path);
        }
    }

    QQmlApplicationEngine qmlEngine;
    KLocalization::setupLocalizedContext(&qmlEngine);                             // makes KI18n.i18n*() work in QML
    qmlEngine.setNetworkAccessManagerFactory(appManager.networkAccessFactory());  // owned by the AppManager

    qmlEngine.setInitialProperties({
        {QStringLiteral("appManager"), QVariant::fromValue(&appManager)},
        {QStringLiteral("openedAtStart"), openedAtStart},
    });

    // The QML module URI is the application ID (see pictorio_ui in src/CMakeLists.txt).
    qmlEngine.loadFromModule(pictorio::utils::applicationId, QStringLiteral("Main"));

    const bool mainWindowLoaded = !qmlEngine.rootObjects().isEmpty();
    if (!mainWindowLoaded)
    {
        return EXIT_FAILURE;
    }

    return application.exec();
}
