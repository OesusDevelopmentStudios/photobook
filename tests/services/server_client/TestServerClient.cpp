/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>

#include "HttpTransport.hpp"
#include "ServerClient.hpp"
#include "ServerClientFactory.hpp"

namespace pictorio::server_client
{

namespace
{

// Records the requests and answers each with the response set before; no network involved.
class FakeTransport : public HttpTransport
{
public:
    QFuture<HttpResponse> send(const HttpRequest& request) override
    {
        requests.append(request);
        return QtFuture::makeReadyValueFuture(nextResponse);
    }

    HttpRequest lastRequest() const
    {
        return requests.last();
    }

    HttpResponse nextResponse;
    QList<HttpRequest> requests;
};

HttpResponse answer(int status, const QByteArray& body = {})
{
    HttpResponse response;
    response.status = status;
    response.body = body;
    return response;
}

QByteArray headerOf(const HttpRequest& request, const QByteArray& name)
{
    for (const auto& [headerName, value] : request.headers)
    {
        if (headerName == name)
        {
            return value;
        }
    }

    return {};
}

QJsonObject jsonBodyOf(const HttpRequest& request)
{
    return QJsonDocument::fromJson(request.body).object();
}

const QByteArray beachSha256(64, 'a');

// An image as the server describes it
QByteArray beachImageJson()
{
    return R"({"id":5,"sha256":)" + ('"' + beachSha256 + '"') + R"(,"original_filename":"beach.jpg",
        "storage_path":"photo/sample_artist/2025/beach_aaaaaaaa.jpg","mime_type":"image/jpeg","width":4000,
        "height":3000,"file_size":5000000000,"taken_at":"2025-07-14T18:03:12","taken_at_source":"exif",
        "imported_at":"2026-11-15T19:22:03.724229Z","updated_at":"2026-11-15T19:22:04Z",
        "tags":["category:photo","general:summer"],"revision":0,"links":{"self":"/api/v1/images/5",
        "file":"/api/v1/images/5/file","thumbnail":"/api/v1/images/5/thumbnail"}})";
}

// As the server sends it once the file changed
QByteArray withRevision(const QByteArray& imageJson, int revision)
{
    QJsonObject image = QJsonDocument::fromJson(imageJson).object();
    image.insert(QStringLiteral("revision"), revision);
    return QJsonDocument(image).toJson(QJsonDocument::Compact);
}

// A query parameter as the server reads it, which turns "+" into a space
QString serverSideValue(const QUrl& url, const QByteArray& key)
{
    for (const QByteArray& pair : url.query(QUrl::FullyEncoded).toUtf8().split('&'))
    {
        const qsizetype separator = pair.indexOf('=');
        if (pair.left(separator) == key)
        {
            QByteArray value = pair.mid(separator + 1);
            value.replace('+', ' ');
            return QString::fromUtf8(QByteArray::fromPercentEncoding(value));
        }
    }

    return {};
}

QString writeFile(const QTemporaryDir& folder, const QString& name, const QByteArray& content)
{
    const QString path = folder.filePath(name);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(content);
    }

    return path;
}

// The future's value; the fake transport answers at once, so it is ready.
template <typename T> T valueOf(const QFuture<T>& future)
{
    Q_ASSERT(future.isFinished());
    return future.result();
}

}  // namespace

class TestServerClient : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void buildsUrlsUnderTheServerUrl_data();
    void buildsUrlsUnderTheServerUrl();
    void sendsTheTokenOnceItIsSet();

    void readsTheServerInfo();
    void asksWhereTheLibraryIsOnThisComputer();
    void logsInAndReadsTheToken();
    void readsTheCurrentUser();
    void readsTheDevices();
    void revokesADevice();

    void turnsProblemDetailsIntoErrors();
    void keepsFieldErrorsAndExtraFields();
    void reportsNetworkErrors();
    void reportsAnswersWithoutProblemDetails();
    void reportsAnswersThatAreNotJson();
    void namesTheFieldThatIsMissing();
    void refusesAnUnknownRole();

    void searchesWithEveryParameter();
    void readsAPageOfImages();
    void hasNoCursorOnTheLastPage();
    void countsTheImagesASearchFinds();
    void readsOneImage();
    void uploadsAFileWithItsMetadata();
    void refusesToUploadAFileItCannotRead();
    void keepsTheDuplicateImageInTheError();
    void changesTheTagsOfAnImage();
    void changesTheTagsOfManyImages();
    void downloadsTheFileAndTheThumbnail();
    void rotatesAnImage();
    void givesAChangedFileANewAddress();

    void suggestsTags();
    void readsTheNamespaces();
    void readsAndReplacesThePreferences();
    void namesTheItemThatIsWrong();

    void realTransportReadsAnAnswer();
    void realTransportReportsARefusedConnection();
    void realTransportUploadsTheFileAsMultipart();

