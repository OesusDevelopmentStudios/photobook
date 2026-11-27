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

#include "Albums.hpp"
#include "Connection.hpp"
#include "FolderScannerFactory.hpp"
#include "HiddenFilterEditor.hpp"
#include "ImageLoader.hpp"
#include "LibraryFactory.hpp"
#include "LibraryView.hpp"
#include "LibraryViewFactory.hpp"
#include "NavigatorFactory.hpp"
#include "SearchSuggestions.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "SlideshowFactory.hpp"
#include "TagEditor.hpp"
#include "TagEditorFactory.hpp"
#include "TagPaneLayout.hpp"
#include "UploadTagEditor.hpp"
#include "Uploader.hpp"
#include "VideoPlayer.hpp"
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
// and keeps what is saved; the search suggestions, the albums (vacation_2015 and garden) and the library view use it
// too, and a tag editor for the tag pane, which shows a library photo once asked to. Registered for the QML tests in
// TestQml.cpp.
class TestServices : public QObject
{
    Q_OBJECT

    Q_PROPERTY(photobook::viewer::Viewer* viewer READ viewer CONSTANT)
    Q_PROPERTY(photobook::video_player::VideoPlayer* videoPlayer READ videoPlayer CONSTANT)
    Q_PROPERTY(photobook::slideshow::Slideshow* slideshow READ slideshow CONSTANT)
    // Logged in, with a video whose storyboard has 100 frames 2224 ms apart, 160×90, ten to a row: openSampleVideo()
    Q_PROPERTY(photobook::video_player::VideoPlayer* previewPlayer READ previewPlayer CONSTANT)
    Q_PROPERTY(photobook::tag_editor::TagEditor* tagEditor READ tagEditor CONSTANT)
    Q_PROPERTY(photobook::tag_editor::UploadTagEditor* uploadTagEditor READ uploadTagEditor CONSTANT)
    Q_PROPERTY(photobook::uploader::Uploader* uploader READ uploader CONSTANT)
    Q_PROPERTY(photobook::connection::Connection* accountConnection READ accountConnection CONSTANT)
    Q_PROPERTY(photobook::tag_editor::HiddenFilterEditor* hiddenFilterEditor READ hiddenFilterEditor CONSTANT)
    Q_PROPERTY(photobook::tag_editor::SearchSuggestions* searchSuggestions READ searchSuggestions CONSTANT)
    Q_PROPERTY(photobook::library::Albums* albums READ albums CONSTANT)
    Q_PROPERTY(photobook::library_view::LibraryView* libraryView READ libraryView CONSTANT)
    Q_PROPERTY(photobook::tag_editor::TagEditor* paneTagEditor READ paneTagEditor CONSTANT)
    Q_PROPERTY(photobook::tag_editor::TagPaneLayout* tagPaneLayout READ tagPaneLayout CONSTANT)
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
        , m_videoPlayer(std::make_unique<video_player::VideoPlayer>(m_connection.get(), m_settings.get()))
        , m_navigator(
              navigator::createNavigator(folder_scanner::createFolderScanner(std::make_unique<FakeImageLoader>())))
        , m_slideshow(slideshow::createSlideshow(m_navigator.get(), m_settings.get()))
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

    video_player::VideoPlayer* previewPlayer() const
    {
        return m_previewPlayer.get();
    }

    Q_INVOKABLE void openSampleVideo()
    {
        m_previewPlayer->openLibraryVideo(9);
    }

    slideshow::Slideshow* slideshow() const
    {
        return m_slideshow.get();
    }

    video_player::VideoPlayer* videoPlayer() const

