/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>
#include <optional>

#include <KLocalizedString>
#include <QDir>
#include <QFileInfo>
#include <QFuture>
#include <QTemporaryDir>
#include <QTest>

#include "ServerClient.hpp"
#include "ServerClientFactory.hpp"
#include "e2e/AppSession.hpp"
#include "e2e/EndToEndServer.hpp"
#include "utils/MachineId.hpp"

namespace pictorio::e2e
{

namespace
{

using FileState = uploader::Uploader::FileState;

QString canonicalPath(const QString& path)
{
    return QFileInfo(path).canonicalFilePath();
}

}  // namespace

// The server's machine ID is the script's file, read by the app as this computer's.
class TestEndToEndLocalLibrary : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void learnsTheLibraryFolderOnTheSameComputer();
    void findsAnUploadedImagesFileInTheLibraryFolder();
    void learnsNothingOnAnotherComputer();

private:
    QString settingsFile() const;
    QString thisComputersId() const;

    std::optional<EndToEndServer> m_server;
    std::unique_ptr<QTemporaryDir> m_folder;
    std::shared_ptr<fakes::Keyring> m_keyring;
};

void TestEndToEndLocalLibrary::initTestCase()
{
    KLocalizedString::setApplicationDomain("pictorio");
    m_server = EndToEndServer::fromEnvironment();
    if (!m_server.has_value())
    {
        QSKIP("No server: run tests/e2e/run_end_to_end_tests.py");
    }

    QVERIFY2(!thisComputersId().isEmpty(), qPrintable(m_server->machineIdFile));
}

void TestEndToEndLocalLibrary::init()
{
    m_folder = std::make_unique<QTemporaryDir>();
    m_keyring = std::make_shared<fakes::Keyring>();
}

void TestEndToEndLocalLibrary::learnsTheLibraryFolderOnTheSameComputer()
{
    const AppConnection connection(settingsFile(), m_keyring, thisComputersId());

    QVERIFY(connection.logIn(*m_server, userName, m_server->password));

    // Asked for right after logging in
    QTRY_VERIFY_WITH_TIMEOUT(!connection->localLibraryDir().isEmpty(), answerTimeout);
    QCOMPARE(canonicalPath(connection->localLibraryDir()), canonicalPath(m_server->libraryDir));
}

void TestEndToEndLocalLibrary::findsAnUploadedImagesFileInTheLibraryFolder()
{
    AppSession session;
    QVERIFY(session.logIn(*m_server, userName));
    const auto [state, message] = session.upload(session.newSampleImage(QStringLiteral("windmill")),
                                                 {{QStringLiteral("category"), QStringLiteral("photo")},
                                                  {QStringLiteral("author"), QStringLiteral("Sample Artist")},
                                                  {QStringLiteral("general"), QStringLiteral("windmill")}});
    QVERIFY2(state == FileState::Uploaded, qPrintable(message));
    const QList<server_client::Image> found = session.search(QStringLiteral("general:windmill"));
    QVERIFY2(found.size() == 1, qPrintable(session.libraryError()));

    const AppConnection connection(settingsFile(), m_keyring, thisComputersId());
    QVERIFY(connection.logIn(*m_server, userName, m_server->password));
    QTRY_VERIFY_WITH_TIMEOUT(!connection->localLibraryDir().isEmpty(), answerTimeout);

    const QString filePath = QDir(connection->localLibraryDir()).filePath(found.first().storagePath);
    QVERIFY2(QFileInfo::exists(filePath), qPrintable(filePath));
}

void TestEndToEndLocalLibrary::learnsNothingOnAnotherComputer()
{
    const QString anotherComputersId = utils::appSpecificMachineId(QStringLiteral("0123456789abcdef0123456789abcdef"));
    const AppConnection connection(settingsFile(), m_keyring, anotherComputersId);
    QVERIFY(connection.logIn(*m_server, userName, m_server->password));

    // The app's question again, asked after it: once this one is answered, the app has most likely had its answer.
    const std::unique_ptr<server_client::ServerClient> client = server_client::createServerClient(m_server->url);
    client->setToken(connection->token());
    QFuture<server_client::Result<server_client::LocalLibrary>> answer = client->localLibrary(anotherComputersId);
    QVERIFY(QTest::qWaitFor([&answer]() { return answer.isFinished(); }, answerTimeout));

    QVERIFY(!answer.result().has_value());
    QCOMPARE(answer.result().error().code, QStringLiteral("not_same_machine"));
    QCOMPARE(connection->localLibraryDir(), QString());
}

QString TestEndToEndLocalLibrary::settingsFile() const
{
    return m_folder->filePath(QStringLiteral("pictoriorc"));
}

QString TestEndToEndLocalLibrary::thisComputersId() const
{
    return utils::thisComputersAppSpecificId(m_server->machineIdFile);
}

}  // namespace pictorio::e2e

QTEST_GUILESS_MAIN(pictorio::e2e::TestEndToEndLocalLibrary)

#include "TestEndToEndLocalLibrary.moc"
