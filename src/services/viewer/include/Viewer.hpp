/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>

#include <QObject>
#include <QSize>
#include <QString>
#include <QUrl>

#include "ImageLoader.hpp"

namespace pictorio::viewer
{

// What the viewer page shows: the current image, from a file or from the server, or why it could not be
// opened.
class Viewer : public QObject
{
    Q_OBJECT

    struct ShownImage
    {
        QUrl source;
        QSize displaySize;
        bool animated = false;
    };

    Q_PROPERTY(bool hasImage READ hasImage NOTIFY currentImageChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY currentImageChanged)
    Q_PROPERTY(QUrl imageSource READ imageSource NOTIFY currentImageChanged)
    Q_PROPERTY(QSize imageDisplaySize READ imageDisplaySize NOTIFY currentImageChanged)
    Q_PROPERTY(bool imageAnimated READ imageAnimated NOTIFY currentImageChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY currentImageChanged)

public:
    explicit Viewer(std::unique_ptr<image_loader::ImageLoader> imageLoader, QObject* parent = nullptr);
    ~Viewer() override;

    Q_INVOKABLE void openImage(const QString& filePath);
    // An image in the server's library, loaded from fileUrl by the page. The size is the server's, after
    // the EXIF orientation; the MIME type tells whether it may be animated.
    void
    openServerImage(const QUrl& fileUrl, const QString& fileName, const QSize& displaySize, const QString& mimeType);

    bool hasImage() const;
    QString fileName() const;  // also set when opening failed, for the error message
    QUrl imageSource() const;
    QSize imageDisplaySize() const;
    bool imageAnimated() const;
    QString errorMessage() const;  // empty when the image opened

Q_SIGNALS:
    void currentImageChanged();

private:
    std::unique_ptr<image_loader::ImageLoader> m_imageLoader;
    std::optional<ShownImage> m_currentImage;
    QString m_fileName;
    QString m_errorMessage;
};

}  // namespace pictorio::viewer
