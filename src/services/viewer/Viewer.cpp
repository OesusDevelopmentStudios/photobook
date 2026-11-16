/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "Viewer.hpp"

#include <QFileInfo>

namespace
{

// Formats whose files may hold an animation. Only the file itself can tell; a still one shows as well.
bool mayBeAnimated(const QString& mimeType)
{
    return mimeType == QLatin1String("image/gif") || mimeType == QLatin1String("image/webp");
}

}  // namespace

namespace pictorio::viewer
{

Viewer::Viewer(std::unique_ptr<image_loader::ImageLoader> imageLoader, QObject* parent)
    : QObject(parent)
    , m_imageLoader(std::move(imageLoader))
{}

Viewer::~Viewer() = default;

void Viewer::openImage(const QString& filePath)
{
    const auto result = m_imageLoader->loadInfo(filePath);

    m_fileName = QFileInfo(filePath).fileName();
    if (result.has_value())
    {
        m_currentImage = ShownImage{QUrl::fromLocalFile(result->filePath), result->displaySize, result->animated};
        m_errorMessage.clear();
    }
    else
    {
        m_currentImage.reset();
        m_errorMessage = image_loader::describeError(result.error());
    }

    Q_EMIT currentImageChanged();
}

void Viewer::openServerImage(const QUrl& fileUrl,
                             const QString& fileName,
                             const QSize& displaySize,
                             const QString& mimeType)
{
    m_fileName = fileName;
    m_currentImage = ShownImage{fileUrl, displaySize, mayBeAnimated(mimeType)};
    m_errorMessage.clear();
    Q_EMIT currentImageChanged();
}

bool Viewer::hasImage() const
{
    return m_currentImage.has_value();
}

QString Viewer::fileName() const
{
    return m_fileName;
}

QUrl Viewer::imageSource() const
{
    return hasImage() ? m_currentImage->source : QUrl();
}

QSize Viewer::imageDisplaySize() const
{
    return hasImage() ? m_currentImage->displaySize : QSize();
}

bool Viewer::imageAnimated() const
{
    return hasImage() && m_currentImage->animated;
}

QString Viewer::errorMessage() const
{
    return m_errorMessage;
}

}  // namespace pictorio::viewer
