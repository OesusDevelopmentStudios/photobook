/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QObject>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

#include "BulkTagEditor.hpp"
#include "Connection.hpp"
#include "Library.hpp"
#include "LibraryView.hpp"
#include "Navigator.hpp"
#include "NetworkAccess.hpp"
#include "Settings.hpp"
#include "Slideshow.hpp"
#include "TagEditor.hpp"
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

    // For the QML engine: images it loads from the server get the login token, and are cached on disk.
    QQmlNetworkAccessManagerFactory* networkAccessFactory() const;

    // An image or folder chosen in a file dialog, which gives a URL
    Q_INVOKABLE void openImage(const QUrl& fileUrl);
    Q_INVOKABLE void openFolder(const QUrl& folderUrl);
    // An image of the library grid, by its row; next and previous step through the library's results.
    Q_INVOKABLE void openLibraryImage(int row);

private:
    void showLibraryImage(int row);  // from the server, in the viewer

    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<viewer::Viewer> m_viewer;
    std::unique_ptr<view_transform::ViewTransform> m_viewTransform;
    std::unique_ptr<navigator::Navigator> m_navigator;
    std::unique_ptr<slideshow::Slideshow> m_slideshow;     // uses m_navigator and m_settings, so declared after them
    std::unique_ptr<connection::Connection> m_connection;  // uses m_settings
    std::unique_ptr<library::Library> m_library;           // uses m_connection
    std::unique_ptr<library_view::LibraryView> m_libraryView;    // uses m_settings
    std::unique_ptr<tag_editor::TagEditor> m_tagEditor;          // uses m_connection
    std::unique_ptr<tag_editor::BulkTagEditor> m_bulkTagEditor;  // uses m_connection
    std::shared_ptr<network_access::ServerCredentials> m_serverCredentials;
    std::unique_ptr<QQmlNetworkAccessManagerFactory> m_networkAccessFactory;
};

}  // namespace pictorio::app_manager
