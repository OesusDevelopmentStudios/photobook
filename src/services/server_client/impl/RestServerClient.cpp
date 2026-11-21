/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/RestServerClient.hpp"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

#include "hlp/ApiJson.hpp"
#include "hlp/ProblemDetails.hpp"

namespace
{

using namespace pictorio::server_client;

const QString apiPath = QStringLiteral("/api/v1");

constexpr int statusOk = 200;
constexpr int statusCreated = 201;
constexpr int statusNoContent = 204;

// The answer as parse() reads it, if it has the expected status; otherwise the error it stands for.
template <typename Parse>
auto jsonAnswer(const HttpResponse& response, int expectedStatus, Parse parse) -> decltype(parse(QJsonValue()))
{
    if (response.status != expectedStatus)
    {
        return std::unexpected(apiErrorFrom(response));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(response.body, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        return std::unexpected(invalidResponse(QStringLiteral("not JSON: %1").arg(parseError.errorString())));
    }

    return parse(document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object()));
}

Result<void> emptyAnswer(const HttpResponse& response, int expectedStatus)
{
    if (response.status != expectedStatus)
    {
        return std::unexpected(apiErrorFrom(response));
    }

    return {};
}

Result<QByteArray> binaryAnswer(const HttpResponse& response, int expectedStatus)
{
    if (response.status != expectedStatus)
    {
        return std::unexpected(apiErrorFrom(response));
    }

    return response.body;
}

// QUrlQuery leaves "+" as it is, but the server reads it as a space: a search for c++ must arrive as c++.
QString queryValue(const QString& value)
{
    return QString(value).replace(QLatin1Char('+'), QStringLiteral("%2B"));
}

QString sortName(SortBy sortBy)
{
    switch (sortBy)
    {
    case SortBy::ImportedAt:
        return QStringLiteral("imported_at");
    case SortBy::Random:
        return QStringLiteral("random");
    case SortBy::TakenAt:
        break;
    }

    return QStringLiteral("taken_at");
}

QString booleanValue(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString imageFilePath(int imageId)
{
    return QStringLiteral("/images/%1/file").arg(imageId);
}

QString thumbnailPath(int imageId)
{
    return QStringLiteral("/images/%1/thumbnail").arg(imageId);
}

QUrlQuery thumbnailQuery(ThumbnailSize size)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("size"), QString::number(static_cast<int>(size)));
    return parameters;
}

// The server ignores it; it makes a changed file a new address. Revision 0 keeps the address it always had.
QUrlQuery withRevision(QUrlQuery parameters, int revision)
{
    if (revision > 0)
    {
        parameters.addQueryItem(QStringLiteral("rev"), QString::number(revision));
    }

    return parameters;
}

QJsonObject tagChangesJson(const TagChanges& changes)
{
    return {
        {QStringLiteral("add_tags"), QJsonArray::fromStringList(changes.addTags)},
        {QStringLiteral("remove_tags"), QJsonArray::fromStringList(changes.removeTags)},
    };
}

QByteArray uploadMetadataJson(const ImageUpload& upload)
{
    QJsonObject metadata{
        {QStringLiteral("original_filename"), upload.originalFilename},
        {QStringLiteral("tags"), QJsonArray::fromStringList(upload.tags)},
    };
    if (!upload.sha256.isEmpty())
    {
        metadata.insert(QStringLiteral("client_sha256"), upload.sha256);
    }

    if (upload.fileModified.has_value())
    {
        // The server takes the time as it is, as the photo's date: so the local clock time, without a zone.
        const QString localTime = upload.fileModified->toLocalTime().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
        metadata.insert(QStringLiteral("file_mtime"), localTime);
    }

    return QJsonDocument(metadata).toJson(QJsonDocument::Compact);
}

}  // namespace

namespace pictorio::server_client
{

RestServerClient::RestServerClient(const QUrl& serverUrl, std::shared_ptr<HttpTransport> transport)
    : m_serverUrl(serverUrl)
    , m_transport(std::move(transport))
{}

QUrl RestServerClient::serverUrl() const
{
    return m_serverUrl;
}

void RestServerClient::setToken(const QString& token)
{
    m_token = token;
}

QFuture<Result<ServerInfo>> RestServerClient::serverInfo()
{
    return send("GET", QStringLiteral("/server/info")).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, serverInfoFrom);
    });
}

