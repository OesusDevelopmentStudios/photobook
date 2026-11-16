/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QSignalSpy>
#include <QTest>
#include <QTimeZone>

#include "Viewer.hpp"
#include "ViewerFactory.hpp"

namespace
{

using pictorio::image_loader::ImageInfo;
using pictorio::image_loader::ImageLoader;
using pictorio::image_loader::ImageLoadError;

// Returns whatever the test sets in nextResult, without touching the file system.
class FakeImageLoader : public ImageLoader
{
public:
    std::expected<ImageInfo, ImageLoadError> loadInfo(const QString& filePath) const override
    {
        requestedPaths.append(filePath);
        return nextResult;
    }

    QDateTime readCaptureDate(const QString& /*filePath*/) const override
    {
        return {};
    }

    std::expected<ImageInfo, ImageLoadError> nextResult = std::unexpected(ImageLoadError::FileNotFound);
    mutable QStringList requestedPaths;
};

ImageInfo landscapePhoto()
{
    ImageInfo imageInfo;
    imageInfo.filePath = QStringLiteral("/photos/beach.jpg");
    imageInfo.displaySize = QSize(4000, 3000);
    imageInfo.mimeType = QStringLiteral("image/jpeg");
    return imageInfo;
}

const QUrl serverFileUrl(QStringLiteral("https://photos.example.org/api/v1/images/5/file"));

// As the library gives it: portrait, with tags sorted by namespace
pictorio::viewer::Viewer::ImageDetails libraryPhoto()
{
    pictorio::viewer::Viewer::ImageDetails details;
    details.fileName = QStringLiteral("beach.jpg");
    details.displaySize = QSize(3000, 4000);
    details.mimeType = QStringLiteral("image/jpeg");
    details.fileSize = 3100000;
    details.takenAt = QDateTime(QDate(2025, 7, 14), QTime(18, 3));
    details.takenAtSource = pictorio::viewer::Viewer::TakenAtSource::FileDate;
    details.importedAt = QDateTime(QDate(2026, 11, 15), QTime(19, 22), QTimeZone::UTC);
    details.location = QStringLiteral("photo/sample_artist/2025/beach_aaaaaaaa.jpg");
    details.tags = QStringList{QStringLiteral("author:sample_artist"),
                               QStringLiteral("category:photo"),
                               QStringLiteral("general:beach"),
                               QStringLiteral("general:summer")};
    return details;
}

}  // namespace

namespace pictorio::viewer
{

class TestViewer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void startsWithoutImage();
    void opensImage();
    void reportsAnimatedImage();
    void showsErrorWhenImageCannotBeOpened();
    void clearsErrorWhenNextImageOpens();
    void notifiesOncePerOpen();

    void opensServerImage();
    void mayAnimateServerImages_data();
    void mayAnimateServerImages();
    void clearsErrorWhenServerImageOpens();

    void givesTheDetailsOfALocalFile();
    void knowsWhenALocalFileHasNoDate();
    void givesTheDetailsOfALibraryImage();
    void groupsTagsByNamespace();
    void forgetsTheDetailsWhenAnImageCannotBeOpened();
    void updatesTheDetailsWithoutReopening();

private:
    std::unique_ptr<Viewer> m_viewer;
    FakeImageLoader* m_imageLoader = nullptr;  // owned by m_viewer
};

void TestViewer::initTestCase()
{
    KLocalizedString::setApplicationDomain("pictorio");
}

void TestViewer::init()
{
    auto imageLoader = std::make_unique<FakeImageLoader>();
    m_imageLoader = imageLoader.get();
    m_viewer = createViewer(std::move(imageLoader));
}

void TestViewer::startsWithoutImage()
{
    QVERIFY(!m_viewer->hasImage());
    QVERIFY(m_viewer->imageSource().isEmpty());
    QVERIFY(m_viewer->errorMessage().isEmpty());
}

void TestViewer::opensImage()
{
    m_imageLoader->nextResult = landscapePhoto();

    m_viewer->openImage(QStringLiteral("/photos/beach.jpg"));

    QCOMPARE(m_imageLoader->requestedPaths, QStringList{QStringLiteral("/photos/beach.jpg")});
    QVERIFY(m_viewer->hasImage());
    QCOMPARE(m_viewer->fileName(), QStringLiteral("beach.jpg"));
    QCOMPARE(m_viewer->imageSource(), QUrl(QStringLiteral("file:///photos/beach.jpg")));
    QCOMPARE(m_viewer->imageDisplaySize(), QSize(4000, 3000));
    QVERIFY(!m_viewer->imageAnimated());
    QVERIFY(m_viewer->errorMessage().isEmpty());
}

