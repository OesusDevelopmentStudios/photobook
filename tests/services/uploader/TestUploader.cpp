/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "TagEditorFactory.hpp"
#include "Uploader.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/FakeTrash.hpp"
#include "fakes/ImageJson.hpp"

namespace photobook::uploader
{

namespace
{

using connection::Connection;
using fakes::FakeServer;
using server_client::HttpRequest;
using FileState = Uploader::FileState;

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString imagesUrl = serverUrl + QStringLiteral("/api/v1/images");

QJsonObject metadataOf(const HttpRequest& request)
{
    return QJsonDocument::fromJson(request.form->fields.at(0).second).object();
}

server_client::HttpResponse duplicate(const QString& sha256, const QJsonValue& existingImageId)
{
    QJsonObject problem{{QStringLiteral("type"), QStringLiteral("about:blank")},
                        {QStringLiteral("title"), QStringLiteral("Image already exists")},
                        {QStringLiteral("status"), 409},
                        {QStringLiteral("code"), QStringLiteral("duplicate_image")},
                        {QStringLiteral("sha256"), sha256}};
    if (!existingImageId.isUndefined())
    {
        problem.insert(QStringLiteral("existing_image_id"), existingImageId);
    }

    return fakes::jsonResponse(409, problem);
}

// The server's answer, by the file's name: new, not an image, a duplicate it names, a duplicate the user may
// not see, or a duplicate of another file.
server_client::HttpResponse answerFor(const HttpRequest& request)
{
    const QString fileName = request.form->fileName;
    const QString sha256 = metadataOf(request).value(QStringLiteral("client_sha256")).toString();
    if (fileName == QLatin1String("notes.png"))
    {
        return fakes::problem(415, "unsupported_media_type");
    }

    if (fileName == QLatin1String("copy.jpg"))
    {
        return duplicate(sha256, 5);
    }

    if (fileName == QLatin1String("hidden_copy.jpg"))
    {
        return duplicate(sha256, QJsonValue::Undefined);
    }

    if (fileName == QLatin1String("other_copy.jpg"))
    {
        return duplicate(QString(64, QLatin1Char('f')), 6);
    }

    return fakes::jsonResponse(201, fakes::imageJson(1));
}

}  // namespace

// Sending files to the server. The user "restrictedUser" is restricted from rating:private.
class TestUploader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void listsTheFilesToUpload();
    void countsTheFilesStillWaiting();
    void countsTheFilesOfARun();
    void addsAFolderWithOrWithoutSubfolders();
    void sendsOneFileAtATimeWithTheTags();
    void tellsHowEachFileWent();
    void addsTheTagsToADuplicate();
    void reportsADuplicateItCannotSee();
    void failsADuplicateOfAnotherFile();
    void stopsAfterTheCurrentFileWhenCancelled();
    void sendsNothingWhenATagIsRefused();
    void keepsTheTagsWhileSending();
    void keepsTheListWhileSending();
    void keepsTheOriginalsByDefault();
    void trashesOnlyWhatTheServerHas();
    void trashesOnlyAfterTheServerAnswers();
    void keepsTheStateWhenTheTrashFails();

private:
    QUrl fileNamed(const QString& relativePath);
    QString pathOf(const QString& relativePath) const;
    QList<FileState> states() const;
    void uploadAndWait();

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<tag_editor::UploadTagEditor> m_tagEditor;
    std::shared_ptr<fakes::TrashCan> m_trashCan;
    std::unique_ptr<Uploader> m_uploader;
};

void TestUploader::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
}

// Logged in from the last run; the upload's tag is general:beach.
void TestUploader::init()
{
    m_uploader.reset();
    m_tagEditor.reset();
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("photobookrc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    m_server->answer(
        "GET",
        serverUrl + QStringLiteral("/api/v1/auth/me"),
        fakes::jsonResponse(200,
                            {{QStringLiteral("id"), 2},
                             {QStringLiteral("username"), QStringLiteral("restrictedUser")},
                             {QStringLiteral("role"), QStringLiteral("user")},
                             {QStringLiteral("restricted_tags"), QJsonArray{QStringLiteral("rating:private")}}}));
    m_server->answerWith("POST", imagesUrl, answerFor);
    m_server->answer("PATCH", imagesUrl + QStringLiteral("/5"), fakes::jsonResponse(200, fakes::imageJson(5)));

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };

    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
    m_tagEditor = tag_editor::createUploadTagEditor(m_connection.get());
    QVERIFY(m_tagEditor->addTag(QStringLiteral("general"), QStringLiteral("beach")));
    m_trashCan = std::make_shared<fakes::TrashCan>();
    m_uploader = std::make_unique<Uploader>(
        m_connection.get(), m_tagEditor.get(), std::make_unique<fakes::FakeTrash>(m_trashCan));
}

