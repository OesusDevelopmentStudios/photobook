/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>
#include <optional>

#include <KLocalizedString>
#include <QDir>
#include <QImage>
#include <QImageIOHandler>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTemporaryDir>
#include <QTest>

#include "ImageActions.hpp"
#include "NetworkAccess.hpp"
#include "e2e/AppSession.hpp"
#include "e2e/EndToEndServer.hpp"
#include "fakes/FakeFileManager.hpp"

namespace photobook::e2e
{

namespace
{

using FileState = uploader::Uploader::FileState;

// Wider than high, so a quarter turn shows in the size
const QSize landscape(32, 16);

// The pixels as stored in the file, before its EXIF orientation turns them
QImage storedPixels(const QString& filePath)
{
    QImageReader reader(filePath);
    reader.setAutoTransform(false);
    return reader.read();
}

}  // namespace

class TestEndToEndRotation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void savesAQuarterTurnIntoTheFile();
    void staysTheSameImage();
    void showsTheTurnedThumbnailWhereTheOldOneIsCached();

private:
    // A new landscape JPEG in the session's folder, e.g. "beach" for beach.jpg
    QString newPhoto(const QString& stem);
    // The file uploaded with a tag of its own, general:<stem>; the image as the server has it then
    std::optional<server_client::Image> upload(const QString& filePath, const QString& stem);
    // As in the viewer: turned, then Save Rotation. The image as the server saved it.
    std::optional<server_client::Image> saveRotation(const server_client::Image& image, int clockwiseDegrees);
    // As the library shows it: loaded from the address of the image's revision, through the app's network access
    // with its disk cache. A null image if it did not load.
    QImage thumbnail(const server_client::Image& image);

    std::optional<EndToEndServer> m_server;
    std::unique_ptr<AppSession> m_session;
    std::unique_ptr<image_actions::ImageActions> m_imageActions;
    QString m_imageActionsError;
    std::unique_ptr<QTemporaryDir> m_cacheFolder;
    std::shared_ptr<network_access::ServerCredentials> m_credentials;
    std::unique_ptr<QNetworkAccessManager> m_networkAccess;
};

void TestEndToEndRotation::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
    m_server = EndToEndServer::fromEnvironment();
    if (!m_server.has_value())
    {
        QSKIP("No server: run tests/e2e/run_end_to_end_tests.py");
    }
}

void TestEndToEndRotation::init()
{
    m_session = std::make_unique<AppSession>();
    QVERIFY2(m_session->logIn(*m_server, userName), qPrintable(m_session->connection().errorMessage()));

    m_imageActions = std::make_unique<image_actions::ImageActions>(
        &m_session->connection(),
        std::make_unique<fakes::FakeFileManager>(std::make_shared<fakes::FileManagerWindows>()));
    m_imageActionsError.clear();
    connect(m_imageActions.get(), &image_actions::ImageActions::failed, this, [this](const QString& message) {
        m_imageActionsError = message;
    });

    m_cacheFolder = std::make_unique<QTemporaryDir>();
    m_credentials = std::make_shared<network_access::ServerCredentials>();
    m_credentials->setLogin(m_server->url, m_session->connection().token());
    m_networkAccess.reset(network_access::createNetworkAccessManager(m_credentials, m_cacheFolder->path(), nullptr));
}

void TestEndToEndRotation::savesAQuarterTurnIntoTheFile()
{
    const std::optional<server_client::Image> photo = upload(newPhoto(QStringLiteral("mesa")), QStringLiteral("mesa"));
    QVERIFY(photo.has_value());
    QCOMPARE(photo->revision, 0);
    QCOMPARE(QSize(photo->width, photo->height), landscape);
    const QStringList stored = storedFiles(*m_server, QStringLiteral("mesa"));
    QCOMPARE(stored.size(), 1);
    const QString storedPath = QDir(m_server->libraryDir).filePath(stored.first());
    const QImage pixelsBefore = storedPixels(storedPath);
    QVERIFY(!pixelsBefore.isNull());

    const std::optional<server_client::Image> turned = saveRotation(*photo, 90);

    QVERIFY2(turned.has_value(), qPrintable(m_imageActionsError));
    QCOMPARE(turned->revision, 1);
    QCOMPARE(QSize(turned->width, turned->height), landscape.transposed());
    const QList<server_client::Image> found = m_session->search(QStringLiteral("general:mesa"));
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first().revision, 1);
    QCOMPARE(QSize(found.first().width, found.first().height), landscape.transposed());

    QCOMPARE(storedFiles(*m_server, QStringLiteral("mesa")), stored);  // the same file, changed in place
    QImageReader reader(storedPath);
    QCOMPARE(reader.transformation(), QImageIOHandler::TransformationRotate90);  // EXIF orientation 6
    QCOMPARE(storedPixels(storedPath), pixelsBefore);                            // not encoded again
}

