/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrlQuery>

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

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString imageUrl = serverUrl + QStringLiteral("/api/v1/media/5");
const QString countsUrl = imageUrl + QStringLiteral("/tag-counts");

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

// A photo of the library, as the search gave it
server_client::Image beachPhoto()
{
    server_client::Image image;
    image.id = 5;
    image.originalFilename = QStringLiteral("beach.jpg");
    image.takenAt = QDateTime(QDate(2025, 7, 14), QTime(18, 3));
    image.tags = {QStringLiteral("category:photo"), QStringLiteral("general:summer"), QStringLiteral("meta:jpeg")};
    return image;
}

// A namespace's row in the edit view, by role name, e.g. row["value"]
QVariantMap rowOf(const TagEditor& editor, const QString& namespaceName)
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

// The namespaces of the rows, in their order
QStringList namespacesShown(const TagEditor& editor)
{
    const QAbstractListModel* rows = editor.namespaceRows();
    QStringList namespaces;
    for (int row = 0; row < rows->rowCount(); ++row)
    {
        namespaces.append(rows->index(row).data(rows->roleNames().key("namespaceName")).toString());
    }

    return namespaces;
}

// The image count shown with a tag; -1 while unknown, -2 if the tag is not shown
int countShown(const TagEditor& editor, const QString& tagText)
{
    const QAbstractListModel* rows = editor.namespaceRows();
    const int tagsRole = rows->roleNames().key("tags");
    for (int row = 0; row < rows->rowCount(); ++row)
    {
        for (const QVariant& entry : rows->index(row).data(tagsRole).toList())
        {
            const QVariantMap tag = entry.toMap();
            if (tag.value(QStringLiteral("tag")).toString() == tagText)
            {
                return tag.value(QStringLiteral("count")).toInt();
            }
        }
    }

    return -2;
}

// What the edit view shows of each tag: {tag: change}, e.g. {"general:beach": Added}
QHash<QString, TagEditor::Change> changesShown(const TagEditor& editor)
{
    QHash<QString, TagEditor::Change> changes;
    const QAbstractListModel* rows = editor.namespaceRows();
    const int tagsRole = rows->roleNames().key("tags");
    for (int row = 0; row < rows->rowCount(); ++row)
    {
        for (const QVariant& entry : rows->index(row).data(tagsRole).toList())
        {
            const QVariantMap tag = entry.toMap();
            changes.insert(tag.value(QStringLiteral("tag")).toString(),
                           static_cast<TagEditor::Change>(tag.value(QStringLiteral("change")).toInt()));
        }
    }

    return changes;
}

}  // namespace

// The user "restrictedUser" is restricted from rating:private, and from images that are both nude and a photo. The server
// has a custom one-tag namespace "weather", and the ratings public and private.
class TestTagEditor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void needsALibraryImage();
    void showsTheImagesTagsByNamespace();
    void putsTheServersOwnNamespacesLast();
    void showsANamespaceTheServerHasNotListed();
    void showsEachTagsImageCountOnceKnown();
    void asksForTheCountsOncePerImage();
    void asksForNewCountsAfterASave();
    void keepsTheCountsWhileEditing();
    void showsARowPerNamespace();
    void collectsChangesUntilSaved();
    void savesAllChangesInOnePatch();
    void cancelDropsTheChanges();
    void tidiesTypedNames();
    void refusesAnInvalidName();
    void refusesARestrictedTag();
    void refusesARestrictedCombination();
    void dropsAnAddedTagWhenRemoved();

    void setsTheValueOfAOneTagNamespace();
    void setsNoValue();
    void keepsTheImagesOwnValue();
    void offersTheAllowedRatings();
    void refusesAValueThatIsNotAllowed();
    void offersTheKnownValuesOfAOneTagNamespace();
    void givesACustomNamespaceNoDropdown();

    void countsATypedNameAsAChange();
    void savesTheTypedNamesToo();
    void doesNotSaveARefusedTypedName();
    void cancelClearsTheTypedNames();
    void keepsTheRowsWhileTagsChange();

    void keepsTheChangesWhenTheServerRefuses();
    void anotherImageEndsEditing();

private:
    void startEditingTheBeachPhoto();

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<TagEditor> m_editor;
};

