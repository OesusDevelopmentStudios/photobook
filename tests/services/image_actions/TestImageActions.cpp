/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QBuffer>
#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "Connection.hpp"
#include "ImageActionsFactory.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/ImageJson.hpp"

namespace pictorio::image_actions
{

namespace
{

using connection::Connection;
using fakes::FakeServer;

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString fileUrl = serverUrl + QStringLiteral("/api/v1/images/7/file");

// A small image in the given format, e.g. "PNG"
QByteArray encodedImage(const char* format, const QSize& size = QSize(4, 3))
{
    QImage image(size, QImage::Format_RGB32);
    image.fill(Qt::darkCyan);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, format);
    return bytes;
}

const QMimeData* clipboard()
{
    return QGuiApplication::clipboard()->mimeData();
}

bool clipboardHasImage()
{
    return clipboard() != nullptr && clipboard()->hasImage();  // an emptied clipboard may have no data at all
}

}  // namespace

// Copying an image to the clipboard: a local file with its URL, a library image once downloaded. Runs on the
// offscreen platform, whose clipboard stays in the process.
class TestImageActions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void copiesALocalImageWithItsFile();
    void keepsAJpegAsItIs();
    void reportsAFileThatIsNotAnImage();
    void reportsAMissingFile();
    void copiesALibraryImageOnceDownloaded();
    void reportsAFailedDownload();

private:
    QString writtenFile(const QString& name, const QByteArray& content) const;

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<ImageActions> m_actions;
};

void TestImageActions::initTestCase()
{
    KLocalizedString::setApplicationDomain("pictorio");
}

// Logged in from the last run; the clipboard empty
void TestImageActions::init()
{
    QGuiApplication::clipboard()->clear();
    m_actions.reset();
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("pictoriorc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/auth/me"),
                     fakes::jsonResponse(200,
                                         {{QStringLiteral("id"), 2},
                                          {QStringLiteral("username"), QStringLiteral("kid")},
                                          {QStringLiteral("role"), QStringLiteral("user")},
                                          {QStringLiteral("restricted_tags"), QJsonArray()}}));
    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    m_connection = std::make_unique<Connection>(
        [server = m_server](const QUrl& url) { return server_client::createServerClient(url, server); },
        std::make_unique<fakes::FakeTokenStore>(keyring),
        m_settings.get());
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
    m_actions = createImageActions(m_connection.get());
}

void TestImageActions::copiesALocalImageWithItsFile()
{
    const QByteArray png = encodedImage("PNG");
    const QString filePath = writtenFile(QStringLiteral("beach.png"), png);
    QSignalSpy copied(m_actions.get(), &ImageActions::copied);

    m_actions->copyLocalImage(filePath);

    QCOMPARE(copied.size(), 1);
    QVERIFY(clipboardHasImage());
    QCOMPARE(qvariant_cast<QImage>(clipboard()->imageData()).size(), QSize(4, 3));
    QCOMPARE(clipboard()->data(QStringLiteral("image/png")), png);
    QCOMPARE(clipboard()->urls(), QList<QUrl>({QUrl::fromLocalFile(filePath)}));  // pasted in Dolphin: the file
}

// Not re-encoded, so nothing is lost
void TestImageActions::keepsAJpegAsItIs()
{
    const QByteArray jpeg = encodedImage("JPEG");
    const QString filePath = writtenFile(QStringLiteral("beach.jpg"), jpeg);

    m_actions->copyLocalImage(filePath);

    QCOMPARE(clipboard()->data(QStringLiteral("image/jpeg")), jpeg);
}

void TestImageActions::reportsAFileThatIsNotAnImage()
{
    const QString filePath = writtenFile(QStringLiteral("notes.png"), "Not a picture at all.");
    QSignalSpy copied(m_actions.get(), &ImageActions::copied);
    QSignalSpy failed(m_actions.get(), &ImageActions::failed);

    m_actions->copyLocalImage(filePath);

    QCOMPARE(failed.size(), 1);
    QVERIFY(!failed.first().first().toString().isEmpty());
    QCOMPARE(copied.size(), 0);
    QVERIFY(!clipboardHasImage());
}

void TestImageActions::reportsAMissingFile()
{
    QSignalSpy failed(m_actions.get(), &ImageActions::failed);

    m_actions->copyLocalImage(m_folder->filePath(QStringLiteral("gone.png")));

    QCOMPARE(failed.size(), 1);
    QVERIFY(!clipboardHasImage());
}

void TestImageActions::copiesALibraryImageOnceDownloaded()
{
    const QByteArray jpeg = encodedImage("JPEG", QSize(6, 2));
    m_server->answer("GET", fileUrl, fakes::response(200, jpeg));
    m_server->holdAnswers = true;
    QSignalSpy copied(m_actions.get(), &ImageActions::copied);

    m_actions->copyLibraryImage(7, QStringLiteral("image/jpeg"));

    QVERIFY(m_actions->isBusy());
    m_actions->copyLibraryImage(7, QStringLiteral("image/jpeg"));  // a second Ctrl+C meanwhile: one download
    QTRY_COMPARE(m_server->requestsTo("GET", fileUrl).size(), 1);
    m_server->releaseAnswers();
    QTRY_COMPARE(copied.size(), 1);
    QVERIFY(!m_actions->isBusy());
    QCOMPARE(qvariant_cast<QImage>(clipboard()->imageData()).size(), QSize(6, 2));
    QCOMPARE(clipboard()->data(QStringLiteral("image/jpeg")), jpeg);
    QVERIFY(clipboard()->urls().isEmpty());  // no file on this computer
}

void TestImageActions::reportsAFailedDownload()
{
    m_server->answer("GET", fileUrl, fakes::problem(404, "not_found"));
    QSignalSpy failed(m_actions.get(), &ImageActions::failed);

    m_actions->copyLibraryImage(7, QStringLiteral("image/jpeg"));

    QTRY_COMPARE(failed.size(), 1);
    QVERIFY(!m_actions->isBusy());
    QVERIFY(!clipboardHasImage());
}

QString TestImageActions::writtenFile(const QString& name, const QByteArray& content) const
{
    const QString filePath = m_folder->filePath(name);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(content);
    }

    return filePath;
}

}  // namespace pictorio::image_actions

QTEST_MAIN(pictorio::image_actions::TestImageActions)

#include "TestImageActions.moc"
