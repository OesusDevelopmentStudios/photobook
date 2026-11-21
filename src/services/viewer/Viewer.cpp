/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "Viewer.hpp"

#include <QFileInfo>
#include <QVariantMap>

namespace
{

using pictorio::viewer::Viewer;

// Formats whose files may hold an animation. Only the file itself can tell; a still one shows as well.
bool mayBeAnimated(const QString& mimeType)
{
    return mimeType == QLatin1String("image/gif") || mimeType == QLatin1String("image/webp");
}

Viewer::ImageDetails detailsOf(const pictorio::image_loader::ImageInfo& imageInfo)
{
    const QFileInfo file(imageInfo.filePath);
    Viewer::ImageDetails details;
    details.fileName = file.fileName();
    details.displaySize = imageInfo.displaySize;
    details.mimeType = imageInfo.mimeType;
    details.fileSize = file.size();
    details.takenAt = imageInfo.captureDate;
    details.takenAtSource =
        imageInfo.captureDate.isValid() ? Viewer::TakenAtSource::Photo : Viewer::TakenAtSource::Unknown;
    details.location = file.path();
    return details;
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
        m_currentImage = ShownImage{QUrl::fromLocalFile(result->filePath), result->animated, false, detailsOf(*result)};
        m_errorMessage.clear();
    }
    else
    {
        m_currentImage.reset();
        m_errorMessage = image_loader::describeError(result.error());
    }

    Q_EMIT currentImageChanged();
    Q_EMIT detailsChanged();
}

void Viewer::openServerImage(const QUrl& fileUrl, const ImageDetails& details)
{
    m_fileName = details.fileName;
    m_currentImage = ShownImage{fileUrl, mayBeAnimated(details.mimeType), true, details};
    m_errorMessage.clear();
    Q_EMIT currentImageChanged();
    Q_EMIT detailsChanged();
}

void Viewer::reloadImage()
{
    if (!hasImage() || m_currentImage->fromLibrary)
    {
        return;
    }

    openImage(m_currentImage->source.toLocalFile());
    Q_EMIT imageFileChanged();
}

void Viewer::updateDetails(const ImageDetails& details)
{
    if (hasImage() && m_currentImage->fromLibrary)
    {
        m_currentImage->details = details;
        Q_EMIT detailsChanged();
    }
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
    return hasImage() ? m_currentImage->details.displaySize : QSize();
}

bool Viewer::imageAnimated() const
{
    return hasImage() && m_currentImage->animated;
}

QString Viewer::errorMessage() const
{
    return m_errorMessage;
}

bool Viewer::isFromLibrary() const
{
    return hasImage() && m_currentImage->fromLibrary;
}

QString Viewer::mimeType() const
{
    return hasImage() ? m_currentImage->details.mimeType : QString();
}

qint64 Viewer::fileSize() const
{
    return hasImage() ? m_currentImage->details.fileSize : 0;
}

QDateTime Viewer::takenAt() const
{
    return hasImage() ? m_currentImage->details.takenAt : QDateTime();
}

Viewer::TakenAtSource Viewer::takenAtSource() const
{
    return hasImage() ? m_currentImage->details.takenAtSource : TakenAtSource::Unknown;
}

QDateTime Viewer::importedAt() const
{
    return hasImage() ? m_currentImage->details.importedAt : QDateTime();
}

QString Viewer::location() const
{
    return hasImage() ? m_currentImage->details.location : QString();
}

// The server sorts the tags by namespace, so each namespace's tags are next to each other.
QVariantList Viewer::tagGroups() const
{
    QVariantList groups;
    QString groupNamespace;
    QStringList groupNames;
    const auto finishGroup = [&]() {
        if (!groupNames.isEmpty())
        {
            groups.append(
                QVariantMap{{QStringLiteral("namespace"), groupNamespace}, {QStringLiteral("names"), groupNames}});
        }
    };

    const QStringList tags = hasImage() ? m_currentImage->details.tags : QStringList();
    for (const QString& tag : tags)
    {
        const qsizetype colon = tag.indexOf(QLatin1Char(':'));
        const QString namespaceName = tag.left(qMax(colon, 0));
        if (namespaceName != groupNamespace)
        {
            finishGroup();
            groupNamespace = namespaceName;
            groupNames.clear();
        }

        groupNames.append(tag.mid(colon + 1));
    }

    finishGroup();
    return groups;
}

}  // namespace pictorio::viewer