void TestTagEditor::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
}

// Logged in from the last run
void TestTagEditor::init()
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
                                         namespaceJson(QStringLiteral("author"), false),
                                         namespaceJson(QStringLiteral("rating"),
                                                       true,
                                                       QJsonArray{QStringLiteral("public"), QStringLiteral("private")}),
                                         namespaceJson(QStringLiteral("general"), false),
                                         namespaceJson(QStringLiteral("weather"), true),
                                         namespaceJson(QStringLiteral("meta"), false)}}}));
    // The categories in use, most used first
    m_server->answerWith("GET", serverUrl + QStringLiteral("/api/v1/tags"), [](const HttpRequest& request) {
        QJsonArray items;
        if (QUrlQuery(request.url).queryItemValue(QStringLiteral("namespace")) == QLatin1String("category"))
        {
            items = {QJsonObject{{QStringLiteral("tag"), QStringLiteral("category:photo")},
                                 {QStringLiteral("media_count"), 9}},
                     QJsonObject{{QStringLiteral("tag"), QStringLiteral("category:artwork")},
                                 {QStringLiteral("media_count"), 4}}};
        }

        return fakes::jsonResponse(200, {{QStringLiteral("items"), items}});
    });
    const auto tagCount = [](const char* tag, int imageCount) {
        return QJsonObject{{QStringLiteral("tag"), QString::fromLatin1(tag)},
                           {QStringLiteral("media_count"), imageCount}};
    };
    m_server->answer("GET",
                     countsUrl,
                     fakes::jsonResponse(200,
                                         {{QStringLiteral("items"),
                                           QJsonArray{tagCount("category:photo", 1200),
                                                      tagCount("general:summer", 412),
                                                      tagCount("meta:jpeg", 9800)}}}));
    m_server->answerWith("PATCH", imageUrl, [](const HttpRequest&) {
        QJsonObject image = fakes::imageJson(5);
        image[QStringLiteral("tags")] = QJsonArray{QStringLiteral("category:photo"), QStringLiteral("general:sunset")};
        return fakes::jsonResponse(200, image);
    });

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
    m_editor = createTagEditor(m_connection.get());
}

// The fake server answers at once, so the namespaces and known values are in when this returns.
void TestTagEditor::startEditingTheBeachPhoto()
{
    m_editor->setImage(beachPhoto());
    m_editor->startEditing();
    QVERIFY(m_editor->isEditing());
    QTRY_VERIFY(!rowOf(*m_editor, QStringLiteral("weather")).isEmpty());
}

void TestTagEditor::needsALibraryImage()
{
    m_editor->setImage(std::nullopt);  // e.g. a local file

    m_editor->startEditing();

    QVERIFY(!m_editor->canEdit());
    QVERIFY(!m_editor->isEditing());
    QCOMPARE(m_editor->namespaceRows()->rowCount(), 0);
}

// For the tag pane: the namespaces the image has tags in, in the server's order
void TestTagEditor::showsTheImagesTagsByNamespace()
{
    m_editor->setImage(beachPhoto());

    QTRY_COMPARE(namespacesShown(*m_editor),
                 QStringList({QStringLiteral("category"), QStringLiteral("general"), QStringLiteral("meta")}));
    QVERIFY(!m_editor->isEditing());
    QCOMPARE(changesShown(*m_editor).value(QStringLiteral("general:summer")), TagEditor::Change::Kept);
}

// Those only the server sets come after the user's, wherever the server lists them.
void TestTagEditor::putsTheServersOwnNamespacesLast()
{
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/namespaces"),
                     fakes::jsonResponse(200,
                                         {{QStringLiteral("items"),
                                           QJsonArray{namespaceJson(QStringLiteral("meta"), false),
                                                      namespaceJson(QStringLiteral("general"), false),
                                                      namespaceJson(QStringLiteral("category"), true)}}}));
    m_editor = createTagEditor(m_connection.get());

    m_editor->setImage(beachPhoto());

    QTRY_COMPARE(namespacesShown(*m_editor),
                 QStringList({QStringLiteral("general"), QStringLiteral("category"), QStringLiteral("meta")}));
}

