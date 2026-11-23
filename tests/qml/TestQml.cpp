/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <QApplication>
#include <QQmlAbstractUrlInterceptor>
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

// The fake servers' addresses are made up: an image from them is a blank pixel instead, so no test reaches the
// network.
class NoNetwork : public QQmlAbstractUrlInterceptor
{
public:
    QUrl intercept(const QUrl& url, DataType /*type*/) override
    {
        static const QUrl blankPixel(QStringLiteral(
            "data:image/"
            "png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII="));
        return url.scheme().startsWith(QLatin1String("http")) ? blankPixel : url;
    }
};

// Each test file's engine is set up as the application's is, without the network.
class Setup : public QObject
{
    Q_OBJECT

public Q_SLOTS:

    void qmlEngineAvailable(QQmlEngine* engine)
    {
        KLocalization::setupLocalizedContext(engine);  // KI18n.i18n*() in QML
        engine->addUrlInterceptor(&m_noNetwork);
    }

private:
    NoNetwork m_noNetwork;
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
