/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>
#include <functional>
#include <optional>

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

namespace photobook::server_client
{

// Error codes made by the client itself; every other code comes from the server.
inline const QString networkErrorCode = QStringLiteral("network_error");        // no answer, e.g. offline
inline const QString invalidResponseCode = QStringLiteral("invalid_response");  // not what the API promises
inline const QString fileUnreadableCode = QStringLiteral("file_unreadable");    // an upload's file, on this computer

struct FieldError
{
    QString field;  // e.g. "query.limit"
    QString message;
};

// Why a request failed: the server's Problem Details (RFC 9457), or a network or response error. The app
// branches on `code` (e.g. "tag_restricted", "duplicate_media"), never on the title.
struct ApiError
{
    int httpStatus = 0;  // 0 if no answer arrived
    QString code;
    QString title;
    QString detail;
    QList<FieldError> fieldErrors;  // for validation errors
    QJsonObject problem;            // the whole answer, for fields that only some codes have
};

template <typename T> using Result = std::expected<T, ApiError>;

enum class Role
{
    User,
    Admin,
};

struct ServerInfo
{
    QString name;
    QString version;
    int apiVersion = 0;
    bool setupRequired = false;  // no admin account yet: the server must be set up in a browser first
};

struct UserSummary
{
    int id = 0;
    QString username;
    Role role = Role::User;
};

struct CurrentUser
{
    UserSummary user;
    QStringList restrictedTags;  // search terms; the app refuses these tags before sending anything
};

struct Login
{
    int tokenId = 0;
    QString token;  // the only time the token is available; store it safely
    UserSummary user;
};

// A logged-in device of the current user
struct DeviceToken
{
    int tokenId = 0;
    QString deviceName;
    QDateTime createdAt;  // UTC
    QDateTime lastUsedAt;
    std::optional<QDateTime> expiresAt;  // only web admin logins end by themselves
    bool isCurrent = false;              // the token this app is using
};

// --- Images

enum class TakenAtSource
{
    Exif,        // the photo's own date
    FileMtime,   // the file's modification time, as the uploading client saw it
    ImportTime,  // neither was known
};

struct Image
{
    int id = 0;
    QString sha256;
    QString originalFilename;
    QString storagePath;  // relative to the server's library folder
    QString mimeType;
    int width = 0;  // as the image is shown, after its EXIF orientation
    int height = 0;
    qint64 fileSize = 0;
    QDateTime takenAt;  // local wall-clock time where the photo was taken, without a time zone
    TakenAtSource takenAtSource = TakenAtSource::ImportTime;
    QDateTime importedAt;  // UTC
    QDateTime updatedAt;
    QStringList tags;  // sorted by namespace, then name, e.g. "category:photo"
    int revision = 0;  // grows when the file changes, e.g. a saved rotation; part of the file's and thumbnails' URLs
};

enum class SortBy
{
    TakenAt,
    ImportedAt,
    Random,  // the same order for the same seed, so paging works
    Album,   // the order images were added to ImageQuery::album, which it needs; ignores the sort order
};

enum class SortOrder
{
    Descending,  // newest first
    Ascending,
};

struct ImageQuery
{
    QString text;  // the search syntax, e.g. "category:photo -rating:private"; empty: all images
    QString album;  // an album's name, without "album:": only its images; empty: any image
    SortBy sortBy = SortBy::TakenAt;
    SortOrder order = SortOrder::Descending;  // ignored for random
    std::optional<qint64> seed;               // for random; without one the server picks it
    int limit = 100;                          // 1 to 500
    QString cursor;                           // nextCursor of the page before; empty: the first page
    bool applyHiddenFilters = true;           // the user's hidden-by-default filters
};

struct ImagePage
{
    QList<Image> items;
    QString nextCursor;      // empty on the last page
    QString effectiveQuery;  // the query with the hidden filters that were added
};

// Where the library is on the server's computer, for a client on that computer
struct LocalLibrary
{
    QString libraryDir;  // an image's file is there at its storagePath
};

struct ImageCount
{
    int count = 0;           // over all pages of the search
    QString effectiveQuery;  // the query with the hidden filters that were added
};

struct TagChanges
{
    QStringList addTags;  // created when unknown; in a one-tag-per-image namespace a new tag replaces the old
    QStringList removeTags;
};

struct ImageUpload
{
    QString filePath;  // read from disk while it is sent
    QString originalFilename;
    std::optional<QDateTime> fileModified;  // sent as local wall-clock time, the date used without EXIF
    QString sha256;                         // hex; the server refuses the file if it arrived different
    QStringList tags;
    std::function<void(double share)> onProgress;  // how much of the file is sent, 0 to 1; may be left empty
};

enum class ThumbnailSize
{
    Small = 256,  // longest side in pixels
    Large = 1024,
};

// --- Albums, tags, namespaces, preferences

// The images with one album: tag
struct Album
{
    QString tag;              // e.g. "album:vacation_2015"; its name goes into ImageQuery::album
    int imageCount = 0;       // the images the user may see
    int coverImageId = 0;     // the album's first image, in the order images were added to it
    int coverRevision = 0;    // for the cover's thumbnail URL
    QDateTime lastChangedAt;  // UTC; when an image was last added to it

    bool operator==(const Album&) const = default;
};

struct TagCount
{
    QString tag;         // e.g. "author:sample_artist"
    int imageCount = 0;  // all images with it, also ones the user cannot see
};

struct Namespace
{
    QString name;
    bool singleValue = false;  // an image has at most one tag in it, e.g. category
    bool isBuiltin = false;
    bool inPathTemplate = false;               // its tags form folder names in the server's library
    bool userEditable = true;                  // false: the server alone sets its tags, e.g. meta
    std::optional<QStringList> allowedValues;  // the only names its tags may have, e.g. the ratings; nothing: any
};

struct Preferences
{
    QStringList hiddenFilters;  // search terms hidden unless searched for, e.g. "rating:private"
};

}  // namespace photobook::server_client
