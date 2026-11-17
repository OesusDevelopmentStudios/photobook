/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>

#include <QObject>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

#include "ApiTypes.hpp"
#include "BulkTagEditor.hpp"
#include "Connection.hpp"
#include "HiddenFilterEditor.hpp"
#include "ImageActions.hpp"
#include "Library.hpp"
#include "LibraryView.hpp"
#include "Navigator.hpp"
#include "NetworkAccess.hpp"
#include "Settings.hpp"
#include "Slideshow.hpp"
#include "TagEditor.hpp"
#include "UploadTagEditor.hpp"
#include "Uploader.hpp"
#include "ViewTransform.hpp"
#include "Viewer.hpp"

namespace pictorio::app_manager
{

// Creates the services through their factories and makes them available to QML. Owns no logic.
class AppManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(pictorio::viewer::Viewer* viewer READ viewer CONSTANT)
    Q_PROPERTY(pictorio::view_transform::ViewTransform* viewTransform READ viewTransform CONSTANT)
    Q_PROPERTY(pictorio::navigator::Navigator* navigator READ navigator CONSTANT)
    Q_PROPERTY(pictorio::slideshow::Slideshow* slideshow READ slideshow CONSTANT)
    Q_PROPERTY(pictorio::connection::Connection* connection READ connection CONSTANT)
    Q_PROPERTY(pictorio::library::Library* library READ library CONSTANT)
    Q_PROPERTY(pictorio::library_view::LibraryView* libraryView READ libraryView CONSTANT)
    Q_PROPERTY(pictorio::tag_editor::TagEditor* tagEditor READ tagEditor CONSTANT)
    Q_PROPERTY(pictorio::tag_editor::BulkTagEditor* bulkTagEditor READ bulkTagEditor CONSTANT)
    Q_PROPERTY(pictorio::tag_editor::UploadTagEditor* uploadTagEditor READ uploadTagEditor CONSTANT)
    Q_PROPERTY(pictorio::tag_editor::HiddenFilterEditor* hiddenFilterEditor READ hiddenFilterEditor CONSTANT)
    Q_PROPERTY(pictorio::image_actions::ImageActions* imageActions READ imageActions CONSTANT)
    Q_PROPERTY(pictorio::uploader::Uploader* uploader READ uploader CONSTANT)

public:
    explicit AppManager(QObject* parent = nullptr);
    ~AppManager() override;

    viewer::Viewer* viewer() const;
    view_transform::ViewTransform* viewTransform() const;
    navigator::Navigator* navigator() const;
    slideshow::Slideshow* slideshow() const;
    connection::Connection* connection() const;
    library::Library* library() const;
    library_view::LibraryView* libraryView() const;
    tag_editor::TagEditor* tagEditor() const;
    tag_editor::BulkTagEditor* bulkTagEditor() const;
    tag_editor::UploadTagEditor* uploadTagEditor() const;
    tag_editor::HiddenFilterEditor* hiddenFilterEditor() const;
    image_actions::ImageActions* imageActions() const;
    uploader::Uploader* uploader() const;

    // For the QML engine: images it loads from the server get the login token, and are cached on disk.
    QQmlNetworkAccessManagerFactory* networkAccessFactory() const;

    // An image or folder chosen in a file dialog, which gives a URL
    Q_INVOKABLE void openImage(const QUrl& fileUrl);
    Q_INVOKABLE void openFolder(const QUrl& folderUrl);
    // An image of the library grid, by its row; next and previous step through the library's results.
    Q_INVOKABLE void openLibraryImage(int row);
    // The local image on screen, or the images of its folder without subfolders, join the upload list.
    Q_INVOKABLE void uploadCurrentImage();
    Q_INVOKABLE void uploadCurrentFolder();
    // The image on screen to the clipboard: a local file, or a library image downloaded first
    Q_INVOKABLE void copyCurrentImage();

private:
    void showLibraryImage(int row);  // from the server, in the viewer

    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<viewer::Viewer> m_viewer;
    std::unique_ptr<view_transform::ViewTransform> m_viewTransform;
    std::unique_ptr<navigator::Navigator> m_navigator;
    std::unique_ptr<slideshow::Slideshow> m_slideshow;     // uses m_navigator and m_settings, so declared after them
    std::unique_ptr<connection::Connection> m_connection;  // uses m_settings
    std::unique_ptr<library::Library> m_library;           // uses m_connection
    std::unique_ptr<library_view::LibraryView> m_libraryView;              // uses m_settings
    std::unique_ptr<tag_editor::TagEditor> m_tagEditor;                    // uses m_connection
    std::unique_ptr<tag_editor::BulkTagEditor> m_bulkTagEditor;            // uses m_connection
    std::unique_ptr<tag_editor::UploadTagEditor> m_uploadTagEditor;        // uses m_connection
    std::unique_ptr<uploader::Uploader> m_uploader;                        // uses m_connection and m_uploadTagEditor
    std::unique_ptr<tag_editor::HiddenFilterEditor> m_hiddenFilterEditor;  // uses m_connection
    std::unique_ptr<image_actions::ImageActions> m_imageActions;           // uses m_connection
    std::optional<server_client::Image> m_libraryImageOnScreen;            // none while a local file is shown
    std::shared_ptr<network_access::ServerCredentials> m_serverCredentials;
    std::unique_ptr<QQmlNetworkAccessManagerFactory> m_networkAccessFactory;
};

}  // namespace pictorio::app_manager
