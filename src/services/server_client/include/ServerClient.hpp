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

namespace photobook::server_client
{

// The photobook-db REST API (version 1): one function per endpoint the app uses. Each returns at once; the
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

    // GET /server/local: the library folder, if the server runs on this computer. machineId is this computer's
    // app-specific ID (utils/MachineId.hpp); another computer's answer is 404 not_same_machine.
    virtual QFuture<Result<LocalLibrary>> localLibrary(const QString& machineId) = 0;

    // POST /auth/tokens: logs this device in. Does not set the token; the caller stores it first.
    virtual QFuture<Result<Login>>
    logIn(const QString& username, const QString& password, const QString& deviceName) = 0;

    // GET /auth/me
    virtual QFuture<Result<CurrentUser>> currentUser() = 0;

    // GET /auth/tokens
    virtual QFuture<Result<QList<DeviceToken>>> devices() = 0;

    // DELETE /auth/tokens/{id}; revoking the current token logs this app out
    virtual QFuture<Result<void>> revokeDevice(int tokenId) = 0;

    // GET /images: one page of the images the user may see
    virtual QFuture<Result<ImagePage>> searchImages(const ImageQuery& query) = 0;

    // GET /images/count: how many images the search finds. Only the query's text and hidden-filter switch
    // count; its sort and paging are ignored.
    virtual QFuture<Result<ImageCount>> countImages(const ImageQuery& query) = 0;

    // GET /images/{id}
    virtual QFuture<Result<Image>> image(int imageId) = 0;

    // POST /images. A file the server has already gives the error duplicate_image; its Problem Details
    // carry the server's sha256 and, if the user may see that image, existing_image_id.
    virtual QFuture<Result<Image>> uploadImage(const ImageUpload& upload) = 0;

    // PATCH /images/{id}; the server moves the file if a folder-forming tag changed
    virtual QFuture<Result<Image>> changeImageTags(int imageId, const TagChanges& changes) = 0;

    // POST /images/bulk-tags: all or nothing; gives how many images were changed
    virtual QFuture<Result<int>> changeTagsOfImages(const QList<int>& imageIds, const TagChanges& changes) = 0;

    // POST /images/{id}/rotation: turned clockwise by 90, 180 or 270 degrees, saved into the file; the image as
    // the server has it then, with its next revision. A type that cannot be turned without losing quality
    // gives rotation_not_supported.
    virtual QFuture<Result<Image>> rotateImage(int imageId, int clockwiseDegrees) = 0;

    // GET /images/{id}/file: the original file
    virtual QFuture<Result<QByteArray>> imageFile(int imageId) = 0;

    // Where the file is, for a view that loads it itself; it must send the token too. The image's revision
    // makes a changed file a new address, so no cache shows the old one.
    virtual QUrl imageFileUrl(int imageId, int revision) const = 0;

    // GET /images/{id}/thumbnail: a WebP image
    virtual QFuture<Result<QByteArray>> thumbnail(int imageId, ThumbnailSize size) = 0;

    // Where the thumbnail is, for a view that loads it itself; it must send the token too. With the image's
    // revision, as for the file.
    virtual QUrl thumbnailUrl(int imageId, ThumbnailSize size, int revision) const = 0;

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
};

}  // namespace photobook::server_client