private:
    std::shared_ptr<FakeTransport> m_transport;
    std::unique_ptr<ServerClient> m_client;
};

void TestServerClient::init()
{
    m_transport = std::make_shared<FakeTransport>();
    m_client = createServerClient(QUrl(QStringLiteral("http://raspberrypi.local:8737")), m_transport);
}

void TestServerClient::buildsUrlsUnderTheServerUrl_data()
{
    QTest::addColumn<QString>("serverUrl");
    QTest::addColumn<QString>("expectedUrl");

    QTest::newRow("host and port") << "http://raspberrypi.local:8737"
                                   << "http://raspberrypi.local:8737/api/v1/server/info";
    QTest::newRow("trailing slash") << "https://photos.example.org/"
                                    << "https://photos.example.org/api/v1/server/info";
    QTest::newRow("behind a proxy path") << "https://example.org/pictorio/"
                                         << "https://example.org/pictorio/api/v1/server/info";
}

void TestServerClient::buildsUrlsUnderTheServerUrl()
{
    QFETCH(QString, serverUrl);
    QFETCH(QString, expectedUrl);
    const auto client = createServerClient(QUrl(serverUrl), m_transport);
    m_transport->nextResponse = answer(200, R"({"name":"pictorio-db","version":"0.1.0","api_version":1,
                                                "setup_required":false})");

    QVERIFY(valueOf(client->serverInfo()).has_value());

    QCOMPARE(m_transport->lastRequest().url.toString(), expectedUrl);
}

void TestServerClient::sendsTheTokenOnceItIsSet()
{
    m_transport->nextResponse = answer(204);

    QVERIFY(valueOf(m_client->revokeDevice(1)).has_value());
    m_client->setToken(QStringLiteral("pk_secret"));
    QVERIFY(valueOf(m_client->revokeDevice(1)).has_value());

    QCOMPARE(headerOf(m_transport->requests.at(0), "Authorization"), QByteArray());
    QCOMPARE(headerOf(m_transport->requests.at(1), "Authorization"), QByteArray("Bearer pk_secret"));
}

void TestServerClient::readsTheServerInfo()
{
    m_transport->nextResponse = answer(200, R"({"name":"pictorio-db","version":"0.1.0","api_version":1,
                                                "setup_required":true})");

    const Result<ServerInfo> info = valueOf(m_client->serverInfo());

    QVERIFY(info.has_value());
    QCOMPARE(m_transport->lastRequest().method, QByteArray("GET"));
    QCOMPARE(info->name, QStringLiteral("pictorio-db"));
    QCOMPARE(info->version, QStringLiteral("0.1.0"));
    QCOMPARE(info->apiVersion, 1);
    QVERIFY(info->setupRequired);
}

void TestServerClient::asksWhereTheLibraryIsOnThisComputer()
{
    m_transport->nextResponse = answer(200, R"({"library_dir":"/home/kid/Pictures/pictorio-library"})");

    const Result<LocalLibrary> library = valueOf(m_client->localLibrary(QStringLiteral("9d623a36")));

    const QUrl url = m_transport->lastRequest().url;
    QCOMPARE(url.path(), QStringLiteral("/api/v1/server/local"));
    QCOMPARE(serverSideValue(url, "machine"), QStringLiteral("9d623a36"));
    QVERIFY(library.has_value());
    QCOMPARE(library->libraryDir, QStringLiteral("/home/kid/Pictures/pictorio-library"));
}

