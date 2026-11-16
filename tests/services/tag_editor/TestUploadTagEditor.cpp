/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "TagEditorFactory.hpp"
#include "UploadTagEditor.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/ImageJson.hpp"

namespace pictorio::tag_editor
{

namespace
{

using connection::Connection;
using fakes::FakeServer;

const QString serverUrl = QStringLiteral("https://photos.example.org");

QJsonObject namespaceJson(const QString& name, bool singleValue, const QJsonValue& allowedValues = QJsonValue::Null)
{
    return {
        {QStringLiteral("name"), name},
        {QStringLiteral("single_value"), singleValue},
        {QStringLiteral("is_builtin"), true},
        {QStringLiteral("in_path_template"), name == QLatin1String("category")},
        {QStringLiteral("allowed_values"), allowedValues},
    };
}

// A namespace's row, by role name, e.g. row["value"]
QVariantMap rowOf(const UploadTagEditor& editor, const QString& namespaceName)
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

}  // namespace

// The tags of an upload. The user "kid" is restricted from rating:nsfw, and from images that are both nude and a
// photo.
class TestUploadTagEditor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void startsWithoutTags();
    void collectsTheTagsForEveryFile();
    void replacesTheValueOfAOneTagNamespace();
    void setsNoValue();
    void removesATag();
    void refusesARestrictedTag();
    void refusesARestrictedCombination();
    void refusesAValueThatIsNotAllowed();
    void addsWhatIsTyped();
    void givesNothingIfWhatIsTypedIsRefused();
    void keepsTheTagsWhileBusy();
    void clearDropsTheTags();

private:
    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<UploadTagEditor> m_editor;
};

void TestUploadTagEditor::initTestCase()
{
    KLocalizedString::setApplicationDomain("pictorio");
}

// Logged in from the last run, the namespaces loaded
void TestUploadTagEditor::init()
{
    m_editor.reset();
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("pictoriorc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    m_server->answer(
        "GET",
        serverUrl + QStringLiteral("/api/v1/auth/me"),
        fakes::jsonResponse(200,
                            {{QStringLiteral("id"), 2},
                             {QStringLiteral("username"), QStringLiteral("kid")},
                             {QStringLiteral("role"), QStringLiteral("user")},
                             {QStringLiteral("restricted_tags"),
                              QJsonArray{QStringLiteral("rating:nsfw"), QStringLiteral("general:nude,photo")}}}));
    m_server->answer(
        "GET",
        serverUrl + QStringLiteral("/api/v1/namespaces"),
        fakes::jsonResponse(
            200,
            {{QStringLiteral("items"),
              QJsonArray{namespaceJson(QStringLiteral("category"), true),
                         namespaceJson(QStringLiteral("author"), false),
                         namespaceJson(
                             QStringLiteral("rating"), true, QJsonArray{QStringLiteral("sfw"), QStringLiteral("nsfw")}),
                         namespaceJson(QStringLiteral("general"), false)}}}));
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/tags"),
                     fakes::jsonResponse(200, {{QStringLiteral("items"), QJsonArray()}}));

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
    m_editor = createUploadTagEditor(m_connection.get());
    m_editor->load();
    QTRY_COMPARE(rowOf(*m_editor, QStringLiteral("rating")).value(QStringLiteral("options")).toStringList(),
                 QStringList({QStringLiteral("sfw"), QStringLiteral("nsfw")}));
}

void TestUploadTagEditor::startsWithoutTags()
{
    const QVariantMap category = rowOf(*m_editor, QStringLiteral("category"));

    QVERIFY(category.value(QStringLiteral("dropdown")).toBool());
    QVERIFY(!category.value(QStringLiteral("keepable")).toBool());  // new files: nothing to keep
    QVERIFY(category.value(QStringLiteral("value")).toString().isEmpty());
    QVERIFY(!m_editor->hasChanges());
    QCOMPARE(m_editor->tagsToUpload(), QStringList());
}

void TestUploadTagEditor::collectsTheTagsForEveryFile()
{
    QSignalSpy hasChangesChanged(m_editor.get(), &UploadTagEditor::hasChangesChanged);

    QVERIFY(m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("photo")));
    QVERIFY(m_editor->addTag(QStringLiteral("author"), QStringLiteral("Sample Artist")));
    QVERIFY(m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach")));

    QCOMPARE(m_editor->tagsToUpload(),
             QStringList({QStringLiteral("category:photo"),
                          QStringLiteral("author:sample_artist"),
                          QStringLiteral("general:beach")}));
    QVERIFY(m_editor->hasChanges());
    QCOMPARE(hasChangesChanged.count(), 1);
    QCOMPARE(rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("value")).toString(),
             QStringLiteral("photo"));
}

