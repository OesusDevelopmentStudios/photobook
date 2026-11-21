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

namespace photobook::app_manager
{

// Creates the services through their factories and makes them available to QML. Owns no logic.
class AppManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created in main.cpp")

    Q_PROPERTY(photobook::viewer::Viewer* viewer READ viewer CONSTANT)
    Q_PROPERTY(photobook::view_transform::ViewTransform* viewTransform READ viewTransform CONSTANT)
    Q_PROPERTY(photobook::navigator::Navigator* navigator READ navigator CONSTANT)
    Q_PROPERTY(photobook::slideshow::Slideshow* slideshow READ slideshow CONSTANT)
    Q_PROPERTY(photobook::connection::Connection* connection READ connection CONSTANT)
    Q_PROPERTY(photobook::library::Library* library READ library CONSTANT)
    Q_PROPERTY(photobook::library_view::LibraryView* libraryView READ libraryView CONSTANT)
    Q_PROPERTY(photobook::tag_editor::TagEditor* tagEditor READ tagEditor CONSTANT)
    Q_PROPERTY(photobook::tag_editor::BulkTagEditor* bulkTagEditor READ bulkTagEditor CONSTANT)
    Q_PROPERTY(photobook::tag_editor::UploadTagEditor* uploadTagEditor READ uploadTagEditor CONSTANT)
    Q_PROPERTY(photobook::tag_editor::HiddenFilterEditor* hiddenFilterEditor READ hiddenFilterEditor CONSTANT)
    Q_PROPERTY(photobook::image_actions::ImageActions* imageActions READ imageActions CONSTANT)
    Q_PROPERTY(photobook::uploader::Uploader* uploader READ uploader CONSTANT)

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

    Q_INVOKABLE void openImage(const QUrl& fileUrl);
    Q_INVOKABLE void openFolder(const QUrl& folderUrl);
    // Next and previous then step through the library's results.
    Q_INVOKABLE void openLibraryImage(int row);
    // Into the upload list; a folder without its subfolders
    Q_INVOKABLE void uploadCurrentImage();
    Q_INVOKABLE void uploadCurrentFolder();
    Q_INVOKABLE void copyCurrentImage();
    // A library image's only when the server runs on this computer
    Q_INVOKABLE void openCurrentImageFolder();
    // A local file here, a library image's by the server
    Q_INVOKABLE void saveCurrentRotation();
    // As the two above, for an image of the library grid by its row, without opening it
    Q_INVOKABLE void copyLibraryImage(int row);
    Q_INVOKABLE void openLibraryImageFolder(int row);

private:
    void showLibraryImage(int row);
    void openFolderOf(const server_client::Image& image);  // only when the server runs on this computer

    // Each service after those it uses: created after them, destroyed before them
    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<viewer::Viewer> m_viewer;
    std::unique_ptr<view_transform::ViewTransform> m_viewTransform;
    std::unique_ptr<navigator::Navigator> m_navigator;
    std::unique_ptr<slideshow::Slideshow> m_slideshow;
    std::unique_ptr<connection::Connection> m_connection;
    std::unique_ptr<library::Library> m_library;
    std::unique_ptr<library_view::LibraryView> m_libraryView;
    std::unique_ptr<tag_editor::TagEditor> m_tagEditor;
    std::unique_ptr<tag_editor::BulkTagEditor> m_bulkTagEditor;
    std::unique_ptr<tag_editor::UploadTagEditor> m_uploadTagEditor;
    std::unique_ptr<uploader::Uploader> m_uploader;
    std::unique_ptr<tag_editor::HiddenFilterEditor> m_hiddenFilterEditor;
    std::unique_ptr<image_actions::ImageActions> m_imageActions;
    std::optional<server_client::Image> m_libraryImageOnScreen;  // none while a local file is shown
    std::shared_ptr<network_access::ServerCredentials> m_serverCredentials;
    std::unique_ptr<QQmlNetworkAccessManagerFactory> m_networkAccessFactory;
};

}  // namespace photobook::app_manager