void TestServerClient::logsInAndReadsTheToken()
{
    m_transport->nextResponse = answer(201, R"({"token_id":3,"token":"pk_new",
                                                "user":{"id":2,"username":"kid","role":"user"}})");

    const Result<Login> login =
        valueOf(m_client->logIn(QStringLiteral("kid"), QStringLiteral("correct horse"), QStringLiteral("laptop")));

    const HttpRequest request = m_transport->lastRequest();
    QCOMPARE(request.method, QByteArray("POST"));
    QCOMPARE(request.url.path(), QStringLiteral("/api/v1/auth/tokens"));
    QCOMPARE(headerOf(request, "Content-Type"), QByteArray("application/json"));
    QCOMPARE(jsonBodyOf(request),
             QJsonObject({{QStringLiteral("username"), QStringLiteral("kid")},
                          {QStringLiteral("password"), QStringLiteral("correct horse")},
                          {QStringLiteral("device_name"), QStringLiteral("laptop")}}));
    QVERIFY(login.has_value());
    QCOMPARE(login->tokenId, 3);
    QCOMPARE(login->token, QStringLiteral("pk_new"));
    QCOMPARE(login->user.id, 2);
    QCOMPARE(login->user.username, QStringLiteral("kid"));
    QVERIFY(login->user.role == Role::User);
}

void TestServerClient::readsTheCurrentUser()
{
    m_transport->nextResponse = answer(200, R"({"id":1,"username":"apo","role":"admin",
                                                "restricted_tags":["rating:nsfw","character:*"]})");

    const Result<CurrentUser> currentUser = valueOf(m_client->currentUser());

    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/auth/me"));
    QVERIFY(currentUser.has_value());
    QCOMPARE(currentUser->user.username, QStringLiteral("apo"));
    QVERIFY(currentUser->user.role == Role::Admin);
    QCOMPARE(currentUser->restrictedTags, QStringList({QStringLiteral("rating:nsfw"), QStringLiteral("character:*")}));
}

void TestServerClient::readsTheDevices()
{
    m_transport->nextResponse = answer(200, R"([
        {"token_id":1,"device_name":"laptop","created_at":"2026-11-15T19:22:03.724229Z",
         "last_used_at":"2026-11-15T19:22:04Z","expires_at":null,"is_current":true},
        {"token_id":2,"device_name":"Web admin","created_at":"2026-11-15T08:00:00Z",
         "last_used_at":"2026-11-15T09:00:00Z","expires_at":"2026-11-15T20:00:00Z","is_current":false}])");

    const Result<QList<DeviceToken>> devices = valueOf(m_client->devices());

    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/auth/tokens"));
    QVERIFY(devices.has_value());
    QCOMPARE(devices->size(), 2);
    const DeviceToken laptop = devices->at(0);
    QCOMPARE(laptop.deviceName, QStringLiteral("laptop"));
    QCOMPARE(laptop.createdAt, QDateTime(QDate(2026, 11, 15), QTime(19, 22, 3, 724), QTimeZone::UTC));
    QVERIFY(!laptop.expiresAt.has_value());
    QVERIFY(laptop.isCurrent);
    QCOMPARE(devices->at(1).expiresAt.value(), QDateTime(QDate(2026, 11, 15), QTime(20, 0), QTimeZone::UTC));
}

void TestServerClient::revokesADevice()
{
    m_transport->nextResponse = answer(204);

    const Result<void> revoked = valueOf(m_client->revokeDevice(7));

    QCOMPARE(m_transport->lastRequest().method, QByteArray("DELETE"));
    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/auth/tokens/7"));
    QVERIFY(revoked.has_value());
}

void TestServerClient::turnsProblemDetailsIntoErrors()
{
    m_transport->nextResponse = answer(404, R"({"type":"https://pictorio.invalid/problems/not-found",
        "title":"Image not found","status":404,"code":"not_found","detail":"One of the images does not exist."})");

    const Result<Image> result = valueOf(m_client->image(7));

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().httpStatus, 404);
    QCOMPARE(result.error().code, QStringLiteral("not_found"));
    QCOMPARE(result.error().title, QStringLiteral("Image not found"));
    QCOMPARE(result.error().detail, QStringLiteral("One of the images does not exist."));
}

void TestServerClient::keepsFieldErrorsAndExtraFields()
{
    m_transport->nextResponse = answer(422, R"({"type":"t","title":"Invalid request","status":422,
        "code":"validation_error","errors":[{"field":"query.limit","message":"1 to 500"}],"tag":"a|b"})");

    const Result<ImagePage> result = valueOf(m_client->searchImages(ImageQuery{}));

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().fieldErrors.size(), 1);
    QCOMPARE(result.error().fieldErrors.at(0).field, QStringLiteral("query.limit"));
    QCOMPARE(result.error().fieldErrors.at(0).message, QStringLiteral("1 to 500"));
    QCOMPARE(result.error().problem.value(QStringLiteral("tag")).toString(), QStringLiteral("a|b"));
}

