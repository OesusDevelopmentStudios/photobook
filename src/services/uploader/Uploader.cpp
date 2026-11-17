/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "Uploader.hpp"

#include <algorithm>
#include <optional>

#include <KLocalizedString>
#include <QFileInfo>
#include <QJsonArray>
#include <QtConcurrentRun>

#include "FolderScanner.hpp"
#include "utils/FileHash.hpp"

namespace
{

using pictorio::server_client::ApiError;

// The server's refusal of a file as a sentence for the user
QString describeError(const ApiError& error)
{
    if (error.code == pictorio::server_client::networkErrorCode)
    {
        return i18nc("@info %1 is the reason, e.g. Connection refused", "The server does not answer: %1", error.detail);
    }

    if (error.code == QLatin1String("tag_restricted"))
    {
        QStringList tags;
        for (const QJsonValue& tag : error.problem.value(QStringLiteral("tags")).toArray())
        {
            tags.append(tag.toString());
        }

        return i18nc("@info %1 is a list of tags",
                     "An admin has restricted these tags for your account: %1",
                     tags.join(QStringLiteral(", ")));
    }

    return error.detail.isEmpty()
               ? error.title
               : i18nc("@info %1 is a title, %2 an explanation", "%1. %2", error.title, error.detail);
}

}  // namespace

namespace pictorio::uploader
{

Uploader::Uploader(connection::Connection* connection,
                   tag_editor::UploadTagEditor* tagEditor,
                   std::unique_ptr<Trash> trash,
                   QObject* parent)
    : QAbstractListModel(parent)
    , m_connection(connection)
    , m_tagEditor(tagEditor)
    , m_trash(std::move(trash))
{}

void Uploader::addFiles(const QList<QUrl>& fileUrls)
{
    QStringList filePaths;
    for (const QUrl& fileUrl : fileUrls)
    {
        filePaths.append(fileUrl.toLocalFile());
    }

    appendFiles(filePaths);
}

void Uploader::addFolder(const QUrl& folderUrl, bool includeSubfolders)
{
    if (++m_foldersBeingListed == 1)
    {
        Q_EMIT findingFilesChanged();
    }

    QtConcurrent::run(folder_scanner::findImages, folderUrl.toLocalFile(), includeSubfolders)
        .then(this, [this](const QStringList& imagePaths) {
            appendFiles(imagePaths);
            if (--m_foldersBeingListed == 0)
            {
                Q_EMIT findingFilesChanged();
            }
        });
}

void Uploader::removeFile(int row)
{
    if (m_running || row < 0 || row >= count())
    {
        return;
    }

    beginRemoveRows({}, row, row);
    m_files.removeAt(row);
    endRemoveRows();
    Q_EMIT countChanged();
    Q_EMIT progressChanged();
}

void Uploader::clear()
{
    if (m_running || m_files.isEmpty())
    {
        return;
    }

    beginResetModel();
    m_files.clear();
    endResetModel();
    Q_EMIT countChanged();
    Q_EMIT progressChanged();
}

void Uploader::start()
{
    if (m_running || countIn(FileState::Waiting) == 0)
    {
        return;
    }

    const std::optional<QStringList> tags = m_tagEditor->tagsToUpload();
    if (!tags.has_value())
    {
        return;  // a tag was refused; the tag editor says why
    }

    m_tags = *tags;
    m_cancelled = false;
    m_tagEditor->setBusy(true);  // the same tags for every file
    setRunning(true);
    uploadNext();
}

void Uploader::cancel()
{
    m_cancelled = m_running;
}

int Uploader::count() const
{
    return static_cast<int>(m_files.size());
}

bool Uploader::isRunning() const
{
    return m_running;
}

bool Uploader::isFindingFiles() const
{
    return m_foldersBeingListed > 0;
}

int Uploader::uploadedCount() const
{
    return countIn(FileState::Uploaded);
}

int Uploader::alreadyThereCount() const
{
    return countIn(FileState::AlreadyThere);
}

int Uploader::failedCount() const
{
    return countIn(FileState::Failed);
}

bool Uploader::movesOriginalsToTrash() const
{
    return m_moveOriginalsToTrash;
}

void Uploader::setMoveOriginalsToTrash(bool move)
{
    if (move != m_moveOriginalsToTrash)
    {
        m_moveOriginalsToTrash = move;
        Q_EMIT moveOriginalsToTrashChanged();
    }
}

int Uploader::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant Uploader::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
    {
        return {};
    }

    const UploadFile& file = m_files.at(index.row());
    switch (role)
    {
    case FilePathRole:
        return file.path;
    case FileNameRole:
    case Qt::DisplayRole:
        return QFileInfo(file.path).fileName();
    case FolderRole:
        return QFileInfo(file.path).path();
    case StateRole:
        return QVariant::fromValue(file.state);
    case MessageRole:
        return file.message;
    default:
        return {};
    }
}

QHash<int, QByteArray> Uploader::roleNames() const
{
    return {
        {FilePathRole, "filePath"},
        {FileNameRole, "fileName"},
        {FolderRole, "folder"},
        {StateRole, "state"},
        {MessageRole, "message"},
    };
}