void TestEndToEndRotation::staysTheSameImage()
{
    const QString filePath = newPhoto(QStringLiteral("gorge"));
    const std::optional<server_client::Image> photo = upload(filePath, QStringLiteral("gorge"));
    QVERIFY(photo.has_value());
    const std::optional<server_client::Image> turned = saveRotation(*photo, 270);
    QVERIFY2(turned.has_value(), qPrintable(m_imageActionsError));

    QCOMPARE(turned->id, photo->id);
    QCOMPARE(turned->sha256, photo->sha256);  // of the file as uploaded, though the file changed

    // The original again: the server knows it, as the same image
    const auto [state, message] = m_session->upload(filePath, {{QStringLiteral("general"), QStringLiteral("ravine")}});
    QVERIFY2(state == FileState::AlreadyThere, qPrintable(message));
    const QList<server_client::Image> found = m_session->search(QStringLiteral("general:ravine"));
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first().id, photo->id);
    QCOMPARE(found.first().revision, 1);
    QCOMPARE(storedFiles(*m_server, QStringLiteral("gorge")).size(), 1);
}

void TestEndToEndRotation::showsTheTurnedThumbnailWhereTheOldOneIsCached()
{
    const std::optional<server_client::Image> photo =
        upload(newPhoto(QStringLiteral("canyon")), QStringLiteral("canyon"));
    QVERIFY(photo.has_value());
    const QImage before = thumbnail(*photo);
    QVERIFY(!before.isNull());
    QVERIFY(before.width() > before.height());

    const std::optional<server_client::Image> turned = saveRotation(*photo, 90);
    QVERIFY2(turned.has_value(), qPrintable(m_imageActionsError));

    const QImage after = thumbnail(*turned);
    QVERIFY(!after.isNull());
    QCOMPARE(after.size(), before.size().transposed());
}

QString TestEndToEndRotation::newPhoto(const QString& stem)
{
    return m_session->newSampleImage(stem, QStringLiteral("jpg"), landscape);
}

std::optional<server_client::Image> TestEndToEndRotation::upload(const QString& filePath, const QString& stem)
{
    const auto [state, message] = m_session->upload(filePath, {{QStringLiteral("general"), stem}});
    if (state != FileState::Uploaded)
    {
        qWarning() << "Upload failed:" << message;
        return std::nullopt;
    }

    const QList<server_client::Image> found = m_session->search(QStringLiteral("general:") + stem);
    if (found.size() != 1)
    {
        qWarning() << "Search failed:" << m_session->libraryError();
        return std::nullopt;
    }

    return found.first();
}

std::optional<server_client::Image> TestEndToEndRotation::saveRotation(const server_client::Image& image,
                                                                       int clockwiseDegrees)
{
    std::optional<server_client::Image> saved;
    const QMetaObject::Connection onSaved = connect(m_imageActions.get(),
                                                    &image_actions::ImageActions::libraryRotationSaved,
                                                    this,
                                                    [&saved](const server_client::Image& answer) { saved = answer; });
    m_imageActions->saveLibraryRotation(image.id, clockwiseDegrees);
    if (!QTest::qWaitFor([this]() { return !m_imageActions->isBusy(); }, answerTimeout))
    {
        m_imageActionsError = QStringLiteral("No answer from the server");
    }

    disconnect(onSaved);
    return saved;
}

QImage TestEndToEndRotation::thumbnail(const server_client::Image& image)
{
    const QUrl url =
        m_session->connection().client()->thumbnailUrl(image.id, server_client::ThumbnailSize::Small, image.revision);
    const std::unique_ptr<QNetworkReply> reply(m_networkAccess->get(QNetworkRequest(url)));
    if (!QTest::qWaitFor([&reply]() { return reply->isFinished(); }, answerTimeout) ||
        reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "Thumbnail failed:" << reply->errorString();
        return {};
    }

    return QImage::fromData(reply->readAll());
}

}  // namespace photobook::e2e

QTEST_GUILESS_MAIN(photobook::e2e::TestEndToEndRotation)

#include "TestEndToEndRotation.moc"