void TestServerClient::reportsNetworkErrors()
{
    m_transport->nextResponse.networkError = QStringLiteral("Connection refused");

    const Result<ServerInfo> result = valueOf(m_client->serverInfo());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, networkErrorCode);
    QCOMPARE(result.error().httpStatus, 0);
    QCOMPARE(result.error().detail, QStringLiteral("Connection refused"));
}

void TestServerClient::reportsAnswersWithoutProblemDetails()
{
    m_transport->nextResponse = answer(502, "<html>Bad Gateway</html>");  // e.g. from a reverse proxy

    const Result<ServerInfo> result = valueOf(m_client->serverInfo());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, QStringLiteral("http_502"));
    QCOMPARE(result.error().httpStatus, 502);
}

void TestServerClient::reportsAnswersThatAreNotJson()
{
    m_transport->nextResponse = answer(200, "<html>a captive portal</html>");

    const Result<ServerInfo> result = valueOf(m_client->serverInfo());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, invalidResponseCode);
}

void TestServerClient::namesTheFieldThatIsMissing()
{
    m_transport->nextResponse = answer(201, R"({"token_id":3,"token":"pk_new","user":{"id":2,"username":"kid"}})");

    const Result<Login> result =
        valueOf(m_client->logIn(QStringLiteral("kid"), QStringLiteral("correct horse"), QStringLiteral("laptop")));

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, invalidResponseCode);
    QCOMPARE(result.error().detail, QStringLiteral("user.role: missing"));
}

void TestServerClient::refusesAnUnknownRole()
{
    m_transport->nextResponse = answer(200, R"({"id":1,"username":"apo","role":"owner","restricted_tags":[]})");

    const Result<CurrentUser> result = valueOf(m_client->currentUser());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().detail, QStringLiteral("role: unknown role owner"));
}

void TestServerClient::searchesWithEveryParameter()
{
    m_transport->nextResponse = answer(200, R"({"items":[],"next_cursor":null,"effective_query":""})");
    ImageQuery query;
    query.text = QStringLiteral("category:photo c++ q&a?#1");  // + & ? # are all allowed in tag names
    query.sortBy = SortBy::Random;
    query.order = SortOrder::Ascending;
    query.seed = 7;
    query.limit = 50;
    query.cursor = QStringLiteral("eyJ0IjoiMjAyNSJ9+x");
    query.applyHiddenFilters = false;

    QVERIFY(valueOf(m_client->searchImages(query)).has_value());

    const QUrl url = m_transport->lastRequest().url;
    QCOMPARE(url.path(), QStringLiteral("/api/v1/images"));
    QCOMPARE(serverSideValue(url, "q"), QStringLiteral("category:photo c++ q&a?#1"));
    QCOMPARE(serverSideValue(url, "sort"), QStringLiteral("random"));
    QCOMPARE(serverSideValue(url, "order"), QStringLiteral("asc"));
    QCOMPARE(serverSideValue(url, "seed"), QStringLiteral("7"));
    QCOMPARE(serverSideValue(url, "limit"), QStringLiteral("50"));
    QCOMPARE(serverSideValue(url, "cursor"), QStringLiteral("eyJ0IjoiMjAyNSJ9+x"));
    QCOMPARE(serverSideValue(url, "apply_hidden_filters"), QStringLiteral("false"));
}

void TestServerClient::readsAPageOfImages()
{
    m_transport->nextResponse = answer(200, R"({"items":[)" + beachImageJson() + R"(],
        "next_cursor":"eyJ0IjoiMjAyNSJ9","effective_query":"category:photo -rating:nsfw"})");

    const Result<ImagePage> page = valueOf(m_client->searchImages(ImageQuery()));

    QVERIFY(page.has_value());
    QCOMPARE(page->nextCursor, QStringLiteral("eyJ0IjoiMjAyNSJ9"));
    QCOMPARE(page->effectiveQuery, QStringLiteral("category:photo -rating:nsfw"));
    QCOMPARE(page->items.size(), 1);
    const Image beach = page->items.at(0);
    QCOMPARE(beach.id, 5);
    QCOMPARE(beach.sha256, QString::fromLatin1(beachSha256));
    QCOMPARE(beach.originalFilename, QStringLiteral("beach.jpg"));
    QCOMPARE(beach.storagePath, QStringLiteral("photo/sample_artist/2025/beach_aaaaaaaa.jpg"));
    QCOMPARE(beach.mimeType, QStringLiteral("image/jpeg"));
    QCOMPARE(beach.width, 4000);
    QCOMPARE(beach.height, 3000);
    QCOMPARE(beach.fileSize, Q_INT64_C(5000000000));  // larger than an int
    QCOMPARE(beach.takenAt.date(), QDate(2025, 7, 14));
    QCOMPARE(beach.takenAt.time(), QTime(18, 3, 12));  // the clock time as given, not converted
    QVERIFY(beach.takenAtSource == TakenAtSource::Exif);
    QCOMPARE(beach.importedAt, QDateTime(QDate(2026, 11, 15), QTime(19, 22, 3, 724), QTimeZone::UTC));
    QCOMPARE(beach.tags, QStringList({QStringLiteral("category:photo"), QStringLiteral("general:summer")}));
}

