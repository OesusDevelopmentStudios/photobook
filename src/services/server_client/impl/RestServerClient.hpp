/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>

#include <QJsonObject>
#include <QUrlQuery>

#include "HttpTransport.hpp"
#include "ServerClient.hpp"

namespace pictorio::server_client
{

// Builds the requests of the REST API and reads its answers; the transport carries them.
class RestServerClient : public ServerClient
{
public:
    RestServerClient(const QUrl& serverUrl, std::shared_ptr<HttpTransport> transport);

    QUrl serverUrl() const override;
    void setToken(const QString& token) override;

    QFuture<Result<ServerInfo>> serverInfo() override;
    QFuture<Result<Login>> logIn(const QString& username, const QString& password, const QString& deviceName) override;
    QFuture<Result<CurrentUser>> currentUser() override;
    QFuture<Result<CurrentUser>> changeOwnPassword(const QString& currentPassword, const QString& newPassword) override;
    QFuture<Result<QList<DeviceToken>>> devices() override;
    QFuture<Result<void>> revokeDevice(int tokenId) override;

    QFuture<Result<ImagePage>> searchImages(const ImageQuery& query) override;
    QFuture<Result<ImageCount>> countImages(const ImageQuery& query) override;
    QFuture<Result<Image>> image(int imageId) override;
    QFuture<Result<Image>> uploadImage(const ImageUpload& upload) override;
    QFuture<Result<Image>> changeImageTags(int imageId, const TagChanges& changes) override;
    QFuture<Result<int>> changeTagsOfImages(const QList<int>& imageIds, const TagChanges& changes) override;
    QFuture<Result<QByteArray>> imageFile(int imageId) override;
    QUrl imageFileUrl(int imageId) const override;
    QFuture<Result<QByteArray>> thumbnail(int imageId, ThumbnailSize size) override;
    QUrl thumbnailUrl(int imageId, ThumbnailSize size) const override;
    QFuture<Result<QList<TagCount>>> tags(const QString& prefix, const QString& namespaceName, int limit) override;
    QFuture<Result<QList<Namespace>>> namespaces() override;
    QFuture<Result<Preferences>> preferences() override;
    QFuture<Result<Preferences>> setPreferences(const Preferences& preferences) override;
    QFuture<Result<QList<Job>>> jobs(std::optional<JobStatus> status) override;
    QFuture<Result<Job>> job(int jobId) override;

private:
    QFuture<HttpResponse> send(const QByteArray& method,
                               const QString& path,
                               const std::optional<QJsonObject>& body = std::nullopt,
                               const QUrlQuery& query = {}) const;
    QFuture<HttpResponse> sendForm(const QString& path, const MultipartForm& form) const;
    HttpRequest requestTo(const QByteArray& method, const QString& path, const QUrlQuery& query) const;
    QUrl endpointUrl(const QString& path, const QUrlQuery& query) const;

    QUrl m_serverUrl;
    QString m_token;
    std::shared_ptr<HttpTransport> m_transport;
};

}  // namespace pictorio::server_client
