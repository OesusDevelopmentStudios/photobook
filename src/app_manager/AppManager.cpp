/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "app_manager/AppManager.hpp"

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QStandardPaths>

#include "ConnectionFactory.hpp"
#include "FolderScannerFactory.hpp"
#include "ImageActionsFactory.hpp"
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

using photobook::server_client::TakenAtSource;
using photobook::viewer::Viewer;

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

Viewer::ImageDetails detailsOf(const photobook::server_client::Image& image)
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

namespace photobook::app_manager
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
    , m_albums(library::createAlbums(m_connection.get(), m_settings.get()))
    , m_albumImages(library::createLibrary(m_connection.get(), library::Library::Content::Album))
    , m_libraryView(library_view::createLibraryView(m_settings.get()))
    , m_tagEditor(tag_editor::createTagEditor(m_connection.get()))
    , m_bulkTagEditor(tag_editor::createBulkTagEditor(m_connection.get()))
    , m_uploadTagEditor(tag_editor::createUploadTagEditor(m_connection.get()))
    , m_uploader(uploader::createUploader(m_connection.get(), m_uploadTagEditor.get()))
    , m_hiddenFilterEditor(tag_editor::createHiddenFilterEditor(m_connection.get()))
    , m_searchSuggestions(tag_editor::createSearchSuggestions(m_connection.get()))
    , m_imageActions(image_actions::createImageActions(m_connection.get()))
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
        m_libraryImageOnScreen.reset();
    });

    // The first page's rows arrive just after loading ends: their count is known once the event loop runs.
    connect(m_albumImages.get(),
            &library::Library::loadingChanged,
            this,
            &AppManager::albumLoadSettled,
            Qt::QueuedConnection);

    connect(m_library.get(), &library::Library::selectionChanged, this, [this]() {
        m_bulkTagEditor->setImages(m_library->selectedImages());
    });
    connect(m_bulkTagEditor.get(),
            &tag_editor::BulkTagEditor::imageUpdated,
            m_library.get(),
            &library::Library::replaceImage);

    connect(m_uploader.get(), &uploader::Uploader::finished, m_library.get(), &library::Library::reload);

    // Saved hidden filters change what the server leaves out of a search, so the search runs again.
    connect(
        m_hiddenFilterEditor.get(), &tag_editor::HiddenFilterEditor::saved, m_library.get(), &library::Library::reload);

    connect(m_tagEditor.get(), &tag_editor::TagEditor::saved, this, [this](const server_client::Image& image) {
        m_library->replaceImage(image);
        m_albumImages->replaceImage(image);
        m_libraryImageOnScreen = image;  // a tag that forms the path may have moved its file
        m_viewer->updateDetails(detailsOf(image));
    });

    // Now in the file, so no longer turned by the view
    connect(m_imageActions.get(),
            &image_actions::ImageActions::rotationSaved,
            m_viewer.get(),
            &viewer::Viewer::reloadImage);
    // The new revision is a new address for the file and its thumbnail.
    connect(m_imageActions.get(),
            &image_actions::ImageActions::libraryRotationSaved,
            this,
            [this](const server_client::Image& image) {
                m_library->replaceImage(image);
                m_albumImages->replaceImage(image);
                if (m_libraryImageOnScreen.has_value() && m_libraryImageOnScreen->id == image.id &&
                    m_connection->client() != nullptr)
                {
                    m_libraryImageOnScreen = image;
                    m_viewer->openServerImage(m_connection->client()->imageFileUrl(image.id, image.revision),
                                              detailsOf(image));
                }
            });

    connect(m_viewer.get(), &viewer::Viewer::currentImageChanged, m_viewTransform.get(), [this]() {
        m_viewTransform->showImage(m_viewer->imageDisplaySize());
    });

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

library::Albums* AppManager::albums() const
{
    return m_albums.get();
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

tag_editor::HiddenFilterEditor* AppManager::hiddenFilterEditor() const
{
    return m_hiddenFilterEditor.get();
}

tag_editor::SearchSuggestions* AppManager::searchSuggestions() const
{
    return m_searchSuggestions.get();
}

image_actions::ImageActions* AppManager::imageActions() const
{
    return m_imageActions.get();
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
    m_openingAlbum = false;
    m_browsedLibrary = m_library.get();
    m_navigator->openLibraryImage(m_library.get(), row);
}

void AppManager::openAlbum(int row)
{
    const std::optional<server_client::Album> album = m_albums->albumAt(row);
    if (!album.has_value())
    {
        return;
    }

    m_openingAlbum = true;
    m_albumImages->openAlbum(album->tag.mid(album->tag.indexOf(QLatin1Char(':')) + 1));
}

void AppManager::albumLoadSettled()
{
    if (!m_openingAlbum || m_albumImages->isLoading())
    {
        return;
    }

    m_openingAlbum = false;
    if (m_albumImages->count() == 0)
    {
        Q_EMIT albumNotOpened(m_albumImages->errorMessage());
        return;
    }

    m_browsedLibrary = m_albumImages.get();
    m_navigator->openLibraryImage(m_albumImages.get(), 0);
    Q_EMIT albumOpened();
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

void AppManager::copyCurrentImage()
{
    const QUrl image = m_viewer->imageSource();
    if (image.isLocalFile())
    {
        m_imageActions->copyLocalImage(image.toLocalFile());
    }
    else if (m_libraryImageOnScreen.has_value())
    {
        m_imageActions->copyLibraryImage(m_libraryImageOnScreen->id, m_libraryImageOnScreen->mimeType);
    }
}

void AppManager::openCurrentImageFolder()
{
    const QUrl image = m_viewer->imageSource();
    if (image.isLocalFile())
    {
        m_imageActions->openContainingFolder(image.toLocalFile());
    }
    else if (m_libraryImageOnScreen.has_value())
    {
        openFolderOf(*m_libraryImageOnScreen);
    }
}

void AppManager::saveCurrentRotation()
{
    const int angle = m_viewTransform->rotationAngle();
    const QUrl image = m_viewer->imageSource();
    if (angle == 0)
    {
        return;
    }

    if (image.isLocalFile())
    {
        m_imageActions->saveLocalRotation(image.toLocalFile(), angle);
    }
    else if (m_libraryImageOnScreen.has_value())
    {
        m_imageActions->saveLibraryRotation(m_libraryImageOnScreen->id, angle);
    }
}

void AppManager::copyLibraryImage(int row)
{
    if (const std::optional<server_client::Image> image = m_library->imageAt(row))
    {
        m_imageActions->copyLibraryImage(image->id, image->mimeType);
    }
}

void AppManager::openLibraryImageFolder(int row)
{
    if (const std::optional<server_client::Image> image = m_library->imageAt(row))
    {
        openFolderOf(*image);
    }
}

void AppManager::openFolderOf(const server_client::Image& image)
{
    if (!m_connection->localLibraryDir().isEmpty())
    {
        m_imageActions->openContainingFolder(QDir(m_connection->localLibraryDir()).filePath(image.storagePath));
    }
}

void AppManager::showLibraryImage(int row)
{
    const std::optional<server_client::Image> image =
        m_browsedLibrary != nullptr ? m_browsedLibrary->imageAt(row) : std::nullopt;
    m_tagEditor->setImage(image);
    m_libraryImageOnScreen = image;
    if (!image.has_value() || m_connection->client() == nullptr)
    {
        return;
    }

    m_viewer->openServerImage(m_connection->client()->imageFileUrl(image->id, image->revision), detailsOf(*image));
}

}  // namespace photobook::app_manager