    {

        return m_videoPlayer.get();
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

    tag_editor::SearchSuggestions* searchSuggestions() const
    {
        return m_searchSuggestions.get();
    }

    library::Albums* albums() const
    {
        return m_albums.get();
    }

    library_view::LibraryView* libraryView() const
    {
        return m_libraryView.get();
    }

    tag_editor::TagEditor* paneTagEditor() const
    {
        return m_paneTagEditor.get();
    }

    tag_editor::TagPaneLayout* tagPaneLayout() const
    {
        return m_tagPaneLayout.get();
    }

    // A library photo on screen in the pane's tag editor, not being edited; its tags' counts come from the server.
    Q_INVOKABLE void showSamplePhoto()
    {
        server_client::Image photo;
        photo.id = 5;
        photo.originalFilename = QStringLiteral("beach.jpg");
        photo.tags = samplePhotoTags();
        m_paneTagEditor->setImage(photo);
    }

    static QStringList samplePhotoTags()
    {
        return {QStringLiteral("author:sample_artist"),
                QStringLiteral("category:photo"),
                QStringLiteral("general:beach_day"),
                QStringLiteral("general:summer"),
                QStringLiteral("meta:jpeg")};
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
                               {QStringLiteral("user_editable"), name != QLatin1String("meta")},
                               {QStringLiteral("allowed_values"), QJsonValue::Null}};
        };