void TestServerClient::hasNoCursorOnTheLastPage()
{
    m_transport->nextResponse = answer(200, R"({"items":[],"next_cursor":null,"effective_query":""})");

    const Result<ImagePage> page = valueOf(m_client->searchImages(ImageQuery()));

    QVERIFY(page.has_value());
    QVERIFY(page->nextCursor.isEmpty());
    QCOMPARE(serverSideValue(m_transport->lastRequest().url, "apply_hidden_filters"), QStringLiteral("true"));
    QVERIFY(!m_transport->lastRequest().url.query().contains(QStringLiteral("cursor")));
}

void TestServerClient::countsTheImagesASearchFinds()
{
    m_transport->nextResponse = answer(200, R"({"count":11230,"effective_query":"summer -rating:nsfw"})");
    ImageQuery query;
    query.text = QStringLiteral("summer");
    query.sortBy = SortBy::Random;  // not part of a count
    query.cursor = QStringLiteral("eyJ0IjoiMjAyNSJ9");
    query.applyHiddenFilters = false;

    const Result<ImageCount> found = valueOf(m_client->countImages(query));

    const QUrl url = m_transport->lastRequest().url;
    QCOMPARE(url.path(), QStringLiteral("/api/v1/images/count"));
    QCOMPARE(serverSideValue(url, "q"), QStringLiteral("summer"));
    QCOMPARE(serverSideValue(url, "apply_hidden_filters"), QStringLiteral("false"));
    QVERIFY(!url.query().contains(QStringLiteral("sort")));
    QVERIFY(!url.query().contains(QStringLiteral("cursor")));
    QCOMPARE(found->count, 11230);
    QCOMPARE(found->effectiveQuery, QStringLiteral("summer -rating:nsfw"));
}

void TestServerClient::readsOneImage()
{
    m_transport->nextResponse = answer(200, beachImageJson());

    const Result<Image> image = valueOf(m_client->image(5));

    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/images/5"));
    QVERIFY(image.has_value());
    QCOMPARE(image->originalFilename, QStringLiteral("beach.jpg"));
}

void TestServerClient::uploadsAFileWithItsMetadata()
{
    QTemporaryDir folder;
    m_transport->nextResponse = answer(201, beachImageJson());
    ImageUpload upload;
    upload.filePath = writeFile(folder, QStringLiteral("beach.jpg"), "fake jpeg bytes");
    upload.originalFilename = QStringLiteral("beach.jpg");
    upload.fileModified = QDateTime(QDate(2025, 7, 14), QTime(18, 3, 12));  // local time on this computer
    upload.sha256 = QString::fromLatin1(beachSha256);
    upload.tags = {QStringLiteral("category:photo"), QStringLiteral("summer")};

    const Result<Image> image = valueOf(m_client->uploadImage(upload));

    const HttpRequest request = m_transport->lastRequest();
    QCOMPARE(request.method, QByteArray("POST"));
    QCOMPARE(request.url.path(), QStringLiteral("/api/v1/images"));
    QVERIFY(request.form.has_value());
    QCOMPARE(request.form->fileField, QByteArray("file"));
    QCOMPARE(request.form->filePath, upload.filePath);
    QCOMPARE(request.form->fileName, QStringLiteral("beach.jpg"));
    QCOMPARE(request.form->fields.size(), 1);
    QCOMPARE(request.form->fields.at(0).first, QByteArray("metadata"));
    const QJsonArray tags({QStringLiteral("category:photo"), QStringLiteral("summer")});
    QCOMPARE(QJsonDocument::fromJson(request.form->fields.at(0).second).object(),
             QJsonObject({{QStringLiteral("original_filename"), QStringLiteral("beach.jpg")},
                          {QStringLiteral("tags"), tags},
                          {QStringLiteral("client_sha256"), QString::fromLatin1(beachSha256)},
                          {QStringLiteral("file_mtime"), QStringLiteral("2025-07-14T18:03:12")}}));
    QVERIFY(image.has_value());
}

