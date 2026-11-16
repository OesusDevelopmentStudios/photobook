/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QFuture>
#include <QString>
#include <QUrl>

#include "ApiTypes.hpp"

namespace pictorio::server_client
{

// The pictorio-db REST API (version 1): one function per endpoint the app uses. Each returns at once; the
// future finishes with the parsed answer or an ApiError. The functions only talk to the server: storing
// the token, retrying or showing errors is up to their callers.
class ServerClient
{
public:
    virtual ~ServerClient();

    virtual QUrl serverUrl() const = 0;

    // Sent with every request; empty: not logged in.
    virtual void setToken(const QString& token) = 0;

    // GET /server/info, works without a login
    virtual QFuture<Result<ServerInfo>> serverInfo() = 0;

    // POST /auth/tokens: logs this device in. Does not set the token; the caller stores it first.
    virtual QFuture<Result<Login>>
    logIn(const QString& username, const QString& password, const QString& deviceName) = 0;

    // GET /auth/me
    virtual QFuture<Result<CurrentUser>> currentUser() = 0;

    // PATCH /auth/me: the user's other devices are logged out, this one stays logged in
    virtual QFuture<Result<CurrentUser>> changeOwnPassword(const QString& currentPassword,
                                                           const QString& newPassword) = 0;

    // GET /auth/tokens
    virtual QFuture<Result<QList<DeviceToken>>> devices() = 0;

    // DELETE /auth/tokens/{id}; revoking the current token logs this app out
    virtual QFuture<Result<void>> revokeDevice(int tokenId) = 0;

    // GET /images: one page of the images the user may see
    virtual QFuture<Result<ImagePage>> searchImages(const ImageQuery& query) = 0;

    // GET /images/{id}
    virtual QFuture<Result<Image>> image(int imageId) = 0;

    // POST /images. A file the server has already gives the error duplicate_image; its Problem Details
    // carry the server's sha256 and, if the user may see that image, existing_image_id.
    virtual QFuture<Result<Image>> uploadImage(const ImageUpload& upload) = 0;

    // PATCH /images/{id}; the server moves the file if a folder-forming tag changed
    virtual QFuture<Result<Image>> changeImageTags(int imageId, const TagChanges& changes) = 0;

    // POST /images/bulk-tags: all or nothing; gives how many images were changed
    virtual QFuture<Result<int>> changeTagsOfImages(const QList<int>& imageIds, const TagChanges& changes) = 0;

    // GET /images/{id}/file: the original file
    virtual QFuture<Result<QByteArray>> imageFile(int imageId) = 0;

    // Where the original file is, for a view that loads it itself; it must send the token too.
    virtual QUrl imageFileUrl(int imageId) const = 0;

    // GET /images/{id}/thumbnail: a WebP image
    virtual QFuture<Result<QByteArray>> thumbnail(int imageId, ThumbnailSize size) = 0;

    // Where the thumbnail is, for a view that loads it itself; it must send the token too.
    virtual QUrl thumbnailUrl(int imageId, ThumbnailSize size) const = 0;

    // GET /tags: tags starting with prefix, most used first. The prefix may name a namespace ("author:sam");
    // namespaceName limits the search to one. The user's restricted tags are never suggested.
    virtual QFuture<Result<QList<TagCount>>>
    tags(const QString& prefix, const QString& namespaceName = {}, int limit = 20) = 0;

    // GET /namespaces
    virtual QFuture<Result<QList<Namespace>>> namespaces() = 0;

    // GET /preferences
    virtual QFuture<Result<Preferences>> preferences() = 0;

    // PUT /preferences: replaces them all
    virtual QFuture<Result<Preferences>> setPreferences(const Preferences& preferences) = 0;

    // GET /jobs: the user's latest jobs, newest first; all of them, or those with the given status
    virtual QFuture<Result<QList<Job>>> jobs(std::optional<JobStatus> status = std::nullopt) = 0;

    // GET /jobs/{id}
    virtual QFuture<Result<Job>> job(int jobId) = 0;
};

}  // namespace pictorio::server_client