// e.g. one added on the server since the list was read: still shown, before meta
void TestTagEditor::showsANamespaceTheServerHasNotListed()
{
    server_client::Image image = beachPhoto();
    image.tags.append(QStringLiteral("mood:calm"));

    m_editor->setImage(image);

    QTRY_COMPARE(
        namespacesShown(*m_editor),
        QStringList(
            {QStringLiteral("category"), QStringLiteral("general"), QStringLiteral("mood"), QStringLiteral("meta")}));
}

void TestTagEditor::showsEachTagsImageCountOnceKnown()
{
    m_server->holdAnswers = true;
    m_editor->setImage(beachPhoto());
    QTRY_COMPARE(m_server->requestsTo("GET", countsUrl).size(), 1);
    QCOMPARE(countShown(*m_editor, QStringLiteral("general:summer")), -1);  // the names come first

    m_server->releaseAnswers();

    QTRY_COMPARE(countShown(*m_editor, QStringLiteral("general:summer")), 412);
    QCOMPARE(countShown(*m_editor, QStringLiteral("meta:jpeg")), 9800);
}

// Stepping back to an image shows its counts at once.
void TestTagEditor::asksForTheCountsOncePerImage()
{
    m_editor->setImage(beachPhoto());
    QTRY_COMPARE(countShown(*m_editor, QStringLiteral("category:photo")), 1200);
    server_client::Image another = beachPhoto();
    another.id = 6;
    m_editor->setImage(another);

    m_editor->setImage(beachPhoto());

    QCOMPARE(countShown(*m_editor, QStringLiteral("category:photo")), 1200);
    QCOMPARE(m_server->requestsTo("GET", countsUrl).size(), 1);
}

// A tag added or removed changes the counts.
void TestTagEditor::asksForNewCountsAfterASave()
{
    m_editor->setImage(beachPhoto());
    QTRY_COMPARE(m_server->requestsTo("GET", countsUrl).size(), 1);
    m_editor->startEditing();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("sunset"));

    m_editor->save();

    QTRY_VERIFY(!m_editor->isEditing());
    QTRY_COMPARE(m_server->requestsTo("GET", countsUrl).size(), 2);
}

// The image's own tags keep their counts; an added one has none yet.
void TestTagEditor::keepsTheCountsWhileEditing()
{
    m_editor->setImage(beachPhoto());
    QTRY_COMPARE(countShown(*m_editor, QStringLiteral("general:summer")), 412);

    startEditingTheBeachPhoto();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("sunset"));

    QCOMPARE(countShown(*m_editor, QStringLiteral("general:summer")), 412);
    QCOMPARE(countShown(*m_editor, QStringLiteral("meta:jpeg")), 9800);
    QCOMPARE(countShown(*m_editor, QStringLiteral("general:sunset")), -1);
}

// Also namespaces the image has no tag in, in the server's order; meta, whose tags the server sets, last and without
// controls
void TestTagEditor::showsARowPerNamespace()
{
    startEditingTheBeachPhoto();

    const QAbstractListModel* rows = m_editor->namespaceRows();
    QStringList namespaces;
    for (int row = 0; row < rows->rowCount(); ++row)
    {
        namespaces.append(rows->index(row).data(rows->roleNames().key("namespaceName")).toString());
    }

    QCOMPARE(namespaces,
             QStringList({QStringLiteral("category"),
                          QStringLiteral("author"),
                          QStringLiteral("rating"),
                          QStringLiteral("general"),
                          QStringLiteral("weather"),
                          QStringLiteral("meta")}));
    QVERIFY(rowOf(*m_editor, QStringLiteral("general")).value(QStringLiteral("editable")).toBool());
    QVERIFY(!rowOf(*m_editor, QStringLiteral("meta")).value(QStringLiteral("editable")).toBool());
    QCOMPARE(rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("value")).toString(),
             QStringLiteral("photo"));
    QVERIFY(rowOf(*m_editor, QStringLiteral("author")).value(QStringLiteral("tags")).toList().isEmpty());
}