QFuture<Result<Login>>
RestServerClient::logIn(const QString& username, const QString& password, const QString& deviceName)
{
    const QJsonObject body{
        {QStringLiteral("username"), username},
        {QStringLiteral("password"), password},
        {QStringLiteral("device_name"), deviceName},
    };
    return send("POST", QStringLiteral("/auth/tokens"), body).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusCreated, loginFrom);
    });
}

QFuture<Result<CurrentUser>> RestServerClient::currentUser()
{
    return send("GET", QStringLiteral("/auth/me")).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, currentUserFrom);
    });
}

QFuture<Result<QList<DeviceToken>>> RestServerClient::devices()
{
    return send("GET", QStringLiteral("/auth/tokens")).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, devicesFrom);
    });
}

QFuture<Result<void>> RestServerClient::revokeDevice(int tokenId)
{
    return send("DELETE", QStringLiteral("/auth/tokens/%1").arg(tokenId)).then([](const HttpResponse& response) {
        return emptyAnswer(response, statusNoContent);
    });
}

QFuture<Result<ImagePage>> RestServerClient::searchImages(const ImageQuery& query)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("q"), queryValue(query.text));
    parameters.addQueryItem(QStringLiteral("sort"), sortName(query.sortBy));
    parameters.addQueryItem(QStringLiteral("order"),
                            query.order == SortOrder::Ascending ? QStringLiteral("asc") : QStringLiteral("desc"));
    if (query.seed.has_value())
    {
        parameters.addQueryItem(QStringLiteral("seed"), QString::number(*query.seed));
    }

    parameters.addQueryItem(QStringLiteral("limit"), QString::number(query.limit));
    if (!query.cursor.isEmpty())
    {
        parameters.addQueryItem(QStringLiteral("cursor"), queryValue(query.cursor));
    }

    parameters.addQueryItem(QStringLiteral("apply_hidden_filters"), booleanValue(query.applyHiddenFilters));
    return send("GET", QStringLiteral("/images"), std::nullopt, parameters).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, imagePageFrom);
    });
}

QFuture<Result<LocalLibrary>> RestServerClient::localLibrary(const QString& machineId)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("machine"), machineId);
    return send("GET", QStringLiteral("/server/local"), std::nullopt, parameters)
        .then([](const HttpResponse& response) { return jsonAnswer(response, statusOk, localLibraryFrom); });
}

QFuture<Result<ImageCount>> RestServerClient::countImages(const ImageQuery& query)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("q"), queryValue(query.text));
    parameters.addQueryItem(QStringLiteral("apply_hidden_filters"), booleanValue(query.applyHiddenFilters));
    return send("GET", QStringLiteral("/images/count"), std::nullopt, parameters)
        .then([](const HttpResponse& response) { return jsonAnswer(response, statusOk, imageCountFrom); });
}

QFuture<Result<Image>> RestServerClient::image(int imageId)
{
    return send("GET", QStringLiteral("/images/%1").arg(imageId)).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, imageFrom);
    });
}

QFuture<Result<Image>> RestServerClient::uploadImage(const ImageUpload& upload)
{
    const QFileInfo file(upload.filePath);
    if (!file.isFile() || !file.isReadable())
    {
        ApiError error;
        error.code = fileUnreadableCode;
        error.title = QStringLiteral("Cannot read the file");
        error.detail = upload.filePath;
        return QtFuture::makeReadyValueFuture(Result<Image>(std::unexpected(error)));
    }

    MultipartForm form;
    form.fields.emplaceBack("metadata", uploadMetadataJson(upload));
    form.fileField = "file";
    form.filePath = upload.filePath;
    form.fileName = upload.originalFilename;
    return sendForm(QStringLiteral("/images"), form).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusCreated, imageFrom);
    });
}

QFuture<Result<Image>> RestServerClient::changeImageTags(int imageId, const TagChanges& changes)
{
    return send("PATCH", QStringLiteral("/images/%1").arg(imageId), tagChangesJson(changes))
        .then([](const HttpResponse& response) { return jsonAnswer(response, statusOk, imageFrom); });
}

QFuture<Result<int>> RestServerClient::changeTagsOfImages(const QList<int>& imageIds, const TagChanges& changes)
{
    QJsonObject body = tagChangesJson(changes);
    QJsonArray ids;
    for (const int imageId : imageIds)
    {
        ids.append(imageId);
    }

    body.insert(QStringLiteral("image_ids"), ids);
    return send("POST", QStringLiteral("/images/bulk-tags"), body).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, updatedCountFrom);
    });
}

