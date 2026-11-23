/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "BulkTagEditor.hpp"
#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "TagEditor.hpp"
#include "TagEditorFactory.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/ImageJson.hpp"

namespace photobook::tag_editor
{

namespace
{

using connection::Connection;
using fakes::FakeServer;
using server_client::HttpRequest;
using server_client::Image;

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString bulkUrl = serverUrl + QStringLiteral("/api/v1/images/bulk-tags");

QJsonObject namespaceJson(const QString& name, bool singleValue, const QJsonValue& allowedValues = QJsonValue::Null)
{
    return {
        {QStringLiteral("name"), name},
        {QStringLiteral("single_value"), singleValue},
        {QStringLiteral("is_builtin"), name != QLatin1String("weather")},
        {QStringLiteral("in_path_template"), name == QLatin1String("category")},
        {QStringLiteral("user_editable"), name != QLatin1String("meta")},  // the server sets meta tags
        {QStringLiteral("allowed_values"), allowedValues},
    };
}

Image imageWith(int id, const QStringList& tags)
{
    Image image;
    image.id = id;
    image.originalFilename = QStringLiteral("photo_%1.jpg").arg(id);
    image.takenAt = QDateTime(QDate(2025, 7, 14), QTime(18, 3));
    image.tags = tags;
    return image;
}

// Two photos at the beach, one also tagged "photo", and an artwork
QList<Image> threeImages()
{
    return {
        imageWith(1,
                  {QStringLiteral("category:photo"), QStringLiteral("general:beach"), QStringLiteral("general:photo")}),
        imageWith(2, {QStringLiteral("category:photo"), QStringLiteral("general:beach")}),
        imageWith(3, {QStringLiteral("category:artwork"), QStringLiteral("general:summer")}),
    };
}

// A namespace's row, by role name, e.g. row["value"]
QVariantMap rowOf(const BulkTagEditor& editor, const QString& namespaceName)
{
    const QAbstractListModel* rows = editor.namespaceRows();
    const QHash<int, QByteArray> roles = rows->roleNames();
    for (int row = 0; row < rows->rowCount(); ++row)
    {
        const QModelIndex index = rows->index(row);
        if (index.data(roles.key("namespaceName")).toString() != namespaceName)
        {
            continue;
        }

        QVariantMap values;
        for (auto role = roles.cbegin(); role != roles.cend(); ++role)
        {
            values.insert(QString::fromLatin1(role.value()), index.data(role.key()));
        }

        return values;
    }

    return {};
}

// What the rows show of each tag: {tag: change}
QHash<QString, TagEditor::Change> tagsShown(const BulkTagEditor& editor)
{
    QHash<QString, TagEditor::Change> shown;
    const QAbstractListModel* rows = editor.namespaceRows();
    const int tagsRole = rows->roleNames().key("tags");
    for (int row = 0; row < rows->rowCount(); ++row)
    {
        for (const QVariant& entry : rows->index(row).data(tagsRole).toList())
        {
            const QVariantMap tag = entry.toMap();
            shown.insert(tag.value(QStringLiteral("tag")).toString(),
                         static_cast<TagEditor::Change>(tag.value(QStringLiteral("change")).toInt()));
        }
    }

    return shown;
}

QJsonObject bodyOf(const HttpRequest& request)
{
    return QJsonDocument::fromJson(request.body).object();
}

}  // namespace

// Tags of many images at once. The user "restrictedUser" is restricted from rating:private, and from images that are both
// nude and a photo.
class TestBulkTagEditor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void showsOnlyTheTagsAllImagesShare();
    void showsTheValueAllImagesShare();
    void keepsAPendingRemovalInView();
    void collectsChangesUntilApplied();
    void appliesToAllImagesInOneChange();
    void setsAValueForAll();
    void removesAValueFromAll();
    void keepsAValueAsIs();
    void addsNothingEveryImageHas();
    void refusesATagThatWouldRestrictAnyImage();
    void refusesMoreImagesThanTheServerTakes();
    void keepsTheChangesWhenTheSelectionChanges();
    void clearDropsTheChanges();
    void appliesTypedNamesToo();
    void keepsTheChangesWhenTheServerRefuses();

private:
    void selectImages(const QList<Image>& images);
    void selectThreeImages();

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<BulkTagEditor> m_editor;
};

