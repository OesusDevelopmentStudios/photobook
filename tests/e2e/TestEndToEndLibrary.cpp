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

bool allStartWith(const QStringList& paths, const QString& folder)
{
    return std::ranges::all_of(paths, [&folder](const QString& path) { return path.startsWith(folder); });
}

}  // namespace

// An image's way into the library: uploaded with its tags, found by a search, and its file moved on the server
// when a tag that forms its path changes. The server lays out files as {category}/{author}/{year}. Runs only
// against the server that tests/e2e/run_end_to_end_tests.py starts.
class TestEndToEndLibrary : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void uploadsAnImageWithItsTags();
    void findsTheImageBySearch();
    void movesTheFileWhenAPathTagChanges();

private:
    // A new image uploaded as a photo by sample_artist, tagged general:<generalTag>, as the library finds it
    std::optional<server_client::Image> uploadedPhoto(const QString& stem, const QString& generalTag);

    std::optional<EndToEndServer> m_server;
    std::unique_ptr<AppSession> m_session;
};

void TestEndToEndLibrary::initTestCase()
{
    KLocalizedString::setApplicationDomain("pictorio");
    m_server = EndToEndServer::fromEnvironment();
    if (!m_server.has_value())
    {
        QSKIP("No server: run tests/e2e/run_end_to_end_tests.py");
    }
}

void TestEndToEndLibrary::init()
{
    m_session = std::make_unique<AppSession>();
    QVERIFY2(m_session->logIn(*m_server, userName), qPrintable(m_session->connection().errorMessage()));
}

void TestEndToEndLibrary::uploadsAnImageWithItsTags()
{
    const QString filePath = m_session->newSampleImage(QStringLiteral("harbour"));

    const auto [state, message] = m_session->upload(filePath,
                                                    {{QStringLiteral("category"), QStringLiteral("photo")},
                                                     {QStringLiteral("author"), QStringLiteral("Sample Artist")},
                                                     {QStringLiteral("general"), QStringLiteral("harbour")}});

    QVERIFY2(state == FileState::Uploaded, qPrintable(message));
    const QStringList stored = storedFiles(*m_server, QStringLiteral("harbour"));
    QCOMPARE(stored.size(), 1);
    QVERIFY2(stored.first().startsWith(QStringLiteral("photo/sample_artist/")), qPrintable(stored.first()));
    QVERIFY(m_session->trashCan().files.isEmpty());  // the originals stay unless asked
}

void TestEndToEndLibrary::findsTheImageBySearch()
{
    QVERIFY(uploadedPhoto(QStringLiteral("lighthouse"), QStringLiteral("lighthouse")).has_value());

    const QList<server_client::Image> found =
        m_session->search(QStringLiteral("general:lighthouse author:sample_artist"));

    QVERIFY2(found.size() == 1, qPrintable(m_session->libraryError()));
    QVERIFY(found.first().tags.contains(QStringLiteral("category:photo")));
    QVERIFY(m_session->search(QStringLiteral("general:lighthouse category:artwork")).isEmpty());
}

void TestEndToEndLibrary::movesTheFileWhenAPathTagChanges()
{
    const std::optional<server_client::Image> photo = uploadedPhoto(QStringLiteral("dunes"), QStringLiteral("dunes"));
    QVERIFY(photo.has_value());
    QVERIFY(allStartWith(storedFiles(*m_server, QStringLiteral("dunes")), QStringLiteral("photo/sample_artist/")));

    const std::optional<server_client::Image> saved = m_session->editTags(*photo, [](tag_editor::TagEditor& editor) {
        return editor.setSingleValue(QStringLiteral("category"), QStringLiteral("artwork"));
    });

    QVERIFY2(saved.has_value(), qPrintable(m_session->tagEditorError()));
    QVERIFY(saved->tags.contains(QStringLiteral("category:artwork")));
    const QStringList stored = storedFiles(*m_server, QStringLiteral("dunes"));
    QCOMPARE(stored.size(), 1);  // moved, not copied
    QVERIFY2(stored.first().startsWith(QStringLiteral("artwork/sample_artist/")), qPrintable(stored.first()));
    QCOMPARE(m_session->search(QStringLiteral("general:dunes category:artwork")).size(), 1);
}

std::optional<server_client::Image> TestEndToEndLibrary::uploadedPhoto(const QString& stem, const QString& generalTag)
{
    const auto [state, message] = m_session->upload(m_session->newSampleImage(stem),
                                                    {{QStringLiteral("category"), QStringLiteral("photo")},
                                                     {QStringLiteral("author"), QStringLiteral("Sample Artist")},
                                                     {QStringLiteral("general"), generalTag}});
    if (state != FileState::Uploaded)
    {
        qWarning() << "Upload failed:" << message;
        return std::nullopt;
    }

    const QList<server_client::Image> found = m_session->search(QStringLiteral("general:") + generalTag);
    return found.size() == 1 ? std::optional(found.first()) : std::nullopt;
}

}  // namespace pictorio::e2e

QTEST_GUILESS_MAIN(pictorio::e2e::TestEndToEndLibrary)

#include "TestEndToEndLibrary.moc"