void Uploader::appendFiles(const QStringList& filePaths)
{
    QStringList newPaths;
    for (const QString& filePath : filePaths)
    {
        const bool listed = std::ranges::any_of(m_files, [&](const UploadFile& file) { return file.path == filePath; });
        if (!listed && !newPaths.contains(filePath))
        {
            newPaths.append(filePath);
        }
    }

    if (newPaths.isEmpty())
    {
        return;
    }

    beginInsertRows({}, count(), count() + static_cast<int>(newPaths.size()) - 1);
    for (const QString& filePath : newPaths)
    {
        m_files.append(UploadFile{.path = filePath, .state = FileState::Waiting, .message = {}});
    }

    endInsertRows();
    Q_EMIT countChanged();
}

// The SHA-256 is computed in the background: large files take a while.
void Uploader::uploadNext()
{
    const auto waiting =
        std::ranges::find_if(m_files, [](const UploadFile& file) { return file.state == FileState::Waiting; });
    if (m_cancelled || waiting == m_files.end())
    {
        finish();
        return;
    }

    const int row = static_cast<int>(std::distance(m_files.begin(), waiting));
    setState(row, FileState::Uploading);
    QtConcurrent::run(utils::sha256Of, waiting->path).then(this, [this, row](const std::optional<QString>& sha256) {
        if (!sha256.has_value())
        {
            fail(row, i18nc("@info", "The file cannot be read."));
            return;
        }

        send(row, *sha256);
    });
}

void Uploader::send(int row, const QString& sha256)
{
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        fail(row, i18nc("@info", "Not connected to the server."));
        return;
    }

    const QFileInfo file(m_files.at(row).path);
    server_client::ImageUpload upload;
    upload.filePath = file.absoluteFilePath();
    upload.originalFilename = file.fileName();
    upload.fileModified = file.lastModified();  // the date of the photo, if it has none of its own
    upload.sha256 = sha256;
    upload.tags = m_tags;
    client->uploadImage(upload).then(
        this, [this, row, sha256](const server_client::Result<server_client::Image>& answer) {
            if (answer.has_value())
            {
                setState(row, FileState::Uploaded);
                fileIsOnTheServer(row);
                return;
            }

            // The same file, by its SHA-256: it is on the server.
            const ApiError& error = answer.error();
            const bool sameFile = error.code == QLatin1String("duplicate_image") &&
                                  error.problem.value(QStringLiteral("sha256")).toString() == sha256;
            if (!sameFile)
            {
                fail(row, describeError(error));
                return;
            }

            // Without the image's id (the user may not see it) there is nothing to add the tags to.
            const QJsonValue imageId = error.problem.value(QStringLiteral("existing_image_id"));
            if (imageId.isDouble() && !m_tags.isEmpty())
            {
                addTagsToDuplicate(row, imageId.toInt());
                return;
            }

            setState(row, FileState::AlreadyThere);
            fileIsOnTheServer(row);
        });
}

// So the upload's tags apply to every file of it, new or not
void Uploader::addTagsToDuplicate(int row, int imageId)
{
    m_connection->client()
        ->changeImageTags(imageId, server_client::TagChanges{m_tags, {}})
        .then(this, [this, row](const server_client::Result<server_client::Image>& answer) {
            setState(row,
                     FileState::AlreadyThere,
                     answer.has_value() ? QString()
                                        : i18nc("@info %1 is the reason",
                                                "Its tags could not be added: %1",
                                                describeError(answer.error())));
            fileIsOnTheServer(row);
        });
}

// The only place where an original goes: the server has a file with the same SHA-256.
void Uploader::fileIsOnTheServer(int row)
{
    if (!m_moveOriginalsToTrash)
    {
        uploadNext();
        return;
    }

    m_trash->moveToTrash(m_files.at(row).path).then(this, [this, row](const std::expected<void, QString>& trashed) {
        if (!trashed.has_value())
        {
            addMessage(row, i18nc("@info %1 is the reason", "It could not be moved to the trash: %1", trashed.error()));
        }

        uploadNext();
    });
}

// Keeps the file's state; e.g. both its tags and the trash may have failed.
void Uploader::addMessage(int row, const QString& message)
{
    const UploadFile& file = m_files.at(row);
    setState(row, file.state, file.message.isEmpty() ? message : file.message + QLatin1Char('\n') + message);
}

void Uploader::fail(int row, const QString& message)
{
    setState(row, FileState::Failed, message);
    uploadNext();
}

void Uploader::finish()
{
    m_cancelled = false;
    m_tagEditor->setBusy(false);
    setRunning(false);
    Q_EMIT finished();
}

void Uploader::setState(int row, FileState state, const QString& message)
{
    m_files[row].state = state;
    m_files[row].message = message;
    Q_EMIT dataChanged(index(row), index(row), {StateRole, MessageRole});
    Q_EMIT progressChanged();
}

int Uploader::countIn(FileState state) const
{
    return static_cast<int>(
        std::ranges::count_if(m_files, [state](const UploadFile& file) { return file.state == state; }));
}

void Uploader::setRunning(bool running)
{
    if (running != m_running)
    {
        m_running = running;
        Q_EMIT runningChanged();
    }
}

}  // namespace pictorio::uploader
