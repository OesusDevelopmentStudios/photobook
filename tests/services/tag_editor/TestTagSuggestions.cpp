/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <memory>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrlQuery>

#include "Connection.hpp"
#include "ServerClientFactory.hpp"
#include "SettingsFactory.hpp"
#include "TagEditorFactory.hpp"
#include "TagSuggestions.hpp"
#include "fakes/FakeServer.hpp"
#include "fakes/FakeTokenStore.hpp"
#include "fakes/ImageJson.hpp"

namespace pictorio::tag_editor
{

namespace
{

using connection::Connection;
using fakes::FakeServer;
using server_client::HttpRequest;

const QString serverUrl = QStringLiteral("https://photos.example.org");
const QString tagsUrl = serverUrl + QStringLiteral("/api/v1/tags");

QString prefixOf(const HttpRequest& request)
{
    return QUrlQuery(request.url).queryItemValue(QStringLiteral("prefix"), QUrl::FullyDecoded);
}

// The server's answer: tags that start with the prefix, most used first. "su" and "sum" find different
// tags, so a test can tell the answers apart; "many" finds more than are shown.
server_client::HttpResponse tagsStartingWith(const QString& prefix)
{
    QStringList names;
    if (prefix == QLatin1String("su"))
    {
        names = {QStringLiteral("general:sunset"), QStringLiteral("general:summer")};
    }
    else if (prefix == QLatin1String("sum"))
    {
        names = {QStringLiteral("general:summer"), QStringLiteral("album:summer_2025")};
    }
    else if (prefix == QLatin1String("many"))
    {
        for (int number = 0; number < 20; ++number)
        {
            names.append(QStringLiteral("general:many_%1").arg(number));
        }
    }

    QJsonArray items;
    int imageCount = 100;
    for (const QString& name : names)
    {
        items.append(QJsonObject{{QStringLiteral("tag"), name}, {QStringLiteral("image_count"), imageCount--}});
    }

    return fakes::jsonResponse(200, {{QStringLiteral("items"), items}});
}

}  // namespace

class TestTagSuggestions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void asksOnceTypingPauses();
    void givesTheTagsMostUsedFirst();
    void clearsAtOnceWhenTheFieldIsEmptied();
    void leavesOutExcludedTags();
    void showsAtMostTen();
    void ignoresTheAnswerToOlderTyping();
    void showsNothingWhenTheServerFails();

private:
    QStringList shownTags() const;

    std::unique_ptr<QTemporaryDir> m_folder;
    std::unique_ptr<settings::Settings> m_settings;
    std::shared_ptr<FakeServer> m_server;
    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<TagSuggestions> m_suggestions;
};

// Logged in from the last run
void TestTagSuggestions::init()
{
    m_suggestions.reset();
    m_folder = std::make_unique<QTemporaryDir>();
    m_settings = settings::createSettings(m_folder->filePath(QStringLiteral("pictoriorc")));
    m_settings->setServerUrl(QUrl(serverUrl));
    m_server = std::make_shared<FakeServer>();
    m_server->answer("GET",
                     serverUrl + QStringLiteral("/api/v1/auth/me"),
                     fakes::jsonResponse(200,
                                         {{QStringLiteral("id"), 2},
                                          {QStringLiteral("username"), QStringLiteral("kid")},
                                          {QStringLiteral("role"), QStringLiteral("user")},
                                          {QStringLiteral("restricted_tags"), QJsonArray()}}));
    m_server->answerWith(
        "GET", tagsUrl, [](const HttpRequest& request) { return tagsStartingWith(prefixOf(request)); });

    auto keyring = std::make_shared<fakes::Keyring>();
    keyring->tokens.insert(serverUrl, QStringLiteral("pk_kept"));
    auto serverClientFactory = [server = m_server](const QUrl& url) {
        return server_client::createServerClient(url, server);
    };
    m_connection = std::make_unique<Connection>(
        serverClientFactory, std::make_unique<fakes::FakeTokenStore>(keyring), m_settings.get());
    m_connection->restoreLogin();
    QTRY_COMPARE(m_connection->state(), Connection::State::LoggedIn);
    m_suggestions = createTagSuggestions(m_connection.get());
}

