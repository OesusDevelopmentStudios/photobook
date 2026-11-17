/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "ImageActions.hpp"

#include <memory>

#include <QBuffer>
#include <QClipboard>
#include <QFile>
#include <QGuiApplication>
#include <QImageReader>
#include <QMimeData>
#include <QMimeDatabase>
#include <QUrl>

#include <KLocalizedString>

namespace pictorio::image_actions
{

ImageActions::ImageActions(connection::Connection* connection, QObject* parent)
    : QObject(parent)
    , m_connection(connection)
{}

void ImageActions::copyLocalImage(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        Q_EMIT failed(i18nc("@info %1 is the reason", "The image could not be read: %1", file.errorString()));
        return;
    }

    const QString mimeType = QMimeDatabase().mimeTypeForFile(filePath).name();
    if (copyToClipboard(file.readAll(), mimeType, filePath))
    {
        Q_EMIT copied();
    }
}

void ImageActions::copyLibraryImage(int imageId, const QString& mimeType)
{
    server_client::ServerClient* client = m_connection->client();
    if (m_busy || client == nullptr)
    {
        return;
    }

    setBusy(true);
    client->imageFile(imageId).then(this, [this, mimeType](const server_client::Result<QByteArray>& file) {
        setBusy(false);
        if (!file.has_value())
        {
            Q_EMIT failed(i18nc("@info %1 is the reason",
                                "The image could not be downloaded: %1",
                                file.error().detail.isEmpty() ? file.error().title : file.error().detail));
            return;
        }

        if (copyToClipboard(*file, mimeType, {}))
        {
            Q_EMIT copied();
        }
    });
}

bool ImageActions::isBusy() const
{
    return m_busy;
}

bool ImageActions::copyToClipboard(const QByteArray& file, const QString& mimeType, const QString& filePath)
{
    QBuffer buffer;
    buffer.setData(file);
    QImageReader reader(&buffer);
    reader.setAutoTransform(true);  // as the viewer shows it, e.g. a portrait photo upright
    const QImage image = reader.read();
    if (image.isNull())
    {
        Q_EMIT failed(i18nc("@info %1 is the reason", "The image could not be copied: %1", reader.errorString()));
        return false;
    }

    auto data = std::make_unique<QMimeData>();
    data->setImageData(image);
    data->setData(mimeType, file);
    if (!filePath.isEmpty())
    {
        data->setUrls({QUrl::fromLocalFile(filePath)});
    }

    QGuiApplication::clipboard()->setMimeData(data.release());  // the clipboard owns it
    return true;
}

void ImageActions::setBusy(bool busy)
{
    if (busy != m_busy)
    {
        m_busy = busy;
        Q_EMIT busyChanged();
    }
}

}  // namespace pictorio::image_actions