void TestServerClient::refusesToUploadAFileItCannotRead()
{
    ImageUpload upload;
    upload.filePath = QStringLiteral("/nowhere/beach.jpg");
    upload.originalFilename = QStringLiteral("beach.jpg");

    const Result<Image> result = valueOf(m_client->uploadImage(upload));

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, fileUnreadableCode);
    QVERIFY(m_transport->requests.isEmpty());  // nothing was sent
}

void TestServerClient::keepsTheDuplicateImageInTheError()
{
    QTemporaryDir folder;
    m_transport->nextResponse =
        answer(409,
               R"({"type":"t","title":"Image already exists","status":409,"code":"duplicate_image",
                        "existing_image_id":5,"sha256":)" +
                   ('"' + beachSha256 + '"') + "}");
    ImageUpload upload;
    upload.filePath = writeFile(folder, QStringLiteral("beach.jpg"), "fake jpeg bytes");
    upload.originalFilename = QStringLiteral("beach.jpg");

    const Result<Image> result = valueOf(m_client->uploadImage(upload));

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().code, QStringLiteral("duplicate_image"));
    QCOMPARE(result.error().problem.value(QStringLiteral("sha256")).toString(), QString::fromLatin1(beachSha256));
    QCOMPARE(result.error().problem.value(QStringLiteral("existing_image_id")).toInt(), 5);
}

void TestServerClient::changesTheTagsOfAnImage()
{
    m_transport->nextResponse = answer(200, beachImageJson());
    const TagChanges changes{{QStringLiteral("author:sample_artist")}, {QStringLiteral("general:summer")}};

    const Result<Image> image = valueOf(m_client->changeImageTags(5, changes));

    const HttpRequest request = m_transport->lastRequest();
    QCOMPARE(request.method, QByteArray("PATCH"));
    QCOMPARE(request.url.path(), QStringLiteral("/api/v1/images/5"));
    QCOMPARE(jsonBodyOf(request),
             QJsonObject({{QStringLiteral("add_tags"), QJsonArray({QStringLiteral("author:sample_artist")})},
                          {QStringLiteral("remove_tags"), QJsonArray({QStringLiteral("general:summer")})}}));
    QVERIFY(image.has_value());
}

void TestServerClient::changesTheTagsOfManyImages()
{
    m_transport->nextResponse = answer(200, R"({"updated":3})");
    const TagChanges changes{{QStringLiteral("album:vacation_2025")}, {}};

    const Result<int> updated = valueOf(m_client->changeTagsOfImages({1, 2, 3}, changes));

    const HttpRequest request = m_transport->lastRequest();
    QCOMPARE(request.method, QByteArray("POST"));
    QCOMPARE(request.url.path(), QStringLiteral("/api/v1/images/bulk-tags"));
    QCOMPARE(jsonBodyOf(request).value(QStringLiteral("image_ids")).toArray(), QJsonArray({1, 2, 3}));
    QCOMPARE(updated.value(), 3);
}

void TestServerClient::rotatesAnImage()
{
    m_transport->nextResponse = answer(200, withRevision(beachImageJson(), 1));

    const Result<Image> image = valueOf(m_client->rotateImage(5, 90));

    const HttpRequest request = m_transport->lastRequest();
    QCOMPARE(request.method, QByteArray("POST"));
    QCOMPARE(request.url.path(), QStringLiteral("/api/v1/images/5/rotation"));
    QCOMPARE(jsonBodyOf(request), QJsonObject({{QStringLiteral("degrees"), 90}}));
    QVERIFY(image.has_value());
    QCOMPARE(image->revision, 1);
}

// The server ignores rev; a new address keeps caches from showing the file as it was
void TestServerClient::givesAChangedFileANewAddress()
{
    QCOMPARE(serverSideValue(m_client->imageFileUrl(5, 2), "rev"), QStringLiteral("2"));
    QCOMPARE(serverSideValue(m_client->thumbnailUrl(5, ThumbnailSize::Small, 2), "rev"), QStringLiteral("2"));
    QCOMPARE(serverSideValue(m_client->thumbnailUrl(5, ThumbnailSize::Small, 2), "size"), QStringLiteral("256"));
    QVERIFY(!m_client->thumbnailUrl(5, ThumbnailSize::Small, 0).query().contains(QStringLiteral("rev")));
}