void TestUploadTagEditor::replacesTheValueOfAOneTagNamespace()
{
    m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("photo"));

    m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("artwork"));

    QCOMPARE(m_editor->tagsToUpload(), QStringList{QStringLiteral("category:artwork")});
}

void TestUploadTagEditor::setsNoValue()
{
    m_editor->setSingleValue(QStringLiteral("category"), QStringLiteral("photo"));

    QVERIFY(m_editor->setSingleValue(QStringLiteral("category"), QString()));

    QCOMPARE(m_editor->tagsToUpload(), QStringList());
    QVERIFY(!rowOf(*m_editor, QStringLiteral("category")).value(QStringLiteral("changed")).toBool());
}

void TestUploadTagEditor::removesATag()
{
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach"));

    QVERIFY(m_editor->removeTag(QStringLiteral("general:beach")));

    QVERIFY(!m_editor->hasChanges());
}

void TestUploadTagEditor::refusesARestrictedTag()
{
    QVERIFY(!m_editor->setSingleValue(QStringLiteral("rating"), QStringLiteral("nsfw")));

    QVERIFY(m_editor->errorMessage().contains(QStringLiteral("rating:nsfw")));
    QCOMPARE(m_editor->tagsToUpload(), QStringList());
}

// "general:nude,photo": each tag alone is allowed, both together are not.
void TestUploadTagEditor::refusesARestrictedCombination()
{
    QVERIFY(m_editor->addTag(QStringLiteral("general"), QStringLiteral("photo")));
    QVERIFY(!m_editor->addTag(QStringLiteral("general"), QStringLiteral("nude")));

    QVERIFY(m_editor->errorMessage().contains(QStringLiteral("general:nude, general:photo")));
}

void TestUploadTagEditor::refusesAValueThatIsNotAllowed()
{
    QVERIFY(!m_editor->setSingleValue(QStringLiteral("rating"), QStringLiteral("maybe")));

    QVERIFY(m_editor->errorMessage().contains(QStringLiteral("sfw, nsfw")));
}

// Typed, not added yet: sent as well, as the user expects
void TestUploadTagEditor::addsWhatIsTyped()
{
    m_editor->setTypedName(QStringLiteral("general"), QStringLiteral("Sunset"));
    QVERIFY(m_editor->hasChanges());

    QCOMPARE(m_editor->tagsToUpload(), QStringList{QStringLiteral("general:sunset")});
    QVERIFY(m_editor->typedName(QStringLiteral("general")).isEmpty());
}

void TestUploadTagEditor::givesNothingIfWhatIsTypedIsRefused()
{
    m_editor->setTypedName(QStringLiteral("general"), QStringLiteral("beach,sand"));

    QCOMPARE(m_editor->tagsToUpload(), std::nullopt);
    QVERIFY(!m_editor->errorMessage().isEmpty());
    QCOMPARE(m_editor->typedName(QStringLiteral("general")), QStringLiteral("beach,sand"));  // to correct it
}

void TestUploadTagEditor::keepsTheTagsWhileBusy()
{
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach"));
    QSignalSpy busyChanged(m_editor.get(), &UploadTagEditor::busyChanged);

    m_editor->setBusy(true);

    QVERIFY(m_editor->isBusy());
    QCOMPARE(busyChanged.count(), 1);
    QVERIFY(!m_editor->addTag(QStringLiteral("general"), QStringLiteral("sea")));
    QVERIFY(!m_editor->removeTag(QStringLiteral("general:beach")));
    QCOMPARE(m_editor->tagsToUpload(), QStringList{QStringLiteral("general:beach")});
}

void TestUploadTagEditor::clearDropsTheTags()
{
    m_editor->addTag(QStringLiteral("general"), QStringLiteral("beach"));
    m_editor->setTypedName(QStringLiteral("author"), QStringLiteral("kid"));

    m_editor->clear();

    QVERIFY(!m_editor->hasChanges());
    QVERIFY(m_editor->typedName(QStringLiteral("author")).isEmpty());
}

}  // namespace pictorio::tag_editor

QTEST_GUILESS_MAIN(pictorio::tag_editor::TestUploadTagEditor)

#include "TestUploadTagEditor.moc"