        auto server = std::make_shared<fakes::FakeServer>();
        server->answer("GET",
                       api + QStringLiteral("/auth/me"),
                       fakes::jsonResponse(200,
                                           {{QStringLiteral("id"), 2},
                                            {QStringLiteral("username"), QStringLiteral("restrictedUser")},
                                            {QStringLiteral("role"), QStringLiteral("user")},
                                            {QStringLiteral("restricted_tags"), QJsonArray()}}));
        server->answer("GET",
                       api + QStringLiteral("/namespaces"),
                       fakes::jsonResponse(200,
                                           {{QStringLiteral("items"),
                                             QJsonArray{namespaceJson(QStringLiteral("category"), true),
                                                        namespaceJson(QStringLiteral("author"), false),
                                                        namespaceJson(QStringLiteral("rating"), true),
                                                        namespaceJson(QStringLiteral("character"), false),
                                                        namespaceJson(QStringLiteral("meta"), false),
                                                        namespaceJson(QStringLiteral("general"), false)}}}));
        server->answer("GET",
                       api + QStringLiteral("/tags"),
                       fakes::jsonResponse(
                           200,
                           {{QStringLiteral("items"),
                             QJsonArray{QJsonObject{{QStringLiteral("tag"), QStringLiteral("character:sample_hero")},
                                                    {QStringLiteral("media_count"), 3}}}}}));
        server->answer(
            "GET",
            api + QStringLiteral("/preferences"),
            fakes::jsonResponse(200,
                                {{QStringLiteral("hidden_filters"),
                                  QJsonArray{QStringLiteral("rating:private"), QStringLiteral("category:refs")}}}));
        const auto albumJson = [](const QString& tag, int imageCount, int coverId, const QString& lastChanged) {
            return QJsonObject{
                {QStringLiteral("tag"), tag},
                {QStringLiteral("media_count"), imageCount},
                {QStringLiteral("cover"),
                 QJsonObject{{QStringLiteral("id"), coverId}, {QStringLiteral("revision"), 0}}},
                {QStringLiteral("last_changed_at"), lastChanged},
            };
        };
        server->answer(
            "GET",
            api + QStringLiteral("/albums"),
            fakes::jsonResponse(
                200,
                {{QStringLiteral("items"),
                  QJsonArray{
                      albumJson(
                          QStringLiteral("album:vacation_2015"), 84, 1234, QStringLiteral("2026-11-22T10:00:00Z")),
                      albumJson(QStringLiteral("album:garden"), 3, 7, QStringLiteral("2026-11-01T10:00:00Z"))}}}));
        const auto tagCount = [](const char* tag, int imageCount) {
            return QJsonObject{{QStringLiteral("tag"), QString::fromLatin1(tag)},
                               {QStringLiteral("media_count"), imageCount}};
        };
        server->answer("GET",
                       api + QStringLiteral("/media/5/tag-counts"),
                       fakes::jsonResponse(200,
                                           {{QStringLiteral("items"),
                                             QJsonArray{tagCount("author:sample_artist", 40),
                                                        tagCount("category:photo", 1234),
                                                        tagCount("general:beach_day", 45678),
                                                        tagCount("general:summer", 999),
                                                        tagCount("meta:jpeg", 3100000)}}}));
        // The sample photo's tags as changed: the server applies what it is sent.
        server->answerWith("PATCH", api + QStringLiteral("/media/5"), [](const server_client::HttpRequest& request) {
            const QJsonObject changes = QJsonDocument::fromJson(request.body).object();
            QStringList tags = samplePhotoTags();
            for (const QJsonValue& removed : changes.value(QStringLiteral("remove_tags")).toArray())
            {
                tags.removeAll(removed.toString());
            }

            for (const QJsonValue& added : changes.value(QStringLiteral("add_tags")).toArray())
            {
                tags.append(added.toString());
            }

            tags.sort();
            QJsonObject image = fakes::imageJson(5);
            image[QStringLiteral("tags")] = QJsonArray::fromStringList(tags);
            return fakes::jsonResponse(200, image);
        });
        server->answer(
            "POST",
            api + QStringLiteral("/media/9/stream"),
            fakes::jsonResponse(200,
                                {{QStringLiteral("url"), QStringLiteral("/api/v1/media/9/file?t=1&exp=2&sig=s")},
                                 {QStringLiteral("expires_at"), QStringLiteral("2026-11-28T00:44:17Z")}}));
        server->answer(
            "GET",
            api + QStringLiteral("/media/9/storyboard"),
            fakes::jsonResponse(200,
                                {{QStringLiteral("interval_ms"), 2224},
                                 {QStringLiteral("frame_count"), 100},
                                 {QStringLiteral("frame_width"), 160},
                                 {QStringLiteral("frame_height"), 90},
                                 {QStringLiteral("columns"), 10},
                                 {QStringLiteral("sprite"), QStringLiteral("/api/v1/media/9/storyboard/sprite")}}));
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
        m_searchSuggestions = tag_editor::createSearchSuggestions(m_accountConnection.get());
        m_albums = library::createAlbums(m_accountConnection.get(), m_accountSettings.get());
        m_libraryView = library_view::createLibraryView(m_accountSettings.get());
        m_paneTagEditor = tag_editor::createTagEditor(m_accountConnection.get());
        m_tagPaneLayout = tag_editor::createTagPaneLayout(m_accountSettings.get());
        m_previewPlayer =
            std::make_unique<video_player::VideoPlayer>(m_accountConnection.get(), m_accountSettings.get());
    }

    QTemporaryDir m_folder;  // the settings, never the user's own, and the images to upload
    std::unique_ptr<settings::Settings> m_settings;
    std::unique_ptr<connection::Connection> m_connection;
    std::unique_ptr<viewer::Viewer> m_viewer;
    std::unique_ptr<tag_editor::TagEditor> m_tagEditor;
    std::unique_ptr<tag_editor::UploadTagEditor> m_uploadTagEditor;
    std::shared_ptr<fakes::TrashCan> m_trashCan = std::make_shared<fakes::TrashCan>();
    std::unique_ptr<uploader::Uploader> m_uploader;
    std::unique_ptr<video_player::VideoPlayer> m_videoPlayer;
    std::unique_ptr<navigator::Navigator> m_navigator;
    std::unique_ptr<slideshow::Slideshow> m_slideshow;
    std::unique_ptr<video_player::VideoPlayer> m_previewPlayer;
    std::unique_ptr<settings::Settings> m_accountSettings;
    std::unique_ptr<connection::Connection> m_accountConnection;
    std::unique_ptr<tag_editor::HiddenFilterEditor> m_hiddenFilterEditor;
    std::unique_ptr<tag_editor::SearchSuggestions> m_searchSuggestions;
    std::unique_ptr<library::Albums> m_albums;
    std::unique_ptr<library_view::LibraryView> m_libraryView;
    std::unique_ptr<tag_editor::TagEditor> m_paneTagEditor;
    std::unique_ptr<tag_editor::TagPaneLayout> m_tagPaneLayout;
};

}  // namespace photobook::tests
