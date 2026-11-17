/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>
#include <optional>

#include <KLocalizedString>
#include <QTest>

#include "e2e/AppSession.hpp"
#include "e2e/EndToEndServer.hpp"

namespace pictorio::e2e
{

namespace
{

using FileState = uploader::Uploader::FileState;

QStringList fileNamesOf(const QList<server_client::Image>& images)
{
    QStringList names;
    for (const server_client::Image& image : images)
    {
        names.append(image.originalFilename);
    }

    names.sort();
    return names;
}

}  // namespace

// Hidden filters saved on the settings page: the library leaves those images out until a search names their
// tag. Two coast images, one of them in fog; the user hides the fog. The filters belong to the user, whom the
// other end-to-end tests share, so each test leaves none behind. Runs only against the server that
// tests/e2e/run_end_to_end_tests.py starts.
class TestEndToEndHiddenFilters : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void leavesAHiddenImageOutOfTheLibrary();
    void findsTheHiddenImageWhenSearchedFor();
    void showsTheImageAgainWithoutTheFilter();

private:
    std::optional<EndToEndServer> m_server;
    std::unique_ptr<AppSession> m_session;
};

void TestEndToEndHiddenFilters::initTestCase()
{
    KLocalizedString::setApplicationDomain("pictorio");
    m_server = EndToEndServer::fromEnvironment();
    if (!m_server.has_value())
    {
        QSKIP("No server: run tests/e2e/run_end_to_end_tests.py");
    }

    AppSession session;
    QVERIFY(session.logIn(*m_server, userName));
    const QList<QPair<QString, QStringList>> images{
        {QStringLiteral("coast_in_fog"), {QStringLiteral("coast"), QStringLiteral("fog")}},
        {QStringLiteral("coast_in_sun"), {QStringLiteral("coast")}},
    };
    for (const auto& [stem, generalTags] : images)
    {
        QList<AppSession::TypedTag> tags;
        for (const QString& tag : generalTags)
        {
            tags.append({QStringLiteral("general"), tag});
        }

        const auto [state, message] = session.upload(session.newSampleImage(stem), tags);
        QVERIFY2(state == FileState::Uploaded, qPrintable(message));
    }
}

void TestEndToEndHiddenFilters::init()
{
    m_session = std::make_unique<AppSession>();
    QVERIFY2(m_session->logIn(*m_server, userName), qPrintable(m_session->connection().errorMessage()));
}

void TestEndToEndHiddenFilters::cleanup()
{
    QVERIFY2(m_session->saveHiddenFilters({}), qPrintable(m_session->hiddenFilterError()));
}

void TestEndToEndHiddenFilters::leavesAHiddenImageOutOfTheLibrary()
{
    QCOMPARE(fileNamesOf(m_session->search(QStringLiteral("general:coast"))),
             QStringList({QStringLiteral("coast_in_fog.png"), QStringLiteral("coast_in_sun.png")}));

    QVERIFY2(m_session->saveHiddenFilters({QStringLiteral("general:fog")}), qPrintable(m_session->hiddenFilterError()));

    QCOMPARE(fileNamesOf(m_session->search(QStringLiteral("general:coast"))),
             QStringList({QStringLiteral("coast_in_sun.png")}));
    QVERIFY2(m_session->effectiveQuery().contains(QStringLiteral("-general:fog")),
             qPrintable(m_session->effectiveQuery()));
}

// Searching for a hidden tag is the explicit wish to see those images.
void TestEndToEndHiddenFilters::findsTheHiddenImageWhenSearchedFor()
{
    QVERIFY(m_session->saveHiddenFilters({QStringLiteral("general:fog")}));

    QCOMPARE(fileNamesOf(m_session->search(QStringLiteral("general:fog"))),
             QStringList({QStringLiteral("coast_in_fog.png")}));
    QCOMPARE(fileNamesOf(m_session->search(QStringLiteral("general:coast -general:fog"))),
             QStringList({QStringLiteral("coast_in_sun.png")}));
}

void TestEndToEndHiddenFilters::showsTheImageAgainWithoutTheFilter()
{
    QVERIFY(m_session->saveHiddenFilters({QStringLiteral("general:fog")}));
    QCOMPARE(m_session->search(QStringLiteral("general:coast")).size(), 1);

    QVERIFY(m_session->saveHiddenFilters({}));

    QCOMPARE(m_session->search(QStringLiteral("general:coast")).size(), 2);
}

}  // namespace pictorio::e2e

QTEST_GUILESS_MAIN(pictorio::e2e::TestEndToEndHiddenFilters)

#include "TestEndToEndHiddenFilters.moc"