void TestTagEditor::collectsChangesUntilSaved()
{
    startEditingTheBeachPhoto();

    QVERIFY(m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach")));
    QVERIFY(m_editor->removeTag(QStringLiteral("general:summer")));

    QVERIFY(m_editor->hasChanges());
    QVERIFY(m_server->requestsTo("PATCH", imageUrl).isEmpty());
    const QHash<QString, TagEditor::Change> shown = changesShown(*m_editor);
    QCOMPARE(shown.value(QStringLiteral("category:photo")), TagEditor::Change::Kept);
    QCOMPARE(shown.value(QStringLiteral("general:beach")), TagEditor::Change::Added);
    QCOMPARE(shown.value(QStringLiteral("general:summer")), TagEditor::Change::Removed);
}

void TestTagEditor::savesAllChangesInOnePatch()
{
    startEditingTheBeachPhoto();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("sunset"));
    m_editor->removeTag(QStringLiteral("general:summer"));
    QSignalSpy saved(m_editor.get(), &TagEditor::saved);

    m_editor->save();

    QTRY_COMPARE(saved.count(), 1);
    const QList<HttpRequest> patches = m_server->requestsTo("PATCH", imageUrl);
    QCOMPARE(patches.size(), 1);
    const QJsonObject body = QJsonDocument::fromJson(patches.first().body).object();
    QCOMPARE(body.value(QStringLiteral("add_tags")).toArray(), QJsonArray{QStringLiteral("general:sunset")});
    QCOMPARE(body.value(QStringLiteral("remove_tags")).toArray(), QJsonArray{QStringLiteral("general:summer")});
    const auto image = saved.first().first().value<server_client::Image>();
    QCOMPARE(image.tags, QStringList({QStringLiteral("category:photo"), QStringLiteral("general:sunset")}));
    QVERIFY(!m_editor->isEditing());
    QVERIFY(!m_editor->hasChanges());
}

void TestTagEditor::cancelDropsTheChanges()
{
    startEditingTheBeachPhoto();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach"));

    m_editor->cancel();

    QVERIFY(!m_editor->isEditing());
    QVERIFY(!m_editor->hasChanges());
    QVERIFY(m_server->requestsTo("PATCH", imageUrl).isEmpty());
}

// The namespace is the row's: no prefix needed, and a ':' is part of the name.
void TestTagEditor::tidiesTypedNames()
{
    startEditingTheBeachPhoto();

    m_editor->addTag(QStringLiteral("author"), QStringLiteral(" Sample Artist "));
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("artist:example"));

    const QHash<QString, TagEditor::Change> shown = changesShown(*m_editor);
    QCOMPARE(shown.value(QStringLiteral("author:sample_artist")), TagEditor::Change::Added);
    QCOMPARE(shown.value(QStringLiteral("general:artist:example")), TagEditor::Change::Added);
}

void TestTagEditor::refusesAnInvalidName()
{
    startEditingTheBeachPhoto();

    QVERIFY(!m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach,sand")));

    QVERIFY(!m_editor->errorMessage().isEmpty());
    QVERIFY(!m_editor->hasChanges());
}

void TestTagEditor::refusesARestrictedTag()
{
    startEditingTheBeachPhoto();

    QVERIFY(!m_editor->setSingleValue(QStringLiteral("rating"), QStringLiteral("private")));

    QVERIFY(m_editor->errorMessage().contains(QStringLiteral("rating:private")));
    QVERIFY(!m_editor->hasChanges());
    QVERIFY(m_server->requestsTo("PATCH", imageUrl).isEmpty());
}

// "general:nude,photo": each tag alone is allowed, both together are not.
void TestTagEditor::refusesARestrictedCombination()
{
    startEditingTheBeachPhoto();

    QVERIFY(m_editor->addTag(QStringLiteral("general"), QStringLiteral("photo")));
    QVERIFY(!m_editor->addTag(QStringLiteral("general"), QStringLiteral("nude")));

    QVERIFY(m_editor->errorMessage().contains(QStringLiteral("general:nude, general:photo")));
    QVERIFY(!changesShown(*m_editor).contains(QStringLiteral("general:nude")));
}

void TestTagEditor::dropsAnAddedTagWhenRemoved()
{
    startEditingTheBeachPhoto();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach"));

    m_editor->removeTag(QStringLiteral("general:beach"));

    QVERIFY(!changesShown(*m_editor).contains(QStringLiteral("general:beach")));
    QVERIFY(!m_editor->hasChanges());
}

void TestTagEditor::setsTheValueOfAOneTagNamespace()
{
    startEditingTheBeachPhoto();

    QVERIFY(m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("artwork")));
    QVariantMap category = rowOf(*m_editor, QStringLiteral("category"));
    QCOMPARE(category.value(QStringLiteral("value")).toString(), QStringLiteral("artwork"));
    QVERIFY(category.value(QStringLiteral("changed")).toBool());
    QCOMPARE(changesShown(*m_editor).value(QStringLiteral("category:photo")), TagEditor::Change::Removed);

    QVERIFY(m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("photo")));  // back to the image's
    category = rowOf(*m_editor, QStringLiteral("category"));
    QCOMPARE(category.value(QStringLiteral("value")).toString(), QStringLiteral("photo"));
    QVERIFY(!category.value(QStringLiteral("changed")).toBool());
    QVERIFY(!m_editor->hasChanges());
}

