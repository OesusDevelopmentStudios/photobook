/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QUrl>

#include "ApiTypes.hpp"
#include "Connection.hpp"
#include "Trash.hpp"
#include "UploadTagEditor.hpp"

namespace pictorio::uploader
{

// The files of an upload and their way to the server: one file at a time, with the tags of the upload. A file
// is on the server after 201 Created, or after a 409 duplicate with the same SHA-256; for a duplicate the
// upload's tags are added to the image the server has. Only then, and only if asked, does the original go to the
// trash. Any other answer leaves the file as it is, with the reason, and the next file goes on.
class Uploader : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool findingFiles READ isFindingFiles NOTIFY findingFilesChanged)  // a folder is being listed
    Q_PROPERTY(int waitingCount READ waitingCount NOTIFY progressChanged)         // what start() would send
    Q_PROPERTY(int uploadedCount READ uploadedCount NOTIFY progressChanged)
    Q_PROPERTY(int alreadyThereCount READ alreadyThereCount NOTIFY progressChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY progressChanged)
    // The files of the last start(), and how many of them are done (also those that failed)
    Q_PROPERTY(int filesInRun READ filesInRun NOTIFY progressChanged)
    Q_PROPERTY(int filesDoneInRun READ filesDoneInRun NOTIFY progressChanged)
    Q_PROPERTY(bool moveOriginalsToTrash READ movesOriginalsToTrash WRITE setMoveOriginalsToTrash NOTIFY
                   moveOriginalsToTrashChanged)  // off by default

public:
    enum class FileState
    {
        Waiting,
        Uploading,
        Uploaded,
        AlreadyThere,  // the server had the same file
        Failed,
    };
    Q_ENUM(FileState)

    enum Role
    {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole,
        FolderRole,
        StateRole,
        MessageRole,  // why it failed, or what else happened
    };

    Uploader(connection::Connection* connection,
             tag_editor::UploadTagEditor* tagEditor,
             std::unique_ptr<Trash> trash,
             QObject* parent = nullptr);

    // Files chosen in a file dialog; ones on the list already are left out.
    Q_INVOKABLE void addFiles(const QList<QUrl>& fileUrls);
    // The folder's images, with includeSubfolders also those below it; listed in the background.
    Q_INVOKABLE void addFolder(const QUrl& folderUrl, bool includeSubfolders);
    Q_INVOKABLE void removeFile(int row);  // not while running
    Q_INVOKABLE void clear();              // not while running

    // Sends the files still waiting, with the tag editor's tags; nothing if one of those is refused.
    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();  // after the current file

    int count() const;
    bool isRunning() const;
    bool isFindingFiles() const;
    int waitingCount() const;
    int uploadedCount() const;
    int alreadyThereCount() const;
    int failedCount() const;
    int filesInRun() const;
    int filesDoneInRun() const;
    bool movesOriginalsToTrash() const;
    void setMoveOriginalsToTrash(bool move);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void countChanged();
    void runningChanged();
    void findingFilesChanged();
    void progressChanged();
    void moveOriginalsToTrashChanged();
    void finished();  // the files are done, or the rest was cancelled

private:
    struct UploadFile
    {
        QString path;
        FileState state = FileState::Waiting;
        QString message;
    };

    void appendFiles(const QStringList& filePaths);
    void uploadNext();
    void send(int row, const QString& sha256);
    void addTagsToDuplicate(int row, int imageId);
    void fileIsOnTheServer(int row);
    void addMessage(int row, const QString& message);
    void fail(int row, const QString& message);
    void finish();
    void setState(int row, FileState state, const QString& message = {});
    int countIn(FileState state) const;
    void setRunning(bool running);

    connection::Connection* m_connection;
    tag_editor::UploadTagEditor* m_tagEditor;
    std::unique_ptr<Trash> m_trash;
    QList<UploadFile> m_files;
    QStringList m_tags;  // of the upload that is running
    bool m_running = false;
    bool m_cancelled = false;
    int m_filesInRun = 0;
    bool m_moveOriginalsToTrash = false;
    int m_foldersBeingListed = 0;
};

}  // namespace pictorio::uploader
