/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QObject>
#include <QTemporaryDir>

#include "Connection.hpp"
#include "ImageLoader.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "TagEditor.hpp"
#include "TagEditorFactory.hpp"
#include "Viewer.hpp"
#include "ViewerFactory.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"

namespace pictorio::tests
{

// Real services for QML parts that need them, over fakes: a viewer showing a photo from a fake image loader,
// and a tag editor of a connection that is not logged in. Registered for the QML tests in TestQml.cpp.
class TestServices : public QObject
{
    Q_OBJECT

    Q_PROPERTY(pictorio::viewer::Viewer* viewer READ viewer CONSTANT)
    Q_PROPERTY(pictorio::tag_editor::TagEditor* tagEditor READ tagEditor CONSTANT)

    // Answers every file with the same landscape photo, without touching the disk.
    class FakeImageLoader : public image_loader::ImageLoader
    {
    public:
        std::expected<image_loader::ImageInfo, image_loader::ImageLoadError>
        loadInfo(const QString& filePath) const override
        {
            image_loader::ImageInfo imageInfo;
            imageInfo.filePath = filePath;
            imageInfo.displaySize = QSize(4000, 3000);
            imageInfo.mimeType = QStringLiteral("image/jpeg");
            return imageInfo;
        }

        QDateTime readCaptureDate(const QString& /*filePath*/) const override
        {
            return {};
        }
    };

public:
    TestServices()
        : m_settings(settings::createSettings(m_folder.filePath(QStringLiteral("pictoriorc"))))
        , m_connection(std::make_unique<connection::Connection>(
              [server = std::make_shared<fakes::FakeServer>()](const QUrl& url) {
                  return server_client::createServerClient(url, server);
              },
              std::make_unique<fakes::FakeTokenStore>(std::make_shared<fakes::Keyring>()),
              m_settings.get()))
        , m_viewer(viewer::createViewer(std::make_unique<FakeImageLoader>()))
        , m_tagEditor(tag_editor::createTagEditor(m_connection.get()))
    {
        m_viewer->openImage(QStringLiteral("/photos/beach.jpg"));
    }

    viewer::Viewer* viewer() const
    {
        return m_viewer.get();
    }

    tag_editor::TagEditor* tagEditor() const
    {
        return m_tagEditor.get();
    }

private:
    QTemporaryDir m_folder;  // the settings, never the user's own
    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<connection::Connection> m_connection;
    std::unique_ptr<viewer::Viewer> m_viewer;
    std::unique_ptr<tag_editor::TagEditor> m_tagEditor;
};

}  // namespace pictorio::tests
