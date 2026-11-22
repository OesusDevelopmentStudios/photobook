/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>
#include <optional>

#include <KLocalizedString>
#include <QFileInfo>
#include <QFuture>
#include <QTest>

#include "ServerClient.hpp"
#include "e2e/AppSession.hpp"
#include "e2e/EndToEndServer.hpp"
#include "utils/FileHash.hpp"

namespace photobook::e2e
{

namespace
{

using FileState = uploader::Uploader::FileState;

// The server's answer, once it has come; an error with HTTP status 0 if none came in time
template <typename T> server_client::Result<T> answerTo(QFuture<server_client::Result<T>> request)
{
    if (!QTest::qWaitFor([&request]() { return request.isFinished(); }, answerTimeout))
    {
        return std::unexpected(server_client::ApiError{});
    }

    return request.result();
}

}  // namespace

// restrictedUser is restricted from rating:private. sample_user, who is not, uploads the images: a night sky rated private and a
// meadow rated public.
class TestEndToEndRestrictions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void refusesUploadingARestrictedTag();
    void refusesTaggingAnImageWithARestrictedTag();
    void neverShowsARestrictedImage();

private:
    std::optional<EndToEndServer> m_server;
    int m_nightImageId = 0;
    std::unique_ptr<AppSession> m_restrictedUser;
};

void TestEndToEndRestrictions::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
    m_server = EndToEndServer::fromEnvironment();
    if (!m_server.has_value())
    {
        QSKIP("No server: run tests/e2e/run_end_to_end_tests.py");
    }

    AppSession owner;
    QVERIFY(owner.logIn(*m_server, userName));
    const QList<QPair<QString, QString>> images{{QStringLiteral("night_sky"), QStringLiteral("private")},
                                                {QStringLiteral("meadow"), QStringLiteral("public")}};
    for (const auto& [stem, rating] : images)
    {
        const auto [state, message] = owner.upload(
            owner.newSampleImage(stem), {{QStringLiteral("rating"), rating}, {QStringLiteral("general"), stem}});
        QVERIFY2(state == FileState::Uploaded, qPrintable(message));
    }

    const QList<server_client::Image> night = owner.search(QStringLiteral("general:night_sky"));
    QCOMPARE(night.size(), 1);  // sample_user sees it
    m_nightImageId = night.first().id;
}

void TestEndToEndRestrictions::init()
{
    m_restrictedUser = std::make_unique<AppSession>();
    QVERIFY2(m_restrictedUser->logIn(*m_server, restrictedUserName), qPrintable(m_restrictedUser->connection().errorMessage()));
    QTRY_VERIFY_WITH_TIMEOUT(!m_restrictedUser->connection().restrictedTags().isEmpty(), answerTimeout);
}

void TestEndToEndRestrictions::refusesUploadingARestrictedTag()
{
    const QString filePath = m_restrictedUser->newSampleImage(QStringLiteral("stars"));

    const auto [state, message] = m_restrictedUser->upload(filePath, {{QStringLiteral("rating"), QStringLiteral("private")}});

    QCOMPARE(state, FileState::Failed);
    QVERIFY2(message.contains(QStringLiteral("rating:private")), qPrintable(message));  // says which tag
    QCOMPARE(m_restrictedUser->uploader().count(), 0);                                          // refused before any file was sent

    // The same upload sent past the app's check
    const server_client::Result<server_client::Image> sent =
        answerTo(m_restrictedUser->connection().client()->uploadImage({filePath,
                                                                       QFileInfo(filePath).fileName(),
                                                                       std::nullopt,
                                                                       *utils::sha256Of(filePath),
                                                                       {QStringLiteral("rating:private")},
                                                                       {}}));
    QVERIFY(!sent.has_value());
    QCOMPARE(sent.error().httpStatus, 403);
    QCOMPARE(sent.error().code, QStringLiteral("tag_restricted"));
}

void TestEndToEndRestrictions::refusesTaggingAnImageWithARestrictedTag()
{
    const QList<server_client::Image> meadow = m_restrictedUser->search(QStringLiteral("general:meadow"));
    QCOMPARE(meadow.size(), 1);  // a public image: restrictedUser sees it

    const std::optional<server_client::Image> saved =
        m_restrictedUser->editTags(meadow.first(), [](tag_editor::TagEditor& editor) {
            return editor.setSingleValue(QStringLiteral("rating"), QStringLiteral("private"));
        });

    QVERIFY(!saved.has_value());
    QVERIFY2(m_restrictedUser->tagEditorError().contains(QStringLiteral("rating:private")), qPrintable(m_restrictedUser->tagEditorError()));

    // The same change sent past the app's check
    const server_client::Result<server_client::Image> sent = answerTo(m_restrictedUser->connection().client()->changeImageTags(
        meadow.first().id, server_client::TagChanges{{QStringLiteral("rating:private")}, {}}));
    QVERIFY(!sent.has_value());
    QCOMPARE(sent.error().httpStatus, 403);
    QCOMPARE(m_restrictedUser->search(QStringLiteral("general:meadow")).first().tags,
             meadow.first().tags);  // unchanged
}

void TestEndToEndRestrictions::neverShowsARestrictedImage()
{
    QVERIFY(m_restrictedUser->search(QStringLiteral("general:night_sky")).isEmpty());
    QVERIFY(m_restrictedUser->search(QStringLiteral("rating:private")).isEmpty());  // naming the tag does not lift it
    QVERIFY(m_restrictedUser->search(QString()).size() > 0);                     // restrictedUser sees the other images

    // Not by its id either; as if it did not exist
    const server_client::Result<server_client::Image> byId =
        answerTo(m_restrictedUser->connection().client()->image(m_nightImageId));
    QVERIFY(!byId.has_value());
    QCOMPARE(byId.error().httpStatus, 404);
}

}  // namespace photobook::e2e

QTEST_GUILESS_MAIN(photobook::e2e::TestEndToEndRestrictions)

#include "TestEndToEndRestrictions.moc"