// A file of the test folder, created with its own content
QUrl TestUploader::fileNamed(const QString& relativePath)
{
    const QString path = m_folder->filePath(relativePath);
    QDir().mkpath(QFileInfo(path).path());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        qFatal("Could not create test file %s", qPrintable(path));
    }

    file.write(relativePath.toUtf8());
    return QUrl::fromLocalFile(path);
}

QString TestUploader::pathOf(const QString& relativePath) const
{
    return m_folder->filePath(relativePath);
}

QList<FileState> TestUploader::states() const
{
    QList<FileState> states;
    for (int row = 0; row < m_uploader->rowCount(); ++row)
    {
        states.append(m_uploader->data(m_uploader->index(row), Uploader::StateRole).value<FileState>());
    }

    return states;
}

void TestUploader::uploadAndWait()
{
    QSignalSpy finished(m_uploader.get(), &Uploader::finished);
    m_uploader->start();
    QVERIFY(finished.wait() || finished.count() == 1);
}

void TestUploader::listsTheFilesToUpload()
{
    const QUrl beach = fileNamed(QStringLiteral("holiday/beach.jpg"));

    m_uploader->addFiles({beach, fileNamed(QStringLiteral("sunset.jpg")), beach});

    QCOMPARE(m_uploader->count(), 2);  // each file once
    const QModelIndex first = m_uploader->index(0);
    QCOMPARE(first.data(Uploader::FileNameRole).toString(), QStringLiteral("beach.jpg"));
    QCOMPARE(first.data(Uploader::FolderRole).toString(), m_folder->filePath(QStringLiteral("holiday")));
    QCOMPARE(first.data(Uploader::StateRole).value<FileState>(), FileState::Waiting);
}

void TestUploader::countsTheFilesStillWaiting()
{
    QSignalSpy progress(m_uploader.get(), &Uploader::progressChanged);

    m_uploader->addFiles({fileNamed(QStringLiteral("notes.png")), fileNamed(QStringLiteral("beach.jpg"))});

    QCOMPARE(m_uploader->waitingCount(), 2);
    QVERIFY(!progress.isEmpty());

    uploadAndWait();
    m_uploader->addFiles({fileNamed(QStringLiteral("sunset.jpg"))});

    QCOMPARE(m_uploader->waitingCount(), 1);  // the failed one is not sent again
}

// For the progress of an upload: an earlier run's files do not count.
void TestUploader::countsTheFilesOfARun()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("notes.png"))});
    uploadAndWait();
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg")), fileNamed(QStringLiteral("sunset.jpg"))});
    m_server->holdAnswers = true;

    m_uploader->start();

    QCOMPARE(m_uploader->filesInRun(), 2);
    QCOMPARE(m_uploader->filesDoneInRun(), 0);
    QTRY_COMPARE(m_server->requestsTo("POST", imagesUrl).size(), 2);  // notes.png, then beach.jpg
    m_server->releaseAnswers();
    QTRY_COMPARE(m_uploader->filesDoneInRun(), 1);
    QTRY_COMPARE(m_server->requestsTo("POST", imagesUrl).size(), 3);
    m_server->releaseAnswers();
    QTRY_VERIFY(!m_uploader->isRunning());
    QCOMPARE(m_uploader->filesDoneInRun(), 2);
}

