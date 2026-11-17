/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTemporaryDir>
#include <QUrl>

#include "Connection.hpp"
#include "ImageLoader.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "TagEditor.hpp"
#include "TagEditorFactory.hpp"
#include "UploadTagEditor.hpp"
#include "Uploader.hpp"
#include "Viewer.hpp"
#include "ViewerFactory.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/FakeTrash.hpp"

namespace pictorio::tests
{

// Real services for QML parts that need them, over fakes: a viewer showing a photo from a fake image loader,
// and a tag editor and an uploader of a connection that is not logged in, with a folder of images to upload.
// Registered for the QML tests in TestQml.cpp.
class TestServices : public QObject
{
    Q_OBJECT

    Q_PROPERTY(pictorio::viewer::Viewer* viewer READ viewer CONSTANT)
    Q_PROPERTY(pictorio::tag_editor::TagEditor* tagEditor READ tagEditor CONSTANT)
    Q_PROPERTY(pictorio::uploader::Uploader* uploader READ uploader CONSTANT)
    Q_PROPERTY(QUrl sampleFolder READ sampleFolder CONSTANT)  // holiday/beach.jpg, holiday/day2/sunset.jpg

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
        , m_uploadTagEditor(tag_editor::createUploadTagEditor(m_connection.get()))
        , m_uploader(std::make_unique<uploader::Uploader>(
              m_connection.get(), m_uploadTagEditor.get(), std::make_unique<fakes::FakeTrash>(m_trashCan)))
    {
        m_viewer->openImage(QStringLiteral("/photos/beach.jpg"));
        for (const QString& image : {QStringLiteral("holiday/beach.jpg"), QStringLiteral("holiday/day2/sunset.jpg")})
        {
            const QString path = m_folder.filePath(image);
            QDir().mkpath(QFileInfo(path).path());
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly))
            {
                qFatal("Could not create test file %s", qPrintable(path));
            }
        }
    }

    viewer::Viewer* viewer() const
    {
        return m_viewer.get();
    }

    tag_editor::TagEditor* tagEditor() const
    {
        return m_tagEditor.get();
    }

    uploader::Uploader* uploader() const
    {
        return m_uploader.get();
    }

    QUrl sampleFolder() const
    {
        return QUrl::fromLocalFile(m_folder.filePath(QStringLiteral("holiday")));
    }

    // A file of the sample folder, by its path below it
    Q_INVOKABLE QUrl sampleFile(const QString& relativePath) const
    {
        return QUrl::fromLocalFile(m_folder.filePath(QStringLiteral("holiday/") + relativePath));
    }

private:
    QTemporaryDir m_folder;  // the settings, never the user's own, and the images to upload
    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<connection::Connection> m_connection;
    std::unique_ptr<viewer::Viewer> m_viewer;
    std::unique_ptr<tag_editor::TagEditor> m_tagEditor;
    std::unique_ptr<tag_editor::UploadTagEditor> m_uploadTagEditor;
    std::shared_ptr<fakes::TrashCan> m_trashCan = std::make_shared<fakes::TrashCan>();
    std::unique_ptr<uploader::Uploader> m_uploader;
};

}  // namespace pictorio::tests