void TestBulkTagEditor::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
}

// Logged in from the last run. After a change the server has given every image the tag sunset.
void TestBulkTagEditor::init()
{
    m_editor.reset();
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("photobookrc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    m_server->answer(
        "GET",
        serverUrl + QStringLiteral("/api/v1/auth/me"),
        fakes::jsonResponse(200,
                            {{QStringLiteral("id"), 2},
                             {QStringLiteral("username"), QStringLiteral("restrictedUser")},
                             {QStringLiteral("role"), QStringLiteral("user")},
                             {QStringLiteral("restricted_tags"),
                              QJsonArray{QStringLiteral("rating:private"), QStringLiteral("general:nude,photo")}}}));
    m_server->answer(
        "GET",
        serverUrl + QStringLiteral("/api/v1/namespaces"),
        fakes::jsonResponse(200,
                            {{QStringLiteral("items"),
                              QJsonArray{namespaceJson(QStringLiteral("category"), true),
                                         namespaceJson(QStringLiteral("rating"),
                                                       true,
                                                       QJsonArray{QStringLiteral("public"), QStringLiteral("private")}),
                                         namespaceJson(QStringLiteral("general"), false),
                                         namespaceJson(QStringLiteral("weather"), true),
                                         namespaceJson(QStringLiteral("meta"), false)}}}));
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/tags"),
                     fakes::jsonResponse(200, {{QStringLiteral("items"), QJsonArray()}}));
    m_server->answerWith("POST", bulkUrl, [](const HttpRequest& request) {
        return fakes::jsonResponse(
            200, {{QStringLiteral("updated"), bodyOf(request).value(QStringLiteral("image_ids")).toArray().size()}});
    });
    for (const Image& image : threeImages())
    {
        QJsonObject changed = fakes::imageJson(image.id);
        changed[QStringLiteral("tags")] =
            QJsonArray::fromStringList(image.tags + QStringList{QStringLiteral("general:sunset")});
        m_server->answer(
            "GET", serverUrl + QStringLiteral("/api/v1/images/%1").arg(image.id), fakes::jsonResponse(200, changed));
    }

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
    m_editor = createBulkTagEditor(m_connection.get());
}

// The fake server answers at once, so the namespaces are in when this returns.
void TestBulkTagEditor::selectImages(const QList<Image>& images)
{
    m_editor->setImages(images);
    QTRY_VERIFY(!rowOf(*m_editor, QStringLiteral("weather")).isEmpty());
    QCOMPARE(m_editor->imageCount(), images.size());
}

void TestBulkTagEditor::selectThreeImages()
{
    selectImages(threeImages());
}

// A tag of a single image would look as if all had it.
void TestBulkTagEditor::showsOnlyTheTagsAllImagesShare()
{
    selectImages(threeImages().mid(0, 2));  // both have beach; only the first has general:photo

    QCOMPARE(tagsShown(*m_editor),
             (QHash<QString, TagEditor::Change>{{QStringLiteral("general:beach"), TagEditor::Change::Kept},
                                                {QStringLiteral("category:photo"), TagEditor::Change::Kept}}));

    selectThreeImages();  // nothing shared

    QVERIFY(tagsShown(*m_editor).isEmpty());
    QVERIFY(rowOf(*m_editor, QStringLiteral("meta")).isEmpty());  // the server sets meta tags
}

// Category and rating are kept as is: the dropdown shows the value all share, or "keep as is" if they differ.
void TestBulkTagEditor::showsTheValueAllImagesShare()
{
    selectImages(threeImages().mid(0, 2));

    QVariantMap category = rowOf(*m_editor, QStringLiteral("category"));
    QVERIFY(category.value(QStringLiteral("dropdown")).toBool());
    QVERIFY(category.value(QStringLiteral("keepable")).toBool());
    QVERIFY(!category.value(QStringLiteral("changed")).toBool());
    QCOMPARE(category.value(QStringLiteral("value")).toString(), QStringLiteral("photo"));

    selectThreeImages();

    category = rowOf(*m_editor, QStringLiteral("category"));
    QVERIFY(!category.value(QStringLiteral("changed")).toBool());
    QCOMPARE(category.value(QStringLiteral("value")).toString(), QString());
    QCOMPARE(category.value(QStringLiteral("options")).toStringList(),
             QStringList({QStringLiteral("artwork"), QStringLiteral("photo")}));  // of any image
}

