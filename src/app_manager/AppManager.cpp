/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "app_manager/AppManager.hpp"

#include <QStandardPaths>

#include "ConnectionFactory.hpp"
#include "FolderScannerFactory.hpp"
#include "ImageLoaderFactory.hpp"
#include "LibraryFactory.hpp"
#include "LibraryViewFactory.hpp"
#include "NavigatorFactory.hpp"
#include "SettingsFactory.hpp"
#include "SlideshowFactory.hpp"
#include "ViewTransformFactory.hpp"
#include "ViewerFactory.hpp"

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
    , m_serverCredentials(std::make_shared<network_access::ServerCredentials>())
    , m_networkAccessFactory(network_access::createNetworkAccessFactory(
          m_serverCredentials,
          QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/server")))
{
    connect(m_navigator.get(), &navigator::Navigator::imageRequested, m_viewer.get(), &viewer::Viewer::openImage);

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
    const std::optional<server_client::Image> image = m_library->imageAt(row);
    if (!image.has_value() || m_connection->client() == nullptr)
    {
        return;
    }

    m_navigator->clear();  // previous and next would step through the last local folder
    m_viewer->openServerImage(m_connection->client()->imageFileUrl(image->id),
                              image->originalFilename,
                              QSize(image->width, image->height),
                              image->mimeType);
}

}  // namespace pictorio::app_manager