void TestTagEditor::setsNoValue()
{
    startEditingTheBeachPhoto();

    QVERIFY(m_editor->setSingleValue(QStringLiteral("category"), QString()));

    QVERIFY(rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("value")).toString().isEmpty());
    QCOMPARE(changesShown(*m_editor).value(QStringLiteral("category:photo")), TagEditor::Change::Removed);
}

void TestTagEditor::keepsTheImagesOwnValue()
{
    startEditingTheBeachPhoto();
    m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("artwork"));

    m_editor->keepValue(QStringLiteral("category"));

    QCOMPARE(rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("value")).toString(),
             QStringLiteral("photo"));
    QVERIFY(!m_editor->hasChanges());
}

void TestTagEditor::offersTheAllowedRatings()
{
    startEditingTheBeachPhoto();

    const QVariantMap rating = rowOf(*m_editor, QStringLiteral("rating"));

    QVERIFY(rating.value(QStringLiteral("fixedValues")).toBool());
    QCOMPARE(rating.value(QStringLiteral("options")).toStringList(),
             QStringList({QStringLiteral("public"), QStringLiteral("private")}));
}

void TestTagEditor::refusesAValueThatIsNotAllowed()
{
    startEditingTheBeachPhoto();

    QVERIFY(!m_editor->setSingleValue(QStringLiteral("rating"), QStringLiteral("maybe")));

    QVERIFY(m_editor->errorMessage().contains(QStringLiteral("public, private")));
}

// Pick one of those in use, or type a new one
void TestTagEditor::offersTheKnownValuesOfAOneTagNamespace()
{
    startEditingTheBeachPhoto();

    QTRY_COMPARE(rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("options")).toStringList(),
                 QStringList({QStringLiteral("photo"), QStringLiteral("artwork")}));
    QVERIFY(!rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("fixedValues")).toBool());
    QVERIFY(m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("screenshot")));
}

// A custom namespace gets an add field even if it holds one tag; a new tag replaces the one there.
void TestTagEditor::givesACustomNamespaceNoDropdown()
{
    startEditingTheBeachPhoto();

    QVERIFY(!rowOf(*m_editor, QStringLiteral("weather")).value(QStringLiteral("dropdown")).toBool());
    QVERIFY(rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("dropdown")).toBool());
    m_editor->addTag(QStringLiteral("weather"), QStringLiteral("rain"));
    m_editor->addTag(QStringLiteral("weather"), QStringLiteral("sun"));

    QVERIFY(!changesShown(*m_editor).contains(QStringLiteral("weather:rain")));
    QCOMPARE(changesShown(*m_editor).value(QStringLiteral("weather:sun")), TagEditor::Change::Added);
    for (const HttpRequest& request : m_server->requestsTo("GET", serverUrl + QStringLiteral("/api/v1/tags")))
    {
        QVERIFY(QUrlQuery(request.url).queryItemValue(QStringLiteral("namespace")) != QLatin1String("weather"));
    }
}

// Typed, not added yet: going on would lose it as well.
void TestTagEditor::countsATypedNameAsAChange()
{
    startEditingTheBeachPhoto();
    QSignalSpy hasChangesChanged(m_editor.get(), &TagEditor::hasChangesChanged);

    m_editor->setTypedName(QStringLiteral("author"), QStringLiteral("example"));

    QVERIFY(m_editor->hasChanges());
    QCOMPARE(hasChangesChanged.count(), 1);
    QCOMPARE(m_editor->suggestions()->namespaceName(), QStringLiteral("author"));  // suggestions for that row
}