QFuture<Result<QByteArray>> RestServerClient::imageFile(int imageId)
{
    return send("GET", imageFilePath(imageId)).then([](const HttpResponse& response) {
        return binaryAnswer(response, statusOk);
    });
}

QFuture<Result<Image>> RestServerClient::rotateImage(int imageId, int clockwiseDegrees)
{
    const QJsonObject body{{QStringLiteral("degrees"), clockwiseDegrees}};
    return send("POST", QStringLiteral("/images/%1/rotation").arg(imageId), body)
        .then([](const HttpResponse& response) { return jsonAnswer(response, statusOk, imageFrom); });
}

QUrl RestServerClient::imageFileUrl(int imageId, int revision) const
{
    return endpointUrl(imageFilePath(imageId), withRevision({}, revision));
}

QFuture<Result<QByteArray>> RestServerClient::thumbnail(int imageId, ThumbnailSize size)
{
    return send("GET", thumbnailPath(imageId), std::nullopt, thumbnailQuery(size))
        .then([](const HttpResponse& response) { return binaryAnswer(response, statusOk); });
}

QUrl RestServerClient::thumbnailUrl(int imageId, ThumbnailSize size, int revision) const
{
    return endpointUrl(thumbnailPath(imageId), withRevision(thumbnailQuery(size), revision));
}

QFuture<Result<QList<TagCount>>> RestServerClient::tags(const QString& prefix, const QString& namespaceName, int limit)
{
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("prefix"), queryValue(prefix));
    if (!namespaceName.isEmpty())
    {
        parameters.addQueryItem(QStringLiteral("namespace"), queryValue(namespaceName));
    }

    parameters.addQueryItem(QStringLiteral("limit"), QString::number(limit));
    return send("GET", QStringLiteral("/tags"), std::nullopt, parameters).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, tagCountsFrom);
    });
}

QFuture<Result<QList<Namespace>>> RestServerClient::namespaces()
{
    return send("GET", QStringLiteral("/namespaces")).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, namespacesFrom);
    });
}

QFuture<Result<Preferences>> RestServerClient::preferences()
{
    return send("GET", QStringLiteral("/preferences")).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, preferencesFrom);
    });
}

QFuture<Result<Preferences>> RestServerClient::setPreferences(const Preferences& preferences)
{
    const QJsonObject body{{QStringLiteral("hidden_filters"), QJsonArray::fromStringList(preferences.hiddenFilters)}};
    return send("PUT", QStringLiteral("/preferences"), body).then([](const HttpResponse& response) {
        return jsonAnswer(response, statusOk, preferencesFrom);
    });
}

QFuture<HttpResponse> RestServerClient::send(const QByteArray& method,
                                             const QString& path,
                                             const std::optional<QJsonObject>& body,
                                             const QUrlQuery& query) const
{
    HttpRequest request = requestTo(method, path, query);
    if (body.has_value())
    {
        request.headers.emplaceBack("Content-Type", "application/json");
        request.body = QJsonDocument(*body).toJson(QJsonDocument::Compact);
    }

    return m_transport->send(request);
}

QFuture<HttpResponse> RestServerClient::sendForm(const QString& path, const MultipartForm& form) const
{
    HttpRequest request = requestTo("POST", path, {});
    request.form = form;  // the transport sets the multipart Content-Type with its boundary
    return m_transport->send(request);
}

HttpRequest RestServerClient::requestTo(const QByteArray& method, const QString& path, const QUrlQuery& query) const
{
    HttpRequest request;
    request.method = method;
    request.url = endpointUrl(path, query);
    request.headers.emplaceBack("Accept", "application/json");  // also for files: errors come as JSON
    if (!m_token.isEmpty())
    {
        request.headers.emplaceBack("Authorization", "Bearer " + m_token.toUtf8());
    }

    return request;
}

QUrl RestServerClient::endpointUrl(const QString& path, const QUrlQuery& query) const
{
    QUrl url = m_serverUrl;
    QString basePath = url.path();
    while (basePath.endsWith(QLatin1Char('/')))
    {
        basePath.chop(1);
    }

    url.setPath(basePath + apiPath + path);
    url.setQuery(query);
    url.setFragment(QString());
    return url;
}

}  // namespace pictorio::server_client