// Changes stay when the selection changes: a removal stays in view, to be undone, also once not all share the tag.
void TestBulkTagEditor::keepsAPendingRemovalInView()
{
    selectImages(threeImages().mid(0, 2));
    QVERIFY(m_editor->removeTag(QStringLiteral("general:beach")));

    selectThreeImages();

    QCOMPARE(tagsShown(*m_editor).value(QStringLiteral("general:beach")), TagEditor::Change::Removed);
    QVERIFY(m_editor->restoreTag(QStringLiteral("general:beach")));
    QVERIFY(!tagsShown(*m_editor).contains(QStringLiteral("general:beach")));
}

void TestBulkTagEditor::collectsChangesUntilApplied()
{
    selectThreeImages();

    QVERIFY(m_editor->addTag(QStringLiteral("general"), QStringLiteral("Sunset")));
    QVERIFY(m_editor->removeTag(QStringLiteral("general:beach")));

    QVERIFY(m_editor->hasChanges());
    QVERIFY(m_server->requestsTo("POST", bulkUrl).isEmpty());
    const QHash<QString, TagEditor::Change> shown = tagsShown(*m_editor);
    QCOMPARE(shown.value(QStringLiteral("general:sunset")), TagEditor::Change::Added);
    QCOMPARE(shown.value(QStringLiteral("general:beach")), TagEditor::Change::Removed);
}

// One request for all, then each image again, as the server answers only how many changed
void TestBulkTagEditor::appliesToAllImagesInOneChange()
{
    selectThreeImages();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("sunset"));
    m_editor->removeTag(QStringLiteral("general:beach"));
    QSignalSpy applied(m_editor.get(), &BulkTagEditor::applied);
    QSignalSpy imageUpdated(m_editor.get(), &BulkTagEditor::imageUpdated);

    m_editor->apply();

    QTRY_COMPARE(imageUpdated.count(), 3);
    QCOMPARE(applied.count(), 1);
    const QList<HttpRequest> changes = m_server->requestsTo("POST", bulkUrl);
    QCOMPARE(changes.size(), 1);
    QCOMPARE(bodyOf(changes.first()).value(QStringLiteral("image_ids")).toArray(), QJsonArray({1, 2, 3}));
    QCOMPARE(bodyOf(changes.first()).value(QStringLiteral("add_tags")).toArray(),
             QJsonArray{QStringLiteral("general:sunset")});
    QCOMPARE(bodyOf(changes.first()).value(QStringLiteral("remove_tags")).toArray(),
             QJsonArray{QStringLiteral("general:beach")});
    QVERIFY(imageUpdated.first().first().value<Image>().tags.contains(QStringLiteral("general:sunset")));
    QVERIFY(!m_editor->hasChanges());
    // The images as they are now: all have it
    QCOMPARE(tagsShown(*m_editor).value(QStringLiteral("general:sunset")), TagEditor::Change::Kept);
}

void TestBulkTagEditor::setsAValueForAll()
{
    selectImages(threeImages().mid(0, 2));  // both photos

    QVERIFY(m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("artwork")));

    const QVariantMap category = rowOf(*m_editor, QStringLiteral("category"));
    QCOMPARE(category.value(QStringLiteral("value")).toString(), QStringLiteral("artwork"));
    QVERIFY(category.value(QStringLiteral("changed")).toBool());
    QCOMPARE(tagsShown(*m_editor).value(QStringLiteral("category:photo")), TagEditor::Change::Removed);
}

