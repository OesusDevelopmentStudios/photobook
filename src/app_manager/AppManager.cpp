/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "app_manager/AppManager.hpp"

#include <QFileInfo>
#include <QStandardPaths>

#include "ConnectionFactory.hpp"
#include "FolderScannerFactory.hpp"
#include "ImageLoaderFactory.hpp"
#include "LibraryFactory.hpp"
#include "LibraryViewFactory.hpp"
#include "NavigatorFactory.hpp"
#include "SettingsFactory.hpp"
#include "SlideshowFactory.hpp"
#include "TagEditorFactory.hpp"
#include "UploaderFactory.hpp"
#include "ViewTransformFactory.hpp"
#include "ViewerFactory.hpp"

namespace
{

using pictorio::server_client::TakenAtSource;
using pictorio::viewer::Viewer;

Viewer::TakenAtSource takenAtSourceFor(TakenAtSource source)
{
    switch (source)
    {
    case TakenAtSource::Exif:
        return Viewer::TakenAtSource::Photo;
    case TakenAtSource::FileMtime:
        return Viewer::TakenAtSource::FileDate;
    case TakenAtSource::ImportTime:
        break;
    }

    return Viewer::TakenAtSource::ImportTime;
}

// The details the viewer shows with a library image
Viewer::ImageDetails detailsOf(const pictorio::server_client::Image& image)
{
    Viewer::ImageDetails details;
    details.fileName = image.originalFilename;
    details.displaySize = QSize(image.width, image.height);
    details.mimeType = image.mimeType;
    details.fileSize = image.fileSize;
    details.takenAt = image.takenAt;
    details.takenAtSource = takenAtSourceFor(image.takenAtSource);
    details.importedAt = image.importedAt;
    details.location = image.storagePath;
    details.tags = image.tags;
    return details;
}

}  // namespace

namespace pictorio::app_manager
{

AppManager::AppManager(QObject* parent)
    : QObject(parent)
    , m_settings(settings::createSettings())
    , m_viewer(viewer::createViewer(image_loader::createImageLoader()))
    , m_viewTransform(view_transform::createViewTransform())
    , m_navigator(navigator::createNavigator(folder_scanner::createFolderScanner(image_loader::createImageLoader())))
    , m_slideshow(slideshow::createSlideshow(m_navigator.get(), m_settings.get()))
    , m_connection(connection::createConnection(m_settings.get()))
    , m_library(library::createLibrary(m_connection.get()))
    , m_libraryView(library_view::createLibraryView(m_settings.get()))
    , m_tagEditor(tag_editor::createTagEditor(m_connection.get()))
    , m_bulkTagEditor(tag_editor::createBulkTagEditor(m_connection.get()))
    , m_uploadTagEditor(tag_editor::createUploadTagEditor(m_connection.get()))
    , m_uploader(uploader::createUploader(m_connection.get(), m_uploadTagEditor.get()))
    , m_serverCredentials(std::make_shared<network_access::ServerCredentials>())
    , m_networkAccessFactory(network_access::createNetworkAccessFactory(
          m_serverCredentials,
          QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/server")))
{
    connect(m_navigator.get(), &navigator::Navigator::imageRequested, m_viewer.get(), &viewer::Viewer::openImage);
    connect(m_navigator.get(), &navigator::Navigator::libraryImageRequested, this, &AppManager::showLibraryImage);

    // Only library images have tags to edit.
    connect(m_navigator.get(), &navigator::Navigator::imageRequested, this, [this]() {
        m_tagEditor->setImage(std::nullopt);
    });

    // Tags for many images at once: the selected ones, and each as the server has it after the change.
    connect(m_library.get(), &library::Library::selectionChanged, this, [this]() {
        m_bulkTagEditor->setImages(m_library->selectedImages());
    });
    connect(m_bulkTagEditor.get(),
            &tag_editor::BulkTagEditor::imageUpdated,
            m_library.get(),
            &library::Library::replaceImage);

    // New images, and tags added to ones the server had: the grid shows them.
    connect(m_uploader.get(), &uploader::Uploader::finished, m_library.get(), &library::Library::reload);

    // Saved tags: the grid and the viewer show the image as the server answered.
    connect(m_tagEditor.get(), &tag_editor::TagEditor::saved, this, [this](const server_client::Image& image) {
        m_library->replaceImage(image);
        m_viewer->updateDetails(detailsOf(image));
    });

    // Every newly opened image starts fitted to the window and not rotated.
    connect(m_viewer.get(), &viewer::Viewer::currentImageChanged, m_viewTransform.get(), [this]() {
        m_viewTransform->showImage(m_viewer->imageDisplaySize());
    });

    // Images QML loads from the server (the library's thumbnails) need the login too.
    connect(m_connection.get(), &connection::Connection::stateChanged, this, [this]() {
        m_serverCredentials->setLogin(QUrl(m_connection->serverAddress()), m_connection->token());
    });

    m_connection->restoreLogin();  // logged in last time: logged in again, without asking
}

AppManager::~AppManager() = default;

viewer::Viewer* AppManager::viewer() const
{
    return m_viewer.get();
}

view_transform::ViewTransform* AppManager::viewTransform() const
{
    return m_viewTransform.get();
}

navigator::Navigator* AppManager::navigator() const
{
    return m_navigator.get();
}

slideshow::Slideshow* AppManager::slideshow() const
{
    return m_slideshow.get();
}

connection::Connection* AppManager::connection() const
{
    return m_connection.get();
}

library::Library* AppManager::library() const
{
    return m_library.get();
}

library_view::LibraryView* AppManager::libraryView() const
{
    return m_libraryView.get();
}

tag_editor::TagEditor* AppManager::tagEditor() const
{
    return m_tagEditor.get();
}

tag_editor::BulkTagEditor* AppManager::bulkTagEditor() const
{
    return m_bulkTagEditor.get();
}

tag_editor::UploadTagEditor* AppManager::uploadTagEditor() const
{
    return m_uploadTagEditor.get();
}

uploader::Uploader* AppManager::uploader() const
{
    return m_uploader.get();
}

QQmlNetworkAccessManagerFactory* AppManager::networkAccessFactory() const
{
    return m_networkAccessFactory.get();
}

void AppManager::openImage(const QUrl& fileUrl)
{
    m_navigator->openImage(fileUrl.toLocalFile());
}

void AppManager::openFolder(const QUrl& folderUrl)
{
    m_navigator->openFolder(folderUrl.toLocalFile());
}

void AppManager::openLibraryImage(int row)
{
    m_navigator->openLibraryImage(m_library.get(), row);
}

void AppManager::uploadCurrentImage()
{
    const QUrl image = m_viewer->imageSource();
    if (image.isLocalFile())
    {
        m_uploader->addFiles({image});
    }
}

void AppManager::uploadCurrentFolder()
{
    const QUrl image = m_viewer->imageSource();
    if (image.isLocalFile())
    {
        m_uploader->addFolder(QUrl::fromLocalFile(QFileInfo(image.toLocalFile()).path()), false);
    }
}

void AppManager::showLibraryImage(int row)
{
    const std::optional<server_client::Image> image = m_library->imageAt(row);
    m_tagEditor->setImage(image);
    if (!image.has_value() || m_connection->client() == nullptr)
    {
        return;
    }

    m_viewer->openServerImage(m_connection->client()->imageFileUrl(image->id), detailsOf(*image));
}

}  // namespace pictorio::app_manager