void TestServerClient::downloadsTheFileAndTheThumbnail()
{
    m_transport->nextResponse = answer(200, "jpeg bytes");
    const Result<QByteArray> file = valueOf(m_client->imageFile(5));
    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/images/5/file"));
    QCOMPARE(file.value(), QByteArray("jpeg bytes"));
    QCOMPARE(m_client->imageFileUrl(5, 0), m_transport->lastRequest().url);  // where views load it

    m_transport->nextResponse = answer(200, "webp bytes");
    const Result<QByteArray> thumbnail = valueOf(m_client->thumbnail(5, ThumbnailSize::Large));
    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/images/5/thumbnail"));
    QCOMPARE(serverSideValue(m_transport->lastRequest().url, "size"), QStringLiteral("1024"));
    QCOMPARE(thumbnail.value(), QByteArray("webp bytes"));
    QCOMPARE(m_client->thumbnailUrl(5, ThumbnailSize::Large, 0),
             m_transport->lastRequest().url);  // where views load it

    m_transport->nextResponse =
        answer(404, R"({"type":"t","title":"Image not found","status":404,"code":"not_found"})");
    const Result<QByteArray> missing = valueOf(m_client->thumbnail(99, ThumbnailSize::Small));
    QCOMPARE(missing.error().code, QStringLiteral("not_found"));
}

void TestServerClient::suggestsTags()
{
    m_transport->nextResponse = answer(200, R"({"items":[{"tag":"author:sample_artist","image_count":412},
                                                         {"tag":"author:sam_other","image_count":3}]})");

    const Result<QList<TagCount>> tags = valueOf(m_client->tags(QStringLiteral("sam"), QStringLiteral("author"), 5));

    const QUrl url = m_transport->lastRequest().url;
    QCOMPARE(url.path(), QStringLiteral("/api/v1/tags"));
    QCOMPARE(serverSideValue(url, "prefix"), QStringLiteral("sam"));
    QCOMPARE(serverSideValue(url, "namespace"), QStringLiteral("author"));
    QCOMPARE(serverSideValue(url, "limit"), QStringLiteral("5"));
    QVERIFY(tags.has_value());
    QCOMPARE(tags->size(), 2);
    QCOMPARE(tags->at(0).tag, QStringLiteral("author:sample_artist"));
    QCOMPARE(tags->at(0).imageCount, 412);
}

void TestServerClient::readsTheNamespaces()
{
    m_transport->nextResponse = answer(200, R"({"items":[
        {"name":"category","single_value":true,"is_builtin":true,"in_path_template":true,"allowed_values":null},
        {"name":"rating","single_value":true,"is_builtin":true,"in_path_template":false,
         "allowed_values":["sfw","nsfw"]},
        {"name":"weather","single_value":false,"is_builtin":false,"in_path_template":false,"allowed_values":null}]})");

    const Result<QList<Namespace>> namespaces = valueOf(m_client->namespaces());

    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/namespaces"));
    QVERIFY(namespaces.has_value());
    QCOMPARE(namespaces->size(), 3);
    const Namespace category = namespaces->at(0);
    QCOMPARE(category.name, QStringLiteral("category"));
    QVERIFY(category.singleValue && category.isBuiltin && category.inPathTemplate);
    QCOMPARE(category.allowedValues, std::nullopt);  // any name
    QCOMPARE(namespaces->at(1).allowedValues, QStringList({QStringLiteral("sfw"), QStringLiteral("nsfw")}));
    QVERIFY(!namespaces->at(2).singleValue);
}

void TestServerClient::readsAndReplacesThePreferences()
{
    m_transport->nextResponse = answer(200, R"({"hidden_filters":["rating:nsfw"]})");
    const Result<Preferences> stored = valueOf(m_client->preferences());
    QCOMPARE(m_transport->lastRequest().url.path(), QStringLiteral("/api/v1/preferences"));
    QCOMPARE(stored->hiddenFilters, QStringList({QStringLiteral("rating:nsfw")}));

    m_transport->nextResponse = answer(200, R"({"hidden_filters":["rating:nsfw","category:refs"]})");
    Preferences changed;
    changed.hiddenFilters = {QStringLiteral("rating:nsfw"), QStringLiteral("category:refs")};
    const Result<Preferences> saved = valueOf(m_client->setPreferences(changed));

    const HttpRequest request = m_transport->lastRequest();
    QCOMPARE(request.method, QByteArray("PUT"));
    QCOMPARE(jsonBodyOf(request),
             QJsonObject({{QStringLiteral("hidden_filters"),
                           QJsonArray({QStringLiteral("rating:nsfw"), QStringLiteral("category:refs")})}}));
    QCOMPARE(saved->hiddenFilters, changed.hiddenFilters);
}

