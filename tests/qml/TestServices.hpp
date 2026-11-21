/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QObject>
#include <QTemporaryDir>
#include <QUrl>

#include "Connection.hpp"
#include "HiddenFilterEditor.hpp"
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
#include "fakes/ImageJson.hpp"

namespace photobook::tests
{

// Real services for QML parts that need them, over fakes: a viewer showing a photo from a fake image loader,
// and a tag editor and an uploader of a connection that is not logged in, with a folder of images to upload.
// The hidden filter editor has a connection of its own, logged in to a server that has two hidden filters saved
// and keeps what is saved. Registered for the QML tests in TestQml.cpp.
class TestServices : public QObject
{
    Q_OBJECT

    Q_PROPERTY(photobook::viewer::Viewer* viewer READ viewer CONSTANT)
    Q_PROPERTY(photobook::tag_editor::TagEditor* tagEditor READ tagEditor CONSTANT)
    Q_PROPERTY(photobook::tag_editor::UploadTagEditor* uploadTagEditor READ uploadTagEditor CONSTANT)
    Q_PROPERTY(photobook::uploader::Uploader* uploader READ uploader CONSTANT)
    Q_PROPERTY(photobook::connection::Connection* accountConnection READ accountConnection CONSTANT)
    Q_PROPERTY(photobook::tag_editor::HiddenFilterEditor* hiddenFilterEditor READ hiddenFilterEditor CONSTANT)
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
        : m_settings(settings::createSettings(m_folder.filePath(QStringLiteral("photobookrc"))))
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
        logInForHiddenFilters();
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

    tag_editor::UploadTagEditor* uploadTagEditor() const
    {
        return m_uploadTagEditor.get();
    }

    uploader::Uploader* uploader() const
    {
        return m_uploader.get();
    }

    connection::Connection* accountConnection() const
    {
        return m_accountConnection.get();
    }

    tag_editor::HiddenFilterEditor* hiddenFilterEditor() const
    {
        return m_hiddenFilterEditor.get();
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
    void logInForHiddenFilters()
    {
        const QString serverUrl = QStringLiteral("https://photos.example.org");
        const QString api = serverUrl + QStringLiteral("/api/v1");
        const auto namespaceJson = [](const QString& name, bool singleValue) {
            return QJsonObject{{QStringLiteral("name"), name},
                               {QStringLiteral("single_value"), singleValue},
                               {QStringLiteral("is_builtin"), true},
                               {QStringLiteral("in_path_template"), false},
                               {QStringLiteral("allowed_values"), QJsonValue::Null}};
        };

        auto server = std::make_shared<fakes::FakeServer>();
        server->answer("GET",
                       api + QStringLiteral("/auth/me"),
                       fakes::jsonResponse(200,
                                           {{QStringLiteral("id"), 2},
                                            {QStringLiteral("username"), QStringLiteral("kid")},
                                            {QStringLiteral("role"), QStringLiteral("user")},
                                            {QStringLiteral("restricted_tags"), QJsonArray()}}));
        server->answer("GET",
                       api + QStringLiteral("/namespaces"),
                       fakes::jsonResponse(200,
                                           {{QStringLiteral("items"),
                                             QJsonArray{namespaceJson(QStringLiteral("category"), true),
                                                        namespaceJson(QStringLiteral("rating"), true),
                                                        namespaceJson(QStringLiteral("character"), false),
                                                        namespaceJson(QStringLiteral("general"), false)}}}));
        server->answer("GET",
                       api + QStringLiteral("/tags"),
                       fakes::jsonResponse(
                           200,
                           {{QStringLiteral("items"),
                             QJsonArray{QJsonObject{{QStringLiteral("tag"), QStringLiteral("character:sample_hero")},
                                                    {QStringLiteral("image_count"), 3}}}}}));
        server->answer(
            "GET",
            api + QStringLiteral("/preferences"),
            fakes::jsonResponse(200,
                                {{QStringLiteral("hidden_filters"),
                                  QJsonArray{QStringLiteral("rating:nsfw"), QStringLiteral("category:refs")}}}));
        server->answerWith("PUT", api + QStringLiteral("/preferences"), [](const server_client::HttpRequest& request) {
            return fakes::jsonResponse(200, QJsonDocument::fromJson(request.body).object());
        });

        m_accountSettings = settings::createSettings(m_folder.filePath(QStringLiteral("accountrc")));
        m_accountSettings->setServerUrl(QUrl(serverUrl));
        auto keyring = std::make_shared<fakes::Keyring>();
        keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
        m_accountConnection = std::make_unique<connection::Connection>(
            [server](const QUrl& url) { return server_client::createServerClient(url, server); },
            std::make_unique<fakes::FakeTokenStore>(keyring),
            m_accountSettings.get());
        m_accountConnection->restoreLogin();  // tests wait until it is logged in
        m_hiddenFilterEditor = tag_editor::createHiddenFilterEditor(m_accountConnection.get());
    }

    QTemporaryDir m_folder;  // the settings, never the user's own, and the images to upload
    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<connection::Connection> m_connection;
    std::unique_ptr<viewer::Viewer> m_viewer;
    std::unique_ptr<tag_editor::TagEditor> m_tagEditor;
    std::unique_ptr<tag_editor::UploadTagEditor> m_uploadTagEditor;
    std::shared_ptr<fakes::TrashCan> m_trashCan = std::make_shared<fakes::TrashCan>();
    std::unique_ptr<uploader::Uploader> m_uploader;
    std::unique_ptr<settings::Settings> m_accountSettings;
    std::unique_ptr<connection::Connection> m_accountConnection;
    std::unique_ptr<tag_editor::HiddenFilterEditor> m_hiddenFilterEditor;
};

}  // namespace photobook::tests