void TestBulkTagEditor::removesAValueFromAll()
{
    selectThreeImages();

    QVERIFY(m_editor->setSingleValue(QStringLiteral("category"), QString()));
    m_editor->apply();

    QTRY_COMPARE(m_server->requestsTo("POST", bulkUrl).size(), 1);
    const QJsonArray removed =
        bodyOf(m_server->requestsTo("POST", bulkUrl).first()).value(QStringLiteral("remove_tags")).toArray();
    QCOMPARE(removed, QJsonArray({QStringLiteral("category:photo"), QStringLiteral("category:artwork")}));
}

void TestBulkTagEditor::keepsAValueAsIs()
{
    selectThreeImages();
    m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("artwork"));

    m_editor->keepValue(QStringLiteral("category"));

    QVERIFY(!m_editor->hasChanges());
    QVERIFY(!rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("changed")).toBool());
}

void TestBulkTagEditor::addsNothingEveryImageHas()
{
    m_editor->setImages(threeImages().mid(0, 2));  // both have beach

    QVERIFY(m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach")));

    QVERIFY(!m_editor->hasChanges());
}

// "general:nude,photo": only the first image is a photo, and that is enough to refuse.
void TestBulkTagEditor::refusesATagThatWouldRestrictAnyImage()
{
    selectThreeImages();

    QVERIFY(!m_editor->addTag(QStringLiteral("general"), QStringLiteral("nude")));

    QVERIFY(m_editor->errorMessage().contains(QStringLiteral("general:nude, general:photo")));
    QVERIFY(!m_editor->hasChanges());
}

void TestBulkTagEditor::refusesMoreImagesThanTheServerTakes()
{
    QList<Image> tooMany;
    for (int id = 1; id <= BulkTagEditor::maximumImageCount + 1; ++id)
    {
        tooMany.append(imageWith(id, {QStringLiteral("general:beach")}));
    }

    m_editor->setImages(tooMany);
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("sunset"));
    m_editor->apply();

    QVERIFY(!m_editor->errorMessage().isEmpty());  // the numbers are written as the user's language does
    QVERIFY(m_server->requestsTo("POST", bulkUrl).isEmpty());
    QVERIFY(m_editor->hasChanges());
}

// The changes apply to whatever is selected on Apply.
void TestBulkTagEditor::keepsTheChangesWhenTheSelectionChanges()
{
    selectThreeImages();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("sunset"));

    m_editor->setImages(threeImages().mid(2));
    m_editor->apply();

    QTRY_COMPARE(m_server->requestsTo("POST", bulkUrl).size(), 1);
    QCOMPARE(bodyOf(m_server->requestsTo("POST", bulkUrl).first()).value(QStringLiteral("image_ids")).toArray(),
             QJsonArray{3});
}

void TestBulkTagEditor::clearDropsTheChanges()
{
    selectThreeImages();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("sunset"));
    m_editor->setTypedName(QStringLiteral("general"), QStringLiteral("sea"));

    m_editor->clear();

    QVERIFY(!m_editor->hasChanges());
    QVERIFY(m_editor->typedName(QStringLiteral("general")).isEmpty());
}

void TestBulkTagEditor::appliesTypedNamesToo()
{
    selectThreeImages();
    m_editor->setTypedName(QStringLiteral("general"), QStringLiteral("Sea"));
    QVERIFY(m_editor->hasChanges());

    m_editor->apply();

    QTRY_COMPARE(m_server->requestsTo("POST", bulkUrl).size(), 1);
    QCOMPARE(bodyOf(m_server->requestsTo("POST", bulkUrl).first()).value(QStringLiteral("add_tags")).toArray(),
             QJsonArray{QStringLiteral("general:sea")});
}

void TestBulkTagEditor::keepsTheChangesWhenTheServerRefuses()
{
    m_server->answerWith("POST", bulkUrl, [](const HttpRequest&) { return fakes::problem(404, "not_found"); });
    selectThreeImages();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("sunset"));

    m_editor->apply();

    QTRY_VERIFY(!m_editor->errorMessage().isEmpty());
    QVERIFY(!m_editor->isApplying());
    QVERIFY(m_editor->hasChanges());
}

}  // namespace photobook::tag_editor

QTEST_GUILESS_MAIN(photobook::tag_editor::TestBulkTagEditor)

#include "TestBulkTagEditor.moc"
