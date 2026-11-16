/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QSignalSpy>
#include <QTest>

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
    const QUrl fileUrl(QStringLiteral("https://photos.example.org/api/v1/images/5/file"));

    m_viewer->openServerImage(fileUrl, QStringLiteral("beach.jpg"), QSize(3000, 4000), QStringLiteral("image/jpeg"));

    QVERIFY(m_imageLoader->requestedPaths.isEmpty());  // nothing on this computer
    QVERIFY(m_viewer->hasImage());
    QCOMPARE(m_viewer->fileName(), QStringLiteral("beach.jpg"));
    QCOMPARE(m_viewer->imageSource(), fileUrl);
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

    m_viewer->openServerImage(QUrl(QStringLiteral("https://photos.example.org/api/v1/images/5/file")),
                              QStringLiteral("waves"),
                              QSize(400, 300),
                              mimeType);

    QCOMPARE(m_viewer->imageAnimated(), animated);
}

void TestViewer::clearsErrorWhenServerImageOpens()
{
    m_imageLoader->nextResult = std::unexpected(ImageLoadError::FileNotFound);
    m_viewer->openImage(QStringLiteral("/photos/missing.jpg"));

    m_viewer->openServerImage(QUrl(QStringLiteral("https://photos.example.org/api/v1/images/5/file")),
                              QStringLiteral("beach.jpg"),
                              QSize(4000, 3000),
                              QStringLiteral("image/jpeg"));

    QVERIFY(m_viewer->hasImage());
    QVERIFY(m_viewer->errorMessage().isEmpty());
}

}  // namespace pictorio::viewer

QTEST_GUILESS_MAIN(pictorio::viewer::TestViewer)

#include "TestViewer.moc"