void TestServerClient::namesTheItemThatIsWrong()
{
    m_transport->nextResponse = answer(200, R"([
        {"token_id":1,"device_name":"laptop","created_at":"2026-11-15T19:22:03Z","last_used_at":"2026-11-15T19:22:04Z",
         "expires_at":null,"is_current":true},
        {"device_name":"phone"}])");

    const Result<QList<DeviceToken>> result = valueOf(m_client->devices());

    QVERIFY(!result.has_value());
    QCOMPARE(result.error().detail, QStringLiteral("[1].token_id: missing"));
}

// The real network transport, against a one-answer HTTP server on this computer.
void TestServerClient::realTransportReadsAnAnswer()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    connect(&server, &QTcpServer::newConnection, &server, [&server]() {
        QTcpSocket* socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
            const QByteArray body =
                R"({"name":"pictorio-db","version":"0.1.0","api_version":1,"setup_required":false})";
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                          QByteArray::number(body.size()) + "\r\n\r\n" + body);
            socket->disconnectFromHost();
        });
    });
    const auto client = createServerClient(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort())));

    const QFuture<Result<ServerInfo>> future = client->serverInfo();

    QTRY_VERIFY(future.isFinished());
    QVERIFY(future.result().has_value());
    QCOMPARE(future.result()->name, QStringLiteral("pictorio-db"));
}

void TestServerClient::realTransportReportsARefusedConnection()
{
    QTcpServer server;  // takes a free port and gives it back, so nothing listens there
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const quint16 closedPort = server.serverPort();
    server.close();
    const auto client = createServerClient(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(closedPort)));

    const QFuture<Result<ServerInfo>> future = client->serverInfo();

    QTRY_VERIFY(future.isFinished());
    QVERIFY(!future.result().has_value());
    QCOMPARE(future.result().error().code, networkErrorCode);
}

void TestServerClient::realTransportUploadsTheFileAsMultipart()
{
    QTemporaryDir folder;
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QByteArray received;
    connect(&server, &QTcpServer::newConnection, &server, [&server, &received]() {
        QTcpSocket* socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket, &received]() {
            received += socket->readAll();
            const qsizetype headerEnd = received.indexOf("\r\n\r\n");
            const QRegularExpression lengthHeader(QStringLiteral("content-length: *(\\d+)"),
                                                  QRegularExpression::CaseInsensitiveOption);
            const QRegularExpressionMatch length = lengthHeader.match(QString::fromLatin1(received.left(headerEnd)));
            if (headerEnd < 0 || !length.hasMatch() ||
                received.size() - headerEnd - 4 < length.captured(1).toLongLong())
            {
                return;  // wait for the rest of the request
            }

            const QByteArray body = beachImageJson();
            socket->write("HTTP/1.1 201 Created\r\nContent-Type: application/json\r\nConnection: close\r\n"
                          "Content-Length: " +
                          QByteArray::number(body.size()) + "\r\n\r\n" + body);
            socket->disconnectFromHost();
        });
    });
    const auto client = createServerClient(QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort())));
    ImageUpload upload;
    upload.filePath = writeFile(folder, QStringLiteral("upload.jpg"), "fake jpeg bytes");
    upload.originalFilename = QStringLiteral("ławka.jpg");  // a non-ASCII name must arrive intact

    const QFuture<Result<Image>> future = client->uploadImage(upload);

    QTRY_VERIFY(future.isFinished());
    QVERIFY(future.result().has_value());
    QVERIFY(received.startsWith("POST /api/v1/images HTTP/1.1"));
    const QByteArray request = received.toLower();  // header names are case-insensitive; Qt writes them lower case
    QVERIFY(request.contains("content-type: multipart/form-data; boundary="));
    QVERIFY(request.contains("content-disposition: form-data; name=\"metadata\""));
    QVERIFY(request.contains("content-disposition: form-data; name=\"file\"; filename=\"ławka.jpg\""));
    QVERIFY(request.contains("fake jpeg bytes"));
}

}  // namespace pictorio::server_client

QTEST_GUILESS_MAIN(pictorio::server_client::TestServerClient)
#include "TestServerClient.moc"