void TestViewer::reportsAnimatedImage()
{
    ImageInfo animation = landscapePhoto();
    animation.animated = true;
    m_imageLoader->nextResult = animation;

    m_viewer->openImage(QStringLiteral("/photos/waves.gif"));

    QVERIFY(m_viewer->imageAnimated());
}

void TestViewer::showsErrorWhenImageCannotBeOpened()
{
    m_imageLoader->nextResult = std::unexpected(ImageLoadError::UnsupportedFormat);

    m_viewer->openImage(QStringLiteral("/documents/notes.txt"));

    QVERIFY(!m_viewer->hasImage());
    QCOMPARE(m_viewer->fileName(), QStringLiteral("notes.txt"));
    QVERIFY(m_viewer->imageSource().isEmpty());
    QCOMPARE(m_viewer->errorMessage(), image_loader::describeError(ImageLoadError::UnsupportedFormat));
}

void TestViewer::clearsErrorWhenNextImageOpens()
{
    m_imageLoader->nextResult = std::unexpected(ImageLoadError::FileNotFound);
    m_viewer->openImage(QStringLiteral("/photos/missing.jpg"));
    m_imageLoader->nextResult = landscapePhoto();

    m_viewer->openImage(QStringLiteral("/photos/beach.jpg"));

    QVERIFY(m_viewer->hasImage());
    QVERIFY(m_viewer->errorMessage().isEmpty());
}

void TestViewer::notifiesOncePerOpen()
{
    QSignalSpy changeSpy(m_viewer.get(), &Viewer::currentImageChanged);
    m_imageLoader->nextResult = landscapePhoto();

    m_viewer->openImage(QStringLiteral("/photos/beach.jpg"));

    QCOMPARE(changeSpy.count(), 1);
}

void TestViewer::opensServerImage()
{
    QSignalSpy changeSpy(m_viewer.get(), &Viewer::currentImageChanged);

    m_viewer->openServerImage(serverFileUrl, libraryPhoto());

    QVERIFY(m_imageLoader->requestedPaths.isEmpty());  // nothing on this computer
    QVERIFY(m_viewer->hasImage());
    QCOMPARE(m_viewer->fileName(), QStringLiteral("beach.jpg"));
    QCOMPARE(m_viewer->imageSource(), serverFileUrl);
    QCOMPARE(m_viewer->imageDisplaySize(), QSize(3000, 4000));
    QVERIFY(!m_viewer->imageAnimated());
    QCOMPARE(changeSpy.count(), 1);
}

void TestViewer::mayAnimateServerImages_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<bool>("animated");

    QTest::newRow("GIF") << QStringLiteral("image/gif") << true;
    QTest::newRow("WebP") << QStringLiteral("image/webp") << true;
    QTest::newRow("JPEG") << QStringLiteral("image/jpeg") << false;
    QTest::newRow("PNG") << QStringLiteral("image/png") << false;
}

void TestViewer::mayAnimateServerImages()
{
    QFETCH(QString, mimeType);
    QFETCH(bool, animated);
    Viewer::ImageDetails details = libraryPhoto();
    details.mimeType = mimeType;

    m_viewer->openServerImage(serverFileUrl, details);

    QCOMPARE(m_viewer->imageAnimated(), animated);
}

void TestViewer::clearsErrorWhenServerImageOpens()
{
    m_imageLoader->nextResult = std::unexpected(ImageLoadError::FileNotFound);
    m_viewer->openImage(QStringLiteral("/photos/missing.jpg"));

    m_viewer->openServerImage(serverFileUrl, libraryPhoto());

    QVERIFY(m_viewer->hasImage());
    QVERIFY(m_viewer->errorMessage().isEmpty());
}

