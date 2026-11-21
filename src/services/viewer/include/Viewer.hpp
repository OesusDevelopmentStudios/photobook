/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>

#include <QDateTime>
#include <QObject>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

#include "ImageLoader.hpp"

namespace pictorio::viewer
{

class Viewer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool hasImage READ hasImage NOTIFY currentImageChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY currentImageChanged)
    Q_PROPERTY(QUrl imageSource READ imageSource NOTIFY currentImageChanged)
    Q_PROPERTY(QSize imageDisplaySize READ imageDisplaySize NOTIFY currentImageChanged)
    Q_PROPERTY(bool imageAnimated READ imageAnimated NOTIFY currentImageChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY currentImageChanged)

    Q_PROPERTY(bool fromLibrary READ isFromLibrary NOTIFY detailsChanged)
    Q_PROPERTY(QString mimeType READ mimeType NOTIFY detailsChanged)
    Q_PROPERTY(qint64 fileSize READ fileSize NOTIFY detailsChanged)
    Q_PROPERTY(QDateTime takenAt READ takenAt NOTIFY detailsChanged)
    Q_PROPERTY(TakenAtSource takenAtSource READ takenAtSource NOTIFY detailsChanged)
    Q_PROPERTY(QDateTime importedAt READ importedAt NOTIFY detailsChanged)
    Q_PROPERTY(QString location READ location NOTIFY detailsChanged)
    Q_PROPERTY(QVariantList tagGroups READ tagGroups NOTIFY detailsChanged)

public:
    enum class TakenAtSource
    {
        Unknown,     // a local file without a date in it
        Photo,       // the photo's own EXIF date
        FileDate,    // the file's modification time, as the uploading computer saw it
        ImportTime,  // neither was known when it came into the library
    };
    Q_ENUM(TakenAtSource)

    struct ImageDetails
    {
        QString fileName;
        QSize displaySize;  // after the EXIF orientation
        QString mimeType;
        qint64 fileSize = 0;
        QDateTime takenAt;  // invalid if not known
        TakenAtSource takenAtSource = TakenAtSource::Unknown;
        QDateTime importedAt;  // into the library; invalid for local files
        QString location;      // the folder, or the place in the server's library
        QStringList tags;      // of a library image, e.g. "category:photo", sorted by namespace
    };

    explicit Viewer(std::unique_ptr<image_loader::ImageLoader> imageLoader, QObject* parent = nullptr);
    ~Viewer() override;

    Q_INVOKABLE void openImage(const QString& filePath);
    // Loaded from fileUrl by the page; the MIME type tells whether it may be animated.
    void openServerImage(const QUrl& fileUrl, const ImageDetails& details);
    // After the file changed, e.g. a saved rotation: imageFileChanged() tells the page to load it again, not from
    // its cache.
    void reloadImage();
    // The image itself stays, so the view keeps its zoom.
    void updateDetails(const ImageDetails& details);

    bool hasImage() const;
    QString fileName() const;  // also set when opening failed, for the error message
    QUrl imageSource() const;
    QSize imageDisplaySize() const;
    bool imageAnimated() const;
    QString errorMessage() const;  // empty when the image opened

    bool isFromLibrary() const;
    QString mimeType() const;
    qint64 fileSize() const;
    QDateTime takenAt() const;
    TakenAtSource takenAtSource() const;
    QDateTime importedAt() const;
    QString location() const;
    // The tags by namespace, in the tags' order: a list of {"namespace": "general", "names": ["beach", …]}
    QVariantList tagGroups() const;

Q_SIGNALS:
    void currentImageChanged();  // also changes the details
    void imageFileChanged();     // the same file, changed
    void detailsChanged();

private:
    struct ShownImage
    {
        QUrl source;
        bool animated = false;
        bool fromLibrary = false;
        ImageDetails details;
    };

    std::unique_ptr<image_loader::ImageLoader> m_imageLoader;
    std::optional<ShownImage> m_currentImage;
    QString m_fileName;
    QString m_errorMessage;
};

}  // namespace pictorio::viewer
