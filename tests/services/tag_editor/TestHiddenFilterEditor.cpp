/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <KLocalizedString>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "Connection.hpp"
#include "HiddenFilterEditor.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
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

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString preferencesUrl = serverUrl + QStringLiteral("/api/v1/preferences");

QJsonObject namespaceJson(const QString& name, bool singleValue)
{
    return {
        {QStringLiteral("name"), name},
        {QStringLiteral("single_value"), singleValue},
        {QStringLiteral("is_builtin"), true},
        {QStringLiteral("in_path_template"), false},
        {QStringLiteral("user_editable"), true},
        {QStringLiteral("allowed_values"), QJsonValue::Null},
    };
}

server_client::HttpResponse preferencesJson(const QStringList& hiddenFilters)
{
    return fakes::jsonResponse(200, {{QStringLiteral("hidden_filters"), QJsonArray::fromStringList(hiddenFilters)}});
}

// The filters as the chips show them: "filter" kept, "+filter" added, "-filter" removed
QStringList shownFilters(const HiddenFilterEditor& editor)
{
    QStringList shown;
    for (const QVariant& entry : editor.filters())
    {
        const QVariantMap filter = entry.toMap();
        const auto change = static_cast<TagRowsEditor::Change>(filter.value(QStringLiteral("change")).toInt());
        const QString prefix = change == TagRowsEditor::Change::Added     ? QStringLiteral("+")
                               : change == TagRowsEditor::Change::Removed ? QStringLiteral("-")
                                                                          : QString();
        shown.append(prefix + filter.value(QStringLiteral("tag")).toString());
    }

    return shown;
}

}  // namespace

// The user's hidden filters. The server has "rating:private" and "category:refs" saved.
class TestHiddenFilterEditor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();

    void loadsTheSavedFilters();
    void addsAFilter();
    void acceptsAnyOneSearchTerm();
    void refusesWhatIsNotOneSearchTerm_data();
    void refusesWhatIsNotOneSearchTerm();
    void refusesMoreFiltersThanTheServerKeeps();
    void addsARepeatOnlyOnce();
    void removesAndRestoresAFilter();
    void dropsAnAddedFilterOnRemove();
    void savesTheFiltersAfterTheChanges();
    void addsWhatIsTypedWhenSaving();
    void savesNothingIfWhatIsTypedIsRefused();
    void keepsTheChangesIfTheServerRefuses();
    void changesNothingWhileSaving();
    void cancelDropsTheChanges();
    void loadDropsUnsavedChanges();

private:
    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<HiddenFilterEditor> m_editor;
};

void TestHiddenFilterEditor::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
}