void TestTagEditor::savesTheTypedNamesToo()
{
    startEditingTheBeachPhoto();
    m_editor->removeTag(QStringLiteral("general:summer"));
    m_editor->setTypedName(QStringLiteral("general"), QStringLiteral("Sunset"));
    QSignalSpy saved(m_editor.get(), &TagEditor::saved);

    m_editor->save();

    QTRY_COMPARE(saved.count(), 1);
    const QJsonObject body = QJsonDocument::fromJson(m_server->requestsTo("PATCH", imageUrl).first().body).object();
    QCOMPARE(body.value(QStringLiteral("add_tags")).toArray(), QJsonArray{QStringLiteral("general:sunset")});
    QVERIFY(m_editor->typedName(QStringLiteral("general")).isEmpty());
}

void TestTagEditor::doesNotSaveARefusedTypedName()
{
    startEditingTheBeachPhoto();
    m_editor->setTypedName(QStringLiteral("general"), QStringLiteral("beach,sand"));

    m_editor->save();

    QVERIFY(!m_editor->errorMessage().isEmpty());
    QVERIFY(m_editor->isEditing());
    QVERIFY(m_server->requestsTo("PATCH", imageUrl).isEmpty());
    QCOMPARE(m_editor->typedName(QStringLiteral("general")), QStringLiteral("beach,sand"));  // to correct it
}

void TestTagEditor::cancelClearsTheTypedNames()
{
    startEditingTheBeachPhoto();
    m_editor->setTypedName(QStringLiteral("general"), QStringLiteral("beach"));
    QSignalSpy typedNamesChanged(m_editor.get(), &TagEditor::typedNamesChanged);

    m_editor->cancel();

    QVERIFY(m_editor->typedName(QStringLiteral("general")).isEmpty());
    QCOMPARE(typedNamesChanged.count(), 1);  // so the field empties
    QVERIFY(!m_editor->hasChanges());
}

// A field the user types in keeps its focus: its row stays, only its content changes.
void TestTagEditor::keepsTheRowsWhileTagsChange()
{
    startEditingTheBeachPhoto();
    QSignalSpy reset(m_editor->namespaceRows(), &QAbstractItemModel::modelReset);
    QSignalSpy dataChanged(m_editor->namespaceRows(), &QAbstractItemModel::dataChanged);

    m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach"));

    QCOMPARE(reset.count(), 0);
    QCOMPARE(dataChanged.count(), 1);  // only the general row
}

// e.g. a restriction the app did not know of yet
void TestTagEditor::keepsTheChangesWhenTheServerRefuses()
{
    m_server->answerWith("PATCH", imageUrl, [](const HttpRequest&) {
        return fakes::jsonResponse(403,
                                   {{QStringLiteral("type"), QStringLiteral("about:blank")},
                                    {QStringLiteral("title"), QStringLiteral("Tag not allowed")},
                                    {QStringLiteral("status"), 403},
                                    {QStringLiteral("code"), QStringLiteral("tag_restricted")},
                                    {QStringLiteral("tags"), QJsonArray{QStringLiteral("general:beach")}}});
    });
    startEditingTheBeachPhoto();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach"));

    m_editor->save();

    QTRY_VERIFY(m_editor->errorMessage().contains(QStringLiteral("general:beach")));
    QVERIFY(!m_editor->isSaving());
    QVERIFY(m_editor->isEditing());
    QVERIFY(m_editor->hasChanges());
}

void TestTagEditor::anotherImageEndsEditing()
{
    startEditingTheBeachPhoto();
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach"));
    server_client::Image another = beachPhoto();
    another.id = 6;

    m_editor->setImage(another);

    QVERIFY(!m_editor->isEditing());
    QVERIFY(!m_editor->hasChanges());
}

}  // namespace photobook::tag_editor

QTEST_GUILESS_MAIN(photobook::tag_editor::TestTagEditor)

#include "TestTagEditor.moc"
