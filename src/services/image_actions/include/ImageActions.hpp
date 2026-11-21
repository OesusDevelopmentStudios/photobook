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

namespace photobook::image_actions
{

// Copying is announced with copied(), saving with rotationSaved(); a failure of any with failed().
class ImageActions : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)  // waiting for the server: a download, or a rotation

public:
    ImageActions(connection::Connection* connection,
                 std::unique_ptr<FileManager> fileManager,
                 QObject* parent = nullptr);
    ~ImageActions() override;

    // The image as decoded, its file as it is (e.g. image/jpeg, so a GIF keeps its frames) and its URL, so
    // pasting into a file manager copies the file.
    void copyLocalImage(const QString& filePath);
    // The same without the URL, once the file has been downloaded from the server
    void copyLibraryImage(int imageId, const QString& mimeType);

    void openContainingFolder(const QString& filePath);

    // Whether the rotation of an image of this type can be saved into its file without losing quality
    Q_INVOKABLE bool canSaveRotation(const QString& mimeType) const;
    // Turns the image in the file clockwise by 90, 180 or 270 degrees from how it is shown now
    void saveLocalRotation(const QString& filePath, int clockwiseDegrees);
    // The same for a library image, done by the server: libraryRotationSaved() with the image as it is then
    void saveLibraryRotation(int imageId, int clockwiseDegrees);

    bool isBusy() const;

Q_SIGNALS:
    void busyChanged();
    void copied();
    void rotationSaved();  // after either kind; a library image's comes after libraryRotationSaved()
    void libraryRotationSaved(const server_client::Image& image);
    void failed(const QString& message);

private:
    // False if the bytes are not an image
    bool copyToClipboard(const QByteArray& file, const QString& mimeType, const QString& filePath);
    void setBusy(bool busy);

    connection::Connection* m_connection;
    std::unique_ptr<FileManager> m_fileManager;
    bool m_busy = false;
};

}  // namespace photobook::image_actions