void TestViewer::givesTheDetailsOfALocalFile()
{
    ImageInfo photo = landscapePhoto();
    photo.captureDate = QDateTime(QDate(2025, 7, 14), QTime(18, 3));
    m_imageLoader->nextResult = photo;

    m_viewer->openImage(QStringLiteral("/photos/beach.jpg"));

    QVERIFY(!m_viewer->isFromLibrary());
    QCOMPARE(m_viewer->mimeType(), QStringLiteral("image/jpeg"));
    QCOMPARE(m_viewer->takenAt(), photo.captureDate);
    QCOMPARE(m_viewer->takenAtSource(), Viewer::TakenAtSource::Photo);
    QVERIFY(!m_viewer->importedAt().isValid());
    QCOMPARE(m_viewer->location(), QStringLiteral("/photos"));
    QVERIFY(m_viewer->tagGroups().isEmpty());
}

void TestViewer::knowsWhenALocalFileHasNoDate()
{
    m_imageLoader->nextResult = landscapePhoto();

    m_viewer->openImage(QStringLiteral("/photos/beach.jpg"));

    QVERIFY(!m_viewer->takenAt().isValid());
    QCOMPARE(m_viewer->takenAtSource(), Viewer::TakenAtSource::Unknown);
}

void TestViewer::givesTheDetailsOfALibraryImage()
{
    m_viewer->openServerImage(serverFileUrl, libraryPhoto());

    QVERIFY(m_viewer->isFromLibrary());
    QCOMPARE(m_viewer->fileSize(), Q_INT64_C(3100000));
    QCOMPARE(m_viewer->takenAtSource(), Viewer::TakenAtSource::FileDate);
    QCOMPARE(m_viewer->importedAt(), QDateTime(QDate(2026, 11, 15), QTime(19, 22), QTimeZone::UTC));
    QCOMPARE(m_viewer->location(), QStringLiteral("photo/sample_artist/2025/beach_aaaaaaaa.jpg"));
}

void TestViewer::groupsTagsByNamespace()
{
    m_viewer->openServerImage(serverFileUrl, libraryPhoto());

    const QVariantList groups = m_viewer->tagGroups();

    QCOMPARE(groups.size(), 3);
    QCOMPARE(groups.at(0).toMap().value(QStringLiteral("namespace")).toString(), QStringLiteral("author"));
    QCOMPARE(groups.at(0).toMap().value(QStringLiteral("names")).toStringList(),
             QStringList{QStringLiteral("sample_artist")});
    QCOMPARE(groups.at(1).toMap().value(QStringLiteral("namespace")).toString(), QStringLiteral("category"));
    QCOMPARE(groups.at(2).toMap().value(QStringLiteral("namespace")).toString(), QStringLiteral("general"));
    QCOMPARE(groups.at(2).toMap().value(QStringLiteral("names")).toStringList(),
             QStringList({QStringLiteral("beach"), QStringLiteral("summer")}));
}

void TestViewer::forgetsTheDetailsWhenAnImageCannotBeOpened()
{
    m_viewer->openServerImage(serverFileUrl, libraryPhoto());
    m_imageLoader->nextResult = std::unexpected(ImageLoadError::FileNotFound);

    m_viewer->openImage(QStringLiteral("/photos/missing.jpg"));

    QVERIFY(!m_viewer->isFromLibrary());
    QVERIFY(m_viewer->tagGroups().isEmpty());
    QVERIFY(m_viewer->location().isEmpty());
}

// After a tag change the image stays as it is on screen; only its details change.
void TestViewer::updatesTheDetailsWithoutReopening()
{
    m_viewer->openServerImage(serverFileUrl, libraryPhoto());
    QSignalSpy imageChanged(m_viewer.get(), &Viewer::currentImageChanged);
    QSignalSpy detailsChanged(m_viewer.get(), &Viewer::detailsChanged);
    Viewer::ImageDetails retagged = libraryPhoto();
    retagged.tags = {QStringLiteral("general:sunset")};
    retagged.location = QStringLiteral("photo/sunset_aaaaaaaa.jpg");

    m_viewer->updateDetails(retagged);

    QCOMPARE(imageChanged.count(), 0);
    QCOMPARE(detailsChanged.count(), 1);
    QCOMPARE(m_viewer->location(), QStringLiteral("photo/sunset_aaaaaaaa.jpg"));
    QCOMPARE(m_viewer->tagGroups().size(), 1);
}

}  // namespace pictorio::viewer

QTEST_GUILESS_MAIN(pictorio::viewer::TestViewer)

#include "TestViewer.moc"
