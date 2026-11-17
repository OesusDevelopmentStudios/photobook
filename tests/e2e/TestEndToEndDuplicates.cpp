/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>
#include <optional>

#include <KLocalizedString>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "e2e/AppSession.hpp"
#include "e2e/EndToEndServer.hpp"

namespace pictorio::e2e
{

namespace
{

using FileState = uploader::Uploader::FileState;

}  // namespace

// A file the server has already: its upload adds the new tags to the image there, and the file is stored only
// once. Originals go to the trash, when asked, only once the server has the image, never after a failed upload.
// Runs only against the server that tests/e2e/run_end_to_end_tests.py starts.
class TestEndToEndDuplicates : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void addsTheTagsOfADuplicateToTheImage();
    void trashesTheOriginalOfANewImage();
    void trashesTheOriginalOfADuplicate();
    void keepsTheOriginalWhenTheUploadFails();

private:
    std::optional<EndToEndServer> m_server;
    std::unique_ptr<AppSession> m_session;
};

void TestEndToEndDuplicates::initTestCase()
{
    KLocalizedString::setApplicationDomain("pictorio");
    m_server = EndToEndServer::fromEnvironment();
    if (!m_server.has_value())
    {
        QSKIP("No server: run tests/e2e/run_end_to_end_tests.py");
    }
}

void TestEndToEndDuplicates::init()
{
    m_session = std::make_unique<AppSession>();
    QVERIFY2(m_session->logIn(*m_server, userName), qPrintable(m_session->connection().errorMessage()));
}

void TestEndToEndDuplicates::addsTheTagsOfADuplicateToTheImage()
{
    const QString filePath = m_session->newSampleImage(QStringLiteral("tide"));
    const auto [firstState, firstMessage] =
        m_session->upload(filePath, {{QStringLiteral("general"), QStringLiteral("tide")}});
    QVERIFY2(firstState == FileState::Uploaded, qPrintable(firstMessage));

    const auto [state, message] = m_session->upload(filePath,
                                                    {{QStringLiteral("general"), QStringLiteral("low_tide")},
                                                     {QStringLiteral("author"), QStringLiteral("Sample Artist")}});

    QVERIFY2(state == FileState::AlreadyThere, qPrintable(message));
    const QList<server_client::Image> found = m_session->search(QStringLiteral("general:tide"));
    QCOMPARE(found.size(), 1);
    for (const QString& tag :
         {QStringLiteral("general:tide"), QStringLiteral("general:low_tide"), QStringLiteral("author:sample_artist")})
    {
        QVERIFY2(found.first().tags.contains(tag), qPrintable(tag));
    }

    QCOMPARE(storedFiles(*m_server, QStringLiteral("tide")).size(), 1);
}

void TestEndToEndDuplicates::trashesTheOriginalOfANewImage()
{
    const QString filePath = m_session->newSampleImage(QStringLiteral("reef"));
    m_session->uploader().setMoveOriginalsToTrash(true);

    const auto [state, message] = m_session->upload(filePath, {{QStringLiteral("general"), QStringLiteral("reef")}});

    QVERIFY2(state == FileState::Uploaded, qPrintable(message));
    QTRY_COMPARE_WITH_TIMEOUT(m_session->trashCan().files, QStringList({filePath}), answerTimeout);
}

void TestEndToEndDuplicates::trashesTheOriginalOfADuplicate()
{
    const QString filePath = m_session->newSampleImage(QStringLiteral("cliff"));
    const auto [firstState, firstMessage] =
        m_session->upload(filePath, {{QStringLiteral("general"), QStringLiteral("cliff")}});
    QVERIFY2(firstState == FileState::Uploaded, qPrintable(firstMessage));
    QVERIFY(m_session->trashCan().files.isEmpty());  // not asked yet
    m_session->uploader().setMoveOriginalsToTrash(true);

    const auto [state, message] = m_session->upload(filePath, {});

    QVERIFY2(state == FileState::AlreadyThere, qPrintable(message));  // the server has it: the copy may go
    QTRY_COMPARE_WITH_TIMEOUT(m_session->trashCan().files, QStringList({filePath}), answerTimeout);
}

void TestEndToEndDuplicates::keepsTheOriginalWhenTheUploadFails()
{
    QTemporaryDir folder;
    const QString filePath = folder.filePath(QStringLiteral("notes.png"));  // an image only by its name
    QFile notes(filePath);
    QVERIFY(notes.open(QIODevice::WriteOnly));
    notes.write("Not a picture at all.");
    notes.close();
    m_session->uploader().setMoveOriginalsToTrash(true);

    const auto [state, message] = m_session->upload(filePath, {});

    QCOMPARE(state, FileState::Failed);
    QVERIFY(!message.isEmpty());  // the server's reason
    QTest::qWait(500);            // a trash request would follow the answer at once
    QVERIFY(m_session->trashCan().files.isEmpty());
}

}  // namespace pictorio::e2e

QTEST_GUILESS_MAIN(pictorio::e2e::TestEndToEndDuplicates)

#include "TestEndToEndDuplicates.moc"