void TestUploader::addsAFolderWithOrWithoutSubfolders()
{
    fileNamed(QStringLiteral("holiday/beach.jpg"));
    fileNamed(QStringLiteral("holiday/day2/sunset.jpg"));
    const QUrl holiday = QUrl::fromLocalFile(m_folder->filePath(QStringLiteral("holiday")));

    m_uploader->addFolder(holiday, false);
    QVERIFY(m_uploader->isFindingFiles());
    QTRY_VERIFY(!m_uploader->isFindingFiles());
    QCOMPARE(m_uploader->count(), 1);

    m_uploader->addFolder(holiday, true);
    QTRY_COMPARE(m_uploader->count(), 2);  // the subfolder's image; beach.jpg was listed already
}

void TestUploader::sendsOneFileAtATimeWithTheTags()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg")), fileNamed(QStringLiteral("sunset.jpg"))});
    m_server->holdAnswers = true;

    m_uploader->start();

    QTRY_COMPARE(m_server->requestsTo("POST", imagesUrl).size(), 1);
    QTest::qWait(50);
    QCOMPARE(m_server->requestsTo("POST", imagesUrl).size(), 1);  // the next waits for this answer
    const QJsonObject metadata = metadataOf(m_server->requestsTo("POST", imagesUrl).first());
    QCOMPARE(metadata.value(QStringLiteral("original_filename")).toString(), QStringLiteral("beach.jpg"));
    QCOMPARE(metadata.value(QStringLiteral("tags")).toArray(), QJsonArray{QStringLiteral("general:beach")});
    QCOMPARE(metadata.value(QStringLiteral("client_sha256")).toString().size(), 64);
    QVERIFY(metadata.contains(QStringLiteral("file_mtime")));

    m_server->releaseAnswers();
    QTRY_COMPARE(m_server->requestsTo("POST", imagesUrl).size(), 2);
    m_server->releaseAnswers();
    QTRY_VERIFY(!m_uploader->isRunning());
}

// A failure does not stop the others.
void TestUploader::tellsHowEachFileWent()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("notes.png")), fileNamed(QStringLiteral("beach.jpg"))});

    uploadAndWait();

    QCOMPARE(states(), QList<FileState>({FileState::Failed, FileState::Uploaded}));
    QVERIFY(!m_uploader->index(0).data(Uploader::MessageRole).toString().isEmpty());
    QCOMPARE(m_uploader->uploadedCount(), 1);
    QCOMPARE(m_uploader->failedCount(), 1);
}

// So the upload's tags apply to every file of it, new or not
void TestUploader::addsTheTagsToADuplicate()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("copy.jpg"))});

    uploadAndWait();

    QCOMPARE(states(), QList<FileState>{FileState::AlreadyThere});
    const QList<HttpRequest> tagChanges = m_server->requestsTo("PATCH", imagesUrl + QStringLiteral("/5"));
    QCOMPARE(tagChanges.size(), 1);
    QCOMPARE(QJsonDocument::fromJson(tagChanges.first().body).object().value(QStringLiteral("add_tags")).toArray(),
             QJsonArray{QStringLiteral("general:beach")});
}

// The server does not say which image it is, as the user may not see it.
void TestUploader::reportsADuplicateItCannotSee()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("hidden_copy.jpg"))});

    uploadAndWait();

    QCOMPARE(states(), QList<FileState>{FileState::AlreadyThere});
    QCOMPARE(m_uploader->alreadyThereCount(), 1);
    QVERIFY(m_server->requestsTo("PATCH", imagesUrl + QStringLiteral("/6")).isEmpty());
}

// Not the same SHA-256: this file is not on the server.
void TestUploader::failsADuplicateOfAnotherFile()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("other_copy.jpg"))});

    uploadAndWait();

    QCOMPARE(states(), QList<FileState>{FileState::Failed});
    QVERIFY(m_server->requestsTo("PATCH", imagesUrl + QStringLiteral("/6")).isEmpty());
}

void TestUploader::stopsAfterTheCurrentFileWhenCancelled()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg")), fileNamed(QStringLiteral("sunset.jpg"))});
    m_server->holdAnswers = true;
    QSignalSpy finished(m_uploader.get(), &Uploader::finished);
    m_uploader->start();
    QTRY_COMPARE(m_server->requestsTo("POST", imagesUrl).size(), 1);

    m_uploader->cancel();
    m_server->releaseAnswers();

    QTRY_COMPARE(finished.count(), 1);
    QCOMPARE(states(), QList<FileState>({FileState::Uploaded, FileState::Waiting}));
    QCOMPARE(m_server->requestsTo("POST", imagesUrl).size(), 1);
}

