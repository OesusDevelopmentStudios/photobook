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

namespace photobook::e2e
{

namespace
{

using FileState = uploader::Uploader::FileState;

const QString albumName = QStringLiteral("e2e_trip");
const QString albumTag = QStringLiteral("album:") + albumName;
const QString hiddenTag = QStringLiteral("general:e2e_misty");

QStringList fileNamesOf(const QList<server_client::Image>& images)
{
    QStringList names;
    for (const server_client::Image& image : images)
    {
        names.append(image.originalFilename);
    }

    return names;
}

std::optional<server_client::Album> albumIn(const QList<server_client::Album>& albums, const QString& tag)
{
    for (const server_client::Album& album : albums)
    {
        if (album.tag == tag)
        {
            return album;
        }
    }

    return std::nullopt;
}

}  // namespace

// Three images put into an album one by one, in an order that is neither their names' nor their dates': forest,
// lake, harbour. The lake is misty, which the user may hide. The filters belong to the user, whom the other
// end-to-end tests share, so each test leaves none behind.
class TestEndToEndAlbums : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void opensAnAlbumInTheOrderItsImagesWereAdded();
    void listsTheAlbumWithItsFirstImageAsCover();
    void showsHiddenImagesInAnOpenedAlbum();

private:
    std::optional<EndToEndServer> m_server;
    std::unique_ptr<AppSession> m_session;
};

void TestEndToEndAlbums::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
    m_server = EndToEndServer::fromEnvironment();
    if (!m_server.has_value())
    {
        QSKIP("No server: run tests/e2e/run_end_to_end_tests.py");
    }

    AppSession session;
    QVERIFY(session.logIn(*m_server, userName));
    const QList<QPair<QString, QStringList>> images{
        {QStringLiteral("a_harbour"), {QStringLiteral("e2e_album_sample")}},
        {QStringLiteral("b_forest"), {QStringLiteral("e2e_album_sample")}},
        {QStringLiteral("c_lake"), {QStringLiteral("e2e_album_sample"), QStringLiteral("e2e_misty")}},
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

    const QList<server_client::Image> uploaded = session.search(QStringLiteral("general:e2e_album_sample"));
    QCOMPARE(uploaded.size(), 3);
    for (const QString& fileName :
         {QStringLiteral("b_forest.png"), QStringLiteral("c_lake.png"), QStringLiteral("a_harbour.png")})
    {
        const auto image = std::ranges::find(uploaded, fileName, &server_client::Image::originalFilename);
        QVERIFY(image != uploaded.end());
        const std::optional<server_client::Image> saved = session.editTags(
            *image, [](tag_editor::TagEditor& editor) { return editor.addTag(QStringLiteral("album"), albumName); });
        QVERIFY2(saved.has_value(), qPrintable(session.tagEditorError()));
    }
}

void TestEndToEndAlbums::init()
{
    m_session = std::make_unique<AppSession>();
    QVERIFY2(m_session->logIn(*m_server, userName), qPrintable(m_session->connection().errorMessage()));
}

void TestEndToEndAlbums::cleanup()
{
    QVERIFY2(m_session->saveHiddenFilters({}), qPrintable(m_session->hiddenFilterError()));
}

void TestEndToEndAlbums::opensAnAlbumInTheOrderItsImagesWereAdded()
{
    QCOMPARE(
        fileNamesOf(m_session->openAlbum(albumName)),
        QStringList({QStringLiteral("b_forest.png"), QStringLiteral("c_lake.png"), QStringLiteral("a_harbour.png")}));
    QVERIFY(m_session->albumError().isEmpty());
}

void TestEndToEndAlbums::listsTheAlbumWithItsFirstImageAsCover()
{
    const std::optional<server_client::Album> album = albumIn(m_session->albums(false), albumTag);

    QVERIFY(album.has_value());
    QCOMPARE(album->imageCount, 3);
    const QList<server_client::Image> images = m_session->openAlbum(albumName);
    QVERIFY(!images.isEmpty());
    QCOMPARE(album->coverImageId, images.first().id);  // the forest, added first
}

// Opened on purpose, an album shows all its images; only the list leaves out an album with hidden ones.
void TestEndToEndAlbums::showsHiddenImagesInAnOpenedAlbum()
{
    QVERIFY2(m_session->saveHiddenFilters({hiddenTag}), qPrintable(m_session->hiddenFilterError()));

    QCOMPARE(m_session->openAlbum(albumName).size(), 3);
    QVERIFY(!albumIn(m_session->albums(false), albumTag).has_value());
    QVERIFY(albumIn(m_session->albums(true), albumTag).has_value());
}

}  // namespace photobook::e2e

QTEST_GUILESS_MAIN(photobook::e2e::TestEndToEndAlbums)

#include "TestEndToEndAlbums.moc"
