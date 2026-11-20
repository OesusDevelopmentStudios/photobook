/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QByteArray>
#include <QObject>
#include <QString>

#include "Connection.hpp"
#include "FileManager.hpp"

namespace pictorio::image_actions
{

// What can be done with an image besides looking at it: copying it to the clipboard, so it can be pasted into
// other apps, showing its file in the file manager, and saving a rotation into its file. Copying is announced
// with copied(), saving with rotationSaved(); a failure of any with failed(), e.g. for a passive notification.
class ImageActions : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)  // a library image is being downloaded

public:
    // The connection is not owned and must outlive the actions.
    ImageActions(connection::Connection* connection,
                 std::unique_ptr<FileManager> fileManager,
                 QObject* parent = nullptr);
    ~ImageActions() override;

    // The image as decoded, its file as it is (e.g. image/jpeg, so a GIF keeps its frames) and its URL, so
    // pasting into a file manager copies the file.
    void copyLocalImage(const QString& filePath);
    // The same without the URL, once the file has been downloaded from the server
    void copyLibraryImage(int imageId, const QString& mimeType);

    // Its folder in the file manager, with the file selected
    void openContainingFolder(const QString& filePath);

    // Whether the rotation of an image of this type can be saved into its file without losing quality
    Q_INVOKABLE bool canSaveRotation(const QString& mimeType) const;
    // Turns the image in the file clockwise by 90, 180 or 270 degrees from how it is shown now
    void saveLocalRotation(const QString& filePath, int clockwiseDegrees);

    bool isBusy() const;

Q_SIGNALS:
    void busyChanged();
    void copied();
    void rotationSaved();
    void failed(const QString& message);

private:
    // False if the bytes are not an image
    bool copyToClipboard(const QByteArray& file, const QString& mimeType, const QString& filePath);
    void setBusy(bool busy);

    connection::Connection* m_connection;
    std::unique_ptr<FileManager> m_fileManager;
    bool m_busy = false;
};

}  // namespace pictorio::image_actions