QStringList TestTagSuggestions::shownTags() const
{
    QStringList tags;
    for (int row = 0; row < m_suggestions->rowCount(); ++row)
    {
        tags.append(m_suggestions->data(m_suggestions->index(row), TagSuggestions::TagRole).toString());
    }

    return tags;
}

void TestTagSuggestions::asksOnceTypingPauses()
{
    m_suggestions->setTypedText(QStringLiteral("s"));
    m_suggestions->setTypedText(QStringLiteral("su"));
    m_suggestions->setTypedText(QStringLiteral("sum"));
    QVERIFY(m_server->requestsTo("GET", tagsUrl).isEmpty());  // not while typing

    QTRY_COMPARE(m_suggestions->count(), 2);
    const QList<HttpRequest> asked = m_server->requestsTo("GET", tagsUrl);
    QCOMPARE(asked.size(), 1);
    QCOMPARE(prefixOf(asked.first()), QStringLiteral("sum"));
}

void TestTagSuggestions::givesTheTagsMostUsedFirst()
{
    m_suggestions->setTypedText(QStringLiteral("sum"));

    QTRY_COMPARE(shownTags(), QStringList({QStringLiteral("general:summer"), QStringLiteral("album:summer_2025")}));
    QCOMPARE(m_suggestions->data(m_suggestions->index(0), TagSuggestions::ImageCountRole).toInt(), 100);
    QCOMPARE(m_suggestions->roleNames().value(TagSuggestions::TagRole), QByteArray("tag"));
    QCOMPARE(m_suggestions->tagAt(1), QStringLiteral("album:summer_2025"));
    QVERIFY(m_suggestions->tagAt(2).isEmpty());
}

void TestTagSuggestions::clearsAtOnceWhenTheFieldIsEmptied()
{
    m_suggestions->setTypedText(QStringLiteral("sum"));
    QTRY_COMPARE(m_suggestions->count(), 2);

    m_suggestions->setTypedText(QStringLiteral("  "));

    QCOMPARE(m_suggestions->count(), 0);
    QTest::qWait(TagSuggestions::typingPause.count() * 2);
    QCOMPARE(m_server->requestsTo("GET", tagsUrl).size(), 1);  // the server is not asked for nothing
}

// e.g. the tags the image already has
void TestTagSuggestions::leavesOutExcludedTags()
{
    m_suggestions->setExcludedTags({QStringLiteral("general:summer")});

    m_suggestions->setTypedText(QStringLiteral("sum"));

    QTRY_COMPARE(shownTags(), QStringList({QStringLiteral("album:summer_2025")}));
}

void TestTagSuggestions::showsAtMostTen()
{
    m_suggestions->setTypedText(QStringLiteral("many"));

    QTRY_COMPARE(m_suggestions->count(), TagSuggestions::maximumCount);
    QCOMPARE(shownTags().first(), QStringLiteral("general:many_0"));  // the most used ones
}

void TestTagSuggestions::ignoresTheAnswerToOlderTyping()
{
    m_server->holdAnswers = true;
    m_suggestions->setTypedText(QStringLiteral("su"));
    QTRY_COMPARE(m_server->requestsTo("GET", tagsUrl).size(), 1);
    m_suggestions->setTypedText(QStringLiteral("sum"));
    QTRY_COMPARE(m_server->requestsTo("GET", tagsUrl).size(), 2);

    m_server->releaseAnswer(1);  // the newer answer first…
    m_server->releaseAnswers();  // …and the older one after it

    QTRY_COMPARE(m_suggestions->count(), 2);
    QTest::qWait(50);
    QCOMPARE(shownTags(), QStringList({QStringLiteral("general:summer"), QStringLiteral("album:summer_2025")}));
}

void TestTagSuggestions::showsNothingWhenTheServerFails()
{
    m_suggestions->setTypedText(QStringLiteral("sum"));
    QTRY_COMPARE(m_suggestions->count(), 2);
    m_server->answerWith("GET", tagsUrl, [](const HttpRequest&) { return fakes::problem(500, "internal"); });

    m_suggestions->setTypedText(QStringLiteral("summ"));

    QTRY_COMPARE(m_suggestions->count(), 0);
}

}  // namespace pictorio::tag_editor

QTEST_GUILESS_MAIN(pictorio::tag_editor::TestTagSuggestions)

#include "TestTagSuggestions.moc"