void TestUploader::sendsNothingWhenATagIsRefused()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg"))});
    m_tagEditor->setTypedName(QStringLiteral("general"), QStringLiteral("sand,sea"));

    m_uploader->start();

    QVERIFY(!m_uploader->isRunning());
    QVERIFY(!m_tagEditor->errorMessage().isEmpty());
    QTest::qWait(50);
    QVERIFY(m_server->requestsTo("POST", imagesUrl).isEmpty());
}

// The same tags for every file of the upload
void TestUploader::keepsTheTagsWhileSending()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg"))});
    m_server->holdAnswers = true;

    m_uploader->start();
    QVERIFY(m_tagEditor->isBusy());

    QTRY_COMPARE(m_server->requestsTo("POST", imagesUrl).size(), 1);
    m_server->releaseAnswers();
    QTRY_VERIFY(!m_uploader->isRunning());
    QVERIFY(!m_tagEditor->isBusy());
}

void TestUploader::keepsTheListWhileSending()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg"))});
    m_server->holdAnswers = true;
    m_uploader->start();

    m_uploader->removeFile(0);
    m_uploader->clear();

    QCOMPARE(m_uploader->count(), 1);
    QTRY_COMPARE(m_server->requestsTo("POST", imagesUrl).size(), 1);
    m_server->releaseAnswers();
    QTRY_VERIFY(!m_uploader->isRunning());
    m_uploader->removeFile(0);
    QCOMPARE(m_uploader->count(), 0);
}

void TestUploader::keepsTheOriginalsByDefault()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg")), fileNamed(QStringLiteral("copy.jpg"))});

    uploadAndWait();

    QCOMPARE(states(), QList<FileState>({FileState::Uploaded, FileState::AlreadyThere}));
    QVERIFY(m_trashCan->files.isEmpty());
}

// New, or the same SHA-256 on the server: only those originals may go.
void TestUploader::trashesOnlyWhatTheServerHas()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg")),
                          fileNamed(QStringLiteral("copy.jpg")),
                          fileNamed(QStringLiteral("hidden_copy.jpg")),
                          fileNamed(QStringLiteral("notes.png")),
                          fileNamed(QStringLiteral("other_copy.jpg"))});
    m_uploader->setMoveOriginalsToTrash(true);

    uploadAndWait();

    QCOMPARE(m_trashCan->files,
             QStringList({pathOf(QStringLiteral("beach.jpg")),
                          pathOf(QStringLiteral("copy.jpg")),
                          pathOf(QStringLiteral("hidden_copy.jpg"))}));
}

void TestUploader::trashesOnlyAfterTheServerAnswers()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg"))});
    m_uploader->setMoveOriginalsToTrash(true);
    m_server->holdAnswers = true;

    m_uploader->start();
    QTRY_COMPARE(m_server->requestsTo("POST", imagesUrl).size(), 1);
    QTest::qWait(50);
    QVERIFY(m_trashCan->files.isEmpty());

    m_server->releaseAnswers();
    QTRY_COMPARE(m_trashCan->files, QStringList{pathOf(QStringLiteral("beach.jpg"))});
}

// The server has the files all the same; the user learns why the originals are still there.
void TestUploader::keepsTheStateWhenTheTrashFails()
{
    m_uploader->addFiles({fileNamed(QStringLiteral("beach.jpg")), fileNamed(QStringLiteral("copy.jpg"))});
    m_uploader->setMoveOriginalsToTrash(true);
    m_trashCan->works = false;

    uploadAndWait();

    QCOMPARE(states(), QList<FileState>({FileState::Uploaded, FileState::AlreadyThere}));
    QVERIFY(m_uploader->index(0).data(Uploader::MessageRole).toString().contains(QStringLiteral("No trash here")));
    QVERIFY(m_uploader->index(1).data(Uploader::MessageRole).toString().contains(QStringLiteral("No trash here")));
}

}  // namespace photobook::uploader

QTEST_GUILESS_MAIN(photobook::uploader::TestUploader)

#include "TestUploader.moc"
