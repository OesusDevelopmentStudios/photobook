/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <QApplication>
#include <QQmlEngine>
#include <QQmlEngineExtensionPlugin>
#include <QQuickStyle>
#include <QtQuickTest>

#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include "FakeTagRowsEditor.hpp"
#include "TestServices.hpp"

// The application's QML module, as main.cpp uses it
Q_IMPORT_QML_PLUGIN(io_github_oesusdevelopmentstudios_photobookPlugin)

namespace
{

// Each test file's engine is set up as the application's is.
class Setup : public QObject
{
    Q_OBJECT

public Q_SLOTS:

    void qmlEngineAvailable(QQmlEngine* engine)
    {
        KLocalization::setupLocalizedContext(engine);  // KI18n.i18n*() in QML
    }
};

}  // namespace

// The QML tests in tests/qml, tst_*.qml. They use the application's QML module, and fakes from the module
// "photobook.tests".
int main(int argc, char* argv[])
{
    QApplication application(argc, argv);  // the KDE style needs widgets
    KLocalizedString::setApplicationDomain("photobook");
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    qmlRegisterType<photobook::tests::FakeTagRowsEditor>("photobook.tests", 1, 0, "FakeTagRowsEditor");
    photobook::tests::TestServices services;
    qmlRegisterSingletonInstance("photobook.tests", 1, 0, "TestServices", &services);

    Setup setup;
    return quick_test_main_with_setup(argc, argv, "TestQml", QUICK_TEST_SOURCE_DIR, &setup);
}

#include "TestQml.moc"