// Logged in from the last run, the filters and namespaces loaded
void TestHiddenFilterEditor::init()
{
    m_editor.reset();
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("photobookrc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/auth/me"),
                     fakes::jsonResponse(200,
                                         {{QStringLiteral("id"), 2},
                                          {QStringLiteral("username"), QStringLiteral("restrictedUser")},
                                          {QStringLiteral("role"), QStringLiteral("user")},
                                          {QStringLiteral("restricted_tags"), QJsonArray()}}));
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/namespaces"),
                     fakes::jsonResponse(200,
                                         {{QStringLiteral("items"),
                                           QJsonArray{namespaceJson(QStringLiteral("category"), true),
                                                      namespaceJson(QStringLiteral("rating"), true),
                                                      namespaceJson(QStringLiteral("character"), false),
                                                      namespaceJson(QStringLiteral("general"), false)}}}));
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/tags"),
                     fakes::jsonResponse(200, {{QStringLiteral("items"), QJsonArray()}}));
    m_server->answer(
        "GET", preferencesUrl, preferencesJson({QStringLiteral("rating:private"), QStringLiteral("category:refs")}));
    // The server keeps what it is sent.
    m_server->answerWith("PUT", preferencesUrl, [](const server_client::HttpRequest& request) {
        return fakes::jsonResponse(200, QJsonDocument::fromJson(request.body).object());
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
    m_editor = createHiddenFilterEditor(m_connection.get());
    m_editor->load();
    QTRY_VERIFY(!m_editor->isLoading());
}

void TestHiddenFilterEditor::loadsTheSavedFilters()
{
    QCOMPARE(shownFilters(*m_editor), QStringList({QStringLiteral("rating:private"), QStringLiteral("category:refs")}));
    QVERIFY(!m_editor->hasChanges());
    QVERIFY(!m_editor->isBusy());
    QVERIFY(m_editor->errorMessage().isEmpty());
}

void TestHiddenFilterEditor::addsAFilter()
{
    QSignalSpy hasChangesChanged(m_editor.get(), &HiddenFilterEditor::hasChangesChanged);

    QVERIFY(m_editor->addTag({}, QStringLiteral(" character:sample_hero ")));

    QCOMPARE(shownFilters(*m_editor),
             QStringList({QStringLiteral("rating:private"),
                          QStringLiteral("category:refs"),
                          QStringLiteral("+character:sample_hero")}));
    QVERIFY(m_editor->hasChanges());
    QCOMPARE(hasChangesChanged.size(), 1);
    QVERIFY(m_server->requestsTo("PUT", preferencesUrl).isEmpty());  // only Save sends
}

void TestHiddenFilterEditor::acceptsAnyOneSearchTerm()
{
    const QStringList terms{QStringLiteral("rating:private|restricted"),
                            QStringLiteral("character:*"),
                            QStringLiteral("general:nude,photo"),
                            QStringLiteral("(rating:private,general:photo)|category:refs")};
    for (const QString& term : terms)
    {
        QVERIFY2(m_editor->addTag({}, term), qPrintable(m_editor->errorMessage()));
    }

    QCOMPARE(m_editor->filtersAfterChanges(),
             QStringList({QStringLiteral("rating:private"), QStringLiteral("category:refs")}) + terms);
}

void TestHiddenFilterEditor::refusesWhatIsNotOneSearchTerm_data()
{
    QTest::addColumn<QString>("typed");

    QTest::newRow("two terms") << QStringLiteral("rating:private category:refs");
    QTest::newRow("empty operand") << QStringLiteral("general:a,,b");
    QTest::newRow("unbalanced") << QStringLiteral("(rating:private|category:refs");
    QTest::newRow("leading minus") << QStringLiteral("-rating:private");
    QTest::newRow("too long") << QStringLiteral("general:") + QString(HiddenFilterEditor::maximumLength, u'a');
}

void TestHiddenFilterEditor::refusesWhatIsNotOneSearchTerm()
{
    QFETCH(QString, typed);

    QVERIFY(!m_editor->addTag({}, typed));

    QVERIFY(!m_editor->errorMessage().isEmpty());
    QVERIFY(!m_editor->hasChanges());
}

void TestHiddenFilterEditor::refusesMoreFiltersThanTheServerKeeps()
{
    for (int number = 2; number < HiddenFilterEditor::maximumCount; ++number)
    {
        QVERIFY(m_editor->addTag({}, QStringLiteral("general:tag%1").arg(number)));
    }

    QVERIFY(!m_editor->addTag({}, QStringLiteral("general:one_more")));

    QVERIFY(!m_editor->errorMessage().isEmpty());
    QCOMPARE(m_editor->filtersAfterChanges().size(), HiddenFilterEditor::maximumCount);
}

void TestHiddenFilterEditor::addsARepeatOnlyOnce()
{
    QVERIFY(m_editor->addTag({}, QStringLiteral("rating:private")));  // saved already
    QVERIFY(!m_editor->hasChanges());

    QVERIFY(m_editor->addTag({}, QStringLiteral("character:*")));
    QVERIFY(m_editor->addTag({}, QStringLiteral("character:*")));

    QCOMPARE(
        m_editor->filtersAfterChanges(),
        QStringList({QStringLiteral("rating:private"), QStringLiteral("category:refs"), QStringLiteral("character:*")}));
}

void TestHiddenFilterEditor::removesAndRestoresAFilter()
{
    QVERIFY(m_editor->removeTag(QStringLiteral("rating:private")));

    QCOMPARE(shownFilters(*m_editor), QStringList({QStringLiteral("-rating:private"), QStringLiteral("category:refs")}));
    QCOMPARE(m_editor->filtersAfterChanges(), QStringList({QStringLiteral("category:refs")}));
    QVERIFY(m_editor->hasChanges());

    QVERIFY(m_editor->restoreTag(QStringLiteral("rating:private")));

    QCOMPARE(shownFilters(*m_editor), QStringList({QStringLiteral("rating:private"), QStringLiteral("category:refs")}));
    QVERIFY(!m_editor->hasChanges());

    // Typing a removed filter again keeps it, too.
    QVERIFY(m_editor->removeTag(QStringLiteral("category:refs")));
    QVERIFY(m_editor->addTag({}, QStringLiteral("category:refs")));
    QVERIFY(!m_editor->hasChanges());
}

void TestHiddenFilterEditor::dropsAnAddedFilterOnRemove()
{
    QVERIFY(m_editor->addTag({}, QStringLiteral("character:*")));

    QVERIFY(m_editor->removeTag(QStringLiteral("character:*")));

    QCOMPARE(shownFilters(*m_editor), QStringList({QStringLiteral("rating:private"), QStringLiteral("category:refs")}));
    QVERIFY(!m_editor->hasChanges());
}

void TestHiddenFilterEditor::savesTheFiltersAfterTheChanges()
{
    QSignalSpy saved(m_editor.get(), &HiddenFilterEditor::saved);
    QVERIFY(m_editor->removeTag(QStringLiteral("rating:private")));
    QVERIFY(m_editor->addTag({}, QStringLiteral("rating:private|restricted")));

    m_editor->save();

    QTRY_COMPARE(saved.size(), 1);
    const QList<server_client::HttpRequest> puts = m_server->requestsTo("PUT", preferencesUrl);
    QCOMPARE(puts.size(), 1);  // all changes at once
    QCOMPARE(QJsonDocument::fromJson(puts.first().body).object().value(QStringLiteral("hidden_filters")).toArray(),
             QJsonArray({QStringLiteral("category:refs"), QStringLiteral("rating:private|restricted")}));
    QCOMPARE(shownFilters(*m_editor),
             QStringList({QStringLiteral("category:refs"), QStringLiteral("rating:private|restricted")}));
    QVERIFY(!m_editor->hasChanges());
    QVERIFY(!m_editor->isBusy());
}

void TestHiddenFilterEditor::addsWhatIsTypedWhenSaving()
{
    QSignalSpy typedNamesChanged(m_editor.get(), &HiddenFilterEditor::typedNamesChanged);
    m_editor->setTypedName({}, QStringLiteral("character:*"));
    QVERIFY(m_editor->hasChanges());  // typed counts

    m_editor->save();

    QTRY_COMPARE(m_server->requestsTo("PUT", preferencesUrl).size(), 1);
    QTRY_COMPARE(
        m_editor->filtersAfterChanges(),
        QStringList({QStringLiteral("rating:private"), QStringLiteral("category:refs"), QStringLiteral("character:*")}));
    QVERIFY(m_editor->typedName({}).isEmpty());
    QVERIFY(typedNamesChanged.size() > 0);  // the field empties
}

void TestHiddenFilterEditor::savesNothingIfWhatIsTypedIsRefused()
{
    QVERIFY(m_editor->removeTag(QStringLiteral("rating:private")));
    m_editor->setTypedName({}, QStringLiteral("rating:private category:refs"));

    m_editor->save();

    QVERIFY(!m_editor->errorMessage().isEmpty());
    QCOMPARE(m_editor->typedName({}), QStringLiteral("rating:private category:refs"));  // stays, to correct
    QVERIFY(m_server->requestsTo("PUT", preferencesUrl).isEmpty());
}

void TestHiddenFilterEditor::keepsTheChangesIfTheServerRefuses()
{
    m_server->answerWith("PUT", preferencesUrl, [](const server_client::HttpRequest&) {
        return fakes::problem(422, "validation_error");
    });
    QVERIFY(m_editor->addTag({}, QStringLiteral("character:*")));

    m_editor->save();

    QTRY_VERIFY(!m_editor->errorMessage().isEmpty());
    QVERIFY(!m_editor->isBusy());
    QVERIFY(m_editor->hasChanges());
    QCOMPARE(m_editor->filtersAfterChanges().last(), QStringLiteral("character:*"));
}

void TestHiddenFilterEditor::changesNothingWhileSaving()
{
    m_server->holdAnswers = true;
    QVERIFY(m_editor->addTag({}, QStringLiteral("character:*")));
    m_editor->save();
    QVERIFY(m_editor->isSaving());
    QVERIFY(m_editor->isBusy());

    QVERIFY(!m_editor->addTag({}, QStringLiteral("general:beach")));
    QVERIFY(!m_editor->removeTag(QStringLiteral("rating:private")));
    m_editor->cancel();
    QVERIFY(m_editor->hasChanges());

    m_server->releaseAnswers();
    QTRY_VERIFY(!m_editor->isSaving());
    QCOMPARE(
        m_editor->filtersAfterChanges(),
        QStringList({QStringLiteral("rating:private"), QStringLiteral("category:refs"), QStringLiteral("character:*")}));
}

void TestHiddenFilterEditor::cancelDropsTheChanges()
{
    QVERIFY(m_editor->addTag({}, QStringLiteral("character:*")));
    QVERIFY(m_editor->removeTag(QStringLiteral("rating:private")));
    m_editor->setTypedName({}, QStringLiteral("general:bea"));
    QVERIFY(!m_editor->addTag({}, QStringLiteral("-rating:private")));  // leaves a message

    m_editor->cancel();

    QCOMPARE(shownFilters(*m_editor), QStringList({QStringLiteral("rating:private"), QStringLiteral("category:refs")}));
    QVERIFY(!m_editor->hasChanges());
    QVERIFY(m_editor->typedName({}).isEmpty());
    QVERIFY(m_editor->errorMessage().isEmpty());
    QVERIFY(m_server->requestsTo("PUT", preferencesUrl).isEmpty());
}

void TestHiddenFilterEditor::loadDropsUnsavedChanges()
{
    m_server->answer("GET", preferencesUrl, preferencesJson({QStringLiteral("general:spoiler")}));
    QVERIFY(m_editor->addTag({}, QStringLiteral("character:*")));

    m_editor->load();

    QVERIFY(!m_editor->hasChanges());
    QTRY_COMPARE(shownFilters(*m_editor), QStringList({QStringLiteral("general:spoiler")}));
}

}  // namespace photobook::tag_editor

QTEST_GUILESS_MAIN(photobook::tag_editor::TestHiddenFilterEditor)

#include "TestHiddenFilterEditor.moc"
