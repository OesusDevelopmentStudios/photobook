/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

#include "Connection.hpp"

namespace pictorio::image_actions
{

// What can be done with an image besides looking at it: copying it to the clipboard, so it can be pasted into
// other apps. The result is announced with copied() or failed(), e.g. for a passive notification.
class ImageActions : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)  // a library image is being downloaded

public:
    // The connection is not owned and must outlive the actions.
    explicit ImageActions(connection::Connection* connection, QObject* parent = nullptr);

    // The image as decoded, its file as it is (e.g. image/jpeg, so a GIF keeps its frames) and its URL, so
    // pasting into a file manager copies the file.
    void copyLocalImage(const QString& filePath);
    // The same without the URL, once the file has been downloaded from the server
    void copyLibraryImage(int imageId, const QString& mimeType);

    bool isBusy() const;

Q_SIGNALS:
    void busyChanged();
    void copied();
    void failed(const QString& message);

private:
    // False if the bytes are not an image
    bool copyToClipboard(const QByteArray& file, const QString& mimeType, const QString& filePath);
    void setBusy(bool busy);

    connection::Connection* m_connection;
    bool m_busy = false;
};

}  // namespace pictorio::image_actions
