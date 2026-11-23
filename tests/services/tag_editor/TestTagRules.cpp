/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include <KLocalizedString>
#include <QTest>

#include "hlp/Restrictions.hpp"
#include "hlp/SearchOperand.hpp"
#include "hlp/SearchTerm.hpp"
#include "hlp/TagNames.hpp"

namespace photobook::tag_editor
{

namespace
{

const QSet<QString> namespaces{
    QStringLiteral("category"),
    QStringLiteral("author"),
    QStringLiteral("character"),
    QStringLiteral("rating"),
    QStringLiteral("album"),
    QStringLiteral("general"),
};

// '^' marks the cursor in a search written for a test: the search without it, and where it was
std::pair<QString, qsizetype> searchAndCursor(const QString& marked)
{
    return {QString(marked).remove(QLatin1Char('^')), marked.indexOf(QLatin1Char('^'))};
}

// Tags as the server writes them, e.g. "rating:private"
QList<TagName> tagsFrom(const QStringList& texts)
{
    QList<TagName> tags;
    for (const QString& text : texts)
    {
        tags.append(tagFromServer(text));
    }

    return tags;
}

}  // namespace

// The rules a tag change is checked against before it is sent: the same as the server's tag names and search
// terms.
class TestTagRules : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void normalisesTypedTags_data();
    void normalisesTypedTags();
    void refusesInvalidNames_data();
    void refusesInvalidNames();
    void countsCharactersNotBytes();
    void tidiesANameTypedForANamespace();
    void readsTagsAsTheServerWritesThem();
    void worksOutTheTagsAfterAChange();
    void replacesTheTagOfASingleValueNamespace();

    void matchesTerms_data();
    void matchesTerms();
    void matchesDates_data();
    void matchesDates();
    void refusesInvalidTerms_data();
    void refusesInvalidTerms();
    void namesTheTagsATermMentions();
    void namesTheRestrictedTagsOfAnImage();

    void findsTheOperandUnderTheCursor_data();
    void findsTheOperandUnderTheCursor();
    void findsNoOperandWhereNoTagIsTyped_data();
    void findsNoOperandWhereNoTagIsTyped();
    void putsAChosenTagInPlaceOfTheOperand_data();
    void putsAChosenTagInPlaceOfTheOperand();
};

void TestTagRules::initTestCase()
{
    KLocalizedString::setApplicationDomain("photobook");
}

void TestTagRules::normalisesTypedTags_data()
{
    QTest::addColumn<QString>("typed");
    QTest::addColumn<QString>("tag");

    QTest::newRow("capitals") << QStringLiteral("Summer") << QStringLiteral("general:summer");
    QTest::newRow("spaces") << QStringLiteral("  On the   Beach ") << QStringLiteral("general:on_the_beach");
    QTest::newRow("a colon is part of the name") << QStringLiteral("re:zero") << QStringLiteral("general:re:zero");
    QTest::newRow("unicode") << QStringLiteral("ĄĘ") << QStringLiteral("general:ąę");
    QTest::newRow("no case") << QStringLiteral("日本") << QStringLiteral("general:日本");
}

void TestTagRules::normalisesTypedTags()
{
    QFETCH(QString, typed);
    QFETCH(QString, tag);

    const std::expected<TagName, QString> result = tagFromTypedName(generalNamespace, typed);

    QVERIFY(result.has_value());
    QCOMPARE(result->text(), tag);
}

void TestTagRules::refusesInvalidNames_data()
{
    QTest::addColumn<QString>("typed");

    QTest::newRow("empty") << QString();
    QTest::newRow("only spaces") << QStringLiteral("   ");
    QTest::newRow("dot") << QStringLiteral(".");
    QTest::newRow("two dots") << QStringLiteral("..");
    QTest::newRow("slash") << QStringLiteral("a/b");
    QTest::newRow("backslash") << QStringLiteral("a\\b");
    QTest::newRow("comma") << QStringLiteral("a,b");
    QTest::newRow("bar") << QStringLiteral("a|b");
    QTest::newRow("parenthesis") << QStringLiteral("(a)");
    QTest::newRow("star") << QStringLiteral("a*");
    QTest::newRow("leading minus") << QStringLiteral("-a");
    QTest::newRow("tab") << QStringLiteral("a\tb");
    QTest::newRow("too long") << QString(101, QLatin1Char('a'));
}

void TestTagRules::refusesInvalidNames()
{
    QFETCH(QString, typed);

    const std::expected<TagName, QString> result = tagFromTypedName(generalNamespace, typed);

    QVERIFY(!result.has_value());
    QVERIFY(!result.error().isEmpty());  // says why
}

// The server counts characters, not UTF-16 units: 100 characters outside the BMP are allowed.
void TestTagRules::countsCharactersNotBytes()
{
    QString longest;
    for (int count = 0; count < 100; ++count)
    {
        longest += QStringLiteral("😀");
    }

    QVERIFY(tagFromTypedName(generalNamespace, QString(100, QLatin1Char('a'))).has_value());
    QVERIFY(tagFromTypedName(generalNamespace, longest).has_value());
    QVERIFY(!tagFromTypedName(generalNamespace, longest + QStringLiteral("😀")).has_value());
}

// In a namespace's own row: the namespace is the row's, and a ':' is part of the name.
void TestTagRules::tidiesANameTypedForANamespace()
{
    QCOMPARE(tagFromTypedName(QStringLiteral("author"), QStringLiteral(" Sample Artist "))->text(),
             QStringLiteral("author:sample_artist"));
    QCOMPARE(tagFromTypedName(QStringLiteral("general"), QStringLiteral("artist:example"))->text(),
             QStringLiteral("general:artist:example"));
    QVERIFY(!tagFromTypedName(QStringLiteral("author"), QStringLiteral("a,b")).has_value());
}

void TestTagRules::readsTagsAsTheServerWritesThem()
{
    QCOMPARE(tagFromServer(QStringLiteral("category:photo")),
             (TagName{QStringLiteral("category"), QStringLiteral("photo")}));
    QCOMPARE(tagFromServer(QStringLiteral("general:re:zero")),
             (TagName{QStringLiteral("general"), QStringLiteral("re:zero")}));
}

void TestTagRules::worksOutTheTagsAfterAChange()
{
    const QList<TagName> current = tagsFrom({QStringLiteral("category:photo"), QStringLiteral("general:summer")});

    const QList<TagName> after =
        tagsAfterChange(current,
                        tagsFrom({QStringLiteral("general:beach"), QStringLiteral("general:summer")}),
                        tagsFrom({QStringLiteral("general:summer")}),
                        {QStringLiteral("category")});

    // Removed, then added again: kept. Added: at the end.
    QCOMPARE(
        after,
        tagsFrom(
            {QStringLiteral("category:photo"), QStringLiteral("general:beach"), QStringLiteral("general:summer")}));
}

void TestTagRules::replacesTheTagOfASingleValueNamespace()
{
    const QList<TagName> current = tagsFrom({QStringLiteral("category:photo"), QStringLiteral("author:restrictedUser")});
    const QSet<QString> singleValue{QStringLiteral("category")};

    const QList<TagName> newCategory =
        tagsAfterChange(current, tagsFrom({QStringLiteral("category:artwork")}), {}, singleValue);
    const QList<TagName> secondAuthor =
        tagsAfterChange(current, tagsFrom({QStringLiteral("author:normalUser")}), {}, singleValue);

    QCOMPARE(newCategory, tagsFrom({QStringLiteral("author:restrictedUser"), QStringLiteral("category:artwork")}));
    QCOMPARE(secondAuthor,
             tagsFrom({QStringLiteral("category:photo"), QStringLiteral("author:restrictedUser"), QStringLiteral("author:normalUser")}));
}

void TestTagRules::matchesTerms_data()
{
    QTest::addColumn<QString>("term");
    QTest::addColumn<QStringList>("tags");
    QTest::addColumn<bool>("matches");

    const auto row = [](const char* name, const char* term, const QStringList& tags, bool matches) {
        QTest::newRow(name) << QString::fromUtf8(term) << tags << matches;
    };

    // Restricted terms
    row("a tag", "rating:private", {QStringLiteral("rating:private")}, true);
    row("another tag", "rating:private", {QStringLiteral("rating:public")}, false);
    row("either tag", "rating:private|restricted", {QStringLiteral("rating:restricted")}, true);
    row("any of a namespace", "character:*", {QStringLiteral("character:sample_hero")}, true);
    row("namespace is not a name", "character:*", {QStringLiteral("general:character")}, false);
    row("name starting with", "author:sample*", {QStringLiteral("author:sample_artist")}, true);
    row("name not starting with", "author:sample*", {QStringLiteral("author:other_artist")}, false);
    row("one of both", "general:nude,photo", {QStringLiteral("general:nude")}, false);
    row("both", "general:nude,photo", {QStringLiteral("general:nude"), QStringLiteral("general:photo")}, true);

    // Groups, negation and the default namespace
    row("general by default", "tag1,tag2", {QStringLiteral("general:tag1"), QStringLiteral("general:tag2")}, true);
    row("a group",
        "general:(tag_1,tag_2)|(tag_3,tag_4)",
        {QStringLiteral("general:tag_3"), QStringLiteral("general:tag_4")},
        true);
    row("not a whole group",
        "general:(tag_1,tag_2)|(tag_3,tag_4)",
        {QStringLiteral("general:tag_1"), QStringLiteral("general:tag_3")},
        false);
    row("own prefix wins", "category:artwork|author:x", {QStringLiteral("author:x")}, true);
    row("negated term", "-rating:private|restricted", {QStringLiteral("rating:public")}, true);
    row("negated term, matching", "-rating:private|restricted", {QStringLiteral("rating:private")}, false);
    row("negated operand",
        "(category:photo,-rating:private)|category:artwork",
        {QStringLiteral("category:photo"), QStringLiteral("rating:public")},
        true);
    row("negated operand, matching",
        "(category:photo,-rating:private)|category:artwork",
        {QStringLiteral("category:photo"), QStringLiteral("rating:private")},
        false);
    row("other side of the or",
        "(category:photo,-rating:private)|category:artwork",
        {QStringLiteral("category:artwork"), QStringLiteral("rating:private")},
        true);

    // Prefixes
    row("not a namespace", "re:zero", {QStringLiteral("general:re:zero")}, true);
    row("prefix in capitals", "Rating:private", {QStringLiteral("rating:private")}, true);
}

void TestTagRules::matchesTerms()
{
    QFETCH(QString, term);
    QFETCH(QStringList, tags);
    QFETCH(bool, matches);

    const std::optional<SearchTerm> searchTerm = SearchTerm::parse(term, namespaces);

    QVERIFY(searchTerm.has_value());
    QCOMPARE(searchTerm->matches(tagsFrom(tags), QDateTime()), matches);
}

void TestTagRules::matchesDates_data()
{
    QTest::addColumn<QString>("term");
    QTest::addColumn<QDateTime>("takenAt");
    QTest::addColumn<bool>("matches");

    const QDateTime summer(QDate(2025, 7, 14), QTime(18, 3));
    QTest::newRow("year") << QStringLiteral("year:2025") << summer << true;
    QTest::newRow("another year") << QStringLiteral("year:2024|2026") << summer << false;
    QTest::newRow("month") << QStringLiteral("month:2025-07") << summer << true;
    QTest::newRow("last day of a month") << QStringLiteral("month:2025-07")
                                         << QDateTime(QDate(2025, 7, 31), QTime(23, 59)) << true;
    QTest::newRow("range") << QStringLiteral("date:2025-07-01..2025-07-14") << summer << true;
    QTest::newRow("range before") << QStringLiteral("date:2025-07-15..2025-08-01") << summer << false;
    QTest::newRow("date not known") << QStringLiteral("year:2025") << QDateTime() << false;
}

void TestTagRules::matchesDates()
{
    QFETCH(QString, term);
    QFETCH(QDateTime, takenAt);
    QFETCH(bool, matches);

    const std::optional<SearchTerm> searchTerm = SearchTerm::parse(term, namespaces);

    QVERIFY(searchTerm.has_value());
    QCOMPARE(searchTerm->matches({}, takenAt), matches);
}

void TestTagRules::refusesInvalidTerms_data()
{
    QTest::addColumn<QString>("term");

    QTest::newRow("empty") << QString();
    QTest::newRow("two terms") << QStringLiteral("a b");
    QTest::newRow("empty operand") << QStringLiteral("a,,b");
    QTest::newRow("open group") << QStringLiteral("(a");
    QTest::newRow("closing too many") << QStringLiteral("a)");
    QTest::newRow("star inside") << QStringLiteral("a*b");
    QTest::newRow("not a year") << QStringLiteral("year:20x5");
    QTest::newRow("no such month") << QStringLiteral("month:2025-13");
    QTest::newRow("no such day") << QStringLiteral("date:2025-02-30");
    QTest::newRow("range backwards") << QStringLiteral("date:2025-07-14..2025-07-01");
}

void TestTagRules::refusesInvalidTerms()
{
    QFETCH(QString, term);

    QVERIFY(!SearchTerm::parse(term, namespaces).has_value());
}

// So a refusal can say which of the image's tags an admin restricted
void TestTagRules::namesTheTagsATermMentions()
{
    const QList<TagName> tags =
        tagsFrom({QStringLiteral("general:nude"), QStringLiteral("general:photo"), QStringLiteral("category:artwork")});

    const std::optional<SearchTerm> both = SearchTerm::parse(QStringLiteral("general:nude,photo"), namespaces);
    const std::optional<SearchTerm> anyCharacter = SearchTerm::parse(QStringLiteral("character:*"), namespaces);

    QCOMPARE(both->mentionedAmong(tags), tagsFrom({QStringLiteral("general:nude"), QStringLiteral("general:photo")}));
    QVERIFY(anyCharacter->mentionedAmong(tags).isEmpty());
}

// Each restriction that applies names the image's tags it mentions; restrictions that do not apply say nothing.
void TestTagRules::namesTheRestrictedTagsOfAnImage()
{
    const QStringList restrictions{
        QStringLiteral("rating:private"), QStringLiteral("general:nude,photo"), QStringLiteral("character:*")};
    const QList<TagName> image =
        tagsFrom({QStringLiteral("rating:private"), QStringLiteral("general:photo"), QStringLiteral("general:nude")});

    QCOMPARE(
        restrictedTagsAmong(restrictions, namespaces, image, QDateTime()),
        QStringList({QStringLiteral("general:nude"), QStringLiteral("general:photo"), QStringLiteral("rating:private")}));
    QVERIFY(restrictedTagsAmong(restrictions, namespaces, tagsFrom({QStringLiteral("general:photo")}), QDateTime())
                .isEmpty());
}

// The tag being typed in a search, for suggestions: the grammar's examples
void TestTagRules::findsTheOperandUnderTheCursor_data()
{
    QTest::addColumn<QString>("search");         // '^' is the cursor
    QTest::addColumn<QString>("typed");          // up to the cursor
    QTest::addColumn<QString>("operand");        // all of it, as a choice replaces it
    QTest::addColumn<QString>("namespaceName");  // in force; empty: none
    QTest::addColumn<bool>("endsTerm");

    const auto row = [](const char* search, const char* typed, const char* operand, const char* ns, bool endsTerm) {
        QTest::newRow(search) << QString::fromUtf8(search) << QString::fromUtf8(typed) << QString::fromUtf8(operand)
                              << QString::fromUtf8(ns) << endsTerm;
    };
    row("summ^", "summ", "summ", "", true);
    row("category:photo summ^", "summ", "summ", "", true);
    row("-su^", "su", "su", "", true);
    row("sci-f^", "sci-f", "sci-f", "", true);  // a '-' inside a name is part of it
    row("re:ze^", "re:ze", "re:ze", "", true);  // "re" is no namespace
    row("rating^", "rating", "rating", "", true);
    row("rating:^", "", "", "rating", true);
    row("Rating:p^", "p", "p", "rating", true);
    row("author:sa^", "sa", "sa", "author", true);
    row("weather:su^", "su", "su", "weather", true);     // a custom namespace
    row("rating:private|p^", "p", "p", "rating", true);  // the term's prefix holds for all of it
    row("-rating:private|-p^", "p", "p", "rating", true);
    row("category:photo,-sun^", "sun", "sun", "category", true);
    row("rating:(private|p^", "p", "p", "rating", true);  // a group still open
    row("(author:sam|be^", "be", "be", "", true);         // an operand's prefix holds for it alone
    row("(author:sam|be^)", "be", "be", "", false);
    row("author:(sam|weather:su^)", "su", "su", "weather", false);
    row("rating:(private|public),sun^", "sun", "sun", "rating", true);
    row("su^mmer|beach", "su", "summer", "", false);
    row("summer ^", "", "", "", true);
    row("summer ^beach", "", "beach", "", true);
    row("category:photo ^summer", "", "summer", "", true);
}

void TestTagRules::findsTheOperandUnderTheCursor()
{
    QFETCH(QString, search);
    QFETCH(QString, typed);
    QFETCH(QString, operand);
    QFETCH(QString, namespaceName);
    QFETCH(bool, endsTerm);
    const auto [text, cursor] = searchAndCursor(search);

    const std::optional<SearchOperand> found =
        searchOperandAt(text, cursor, namespaces + QSet<QString>{QStringLiteral("weather")});

    QVERIFY(found.has_value());
    QCOMPARE(found->typed, typed);
    QCOMPARE(text.mid(found->start, found->end - found->start), operand);
    QCOMPARE(found->namespaceName, namespaceName);
    QCOMPARE(found->endsTerm, endsTerm);
}

void TestTagRules::findsNoOperandWhereNoTagIsTyped_data()
{
    QTest::addColumn<QString>("search");

    QTest::newRow("a year") << QStringLiteral("year:20^");
    QTest::newRow("a date") << QStringLiteral("summer date:2025-07^");
    QTest::newRow("a month in a group") << QStringLiteral("(year:2025|month:2025-0^");
    QTest::newRow("a wildcard") << QStringLiteral("sum*^");
    QTest::newRow("a wildcard after the cursor") << QStringLiteral("su^m*");
}

void TestTagRules::findsNoOperandWhereNoTagIsTyped()
{
    QFETCH(QString, search);
    const auto [text, cursor] = searchAndCursor(search);

    QCOMPARE(searchOperandAt(text, cursor, namespaces), std::nullopt);
}

void TestTagRules::putsAChosenTagInPlaceOfTheOperand_data()
{
    QTest::addColumn<QString>("search");  // '^' is the cursor
    QTest::addColumn<QString>("tag");
    QTest::addColumn<QString>("completed");

    const auto row = [](const char* name, const char* search, const char* tag, const char* completed) {
        QTest::newRow(name) << QString::fromUtf8(search) << QString::fromUtf8(tag) << QString::fromUtf8(completed);
    };
    row("a general tag, bare", "summ^", "general:summer", "summer ^");
    row("after other terms", "category:photo -summ^", "general:summer", "category:photo -summer ^");
    row("another namespace, whole", "priv^", "rating:private", "rating:private ^");
    row("the prefix's namespace, bare", "rating:private|p^", "rating:public", "rating:private|public ^");
    row("in the middle of a term", "rating:p^|private", "rating:public", "rating:public^|private");
    row("before a group closes", "author:(sam|b^)", "author:bob", "author:(sam|bob^)");
    row("all of the operand", "su^mmer beach", "general:sunset", "sunset ^beach");
    row("general named", "general:sum^", "general:summer", "general:summer ^");
    row("in a group without a prefix", "(sam^|beach)", "author:sample_artist", "(author:sample_artist^|beach)");
}

void TestTagRules::putsAChosenTagInPlaceOfTheOperand()
{
    QFETCH(QString, search);
    QFETCH(QString, tag);
    QFETCH(QString, completed);
    const auto [text, cursor] = searchAndCursor(search);
    const std::optional<SearchOperand> operand = searchOperandAt(text, cursor, namespaces);
    QVERIFY(operand.has_value());

    const CompletedSearch result = completedSearch(text, *operand, tagFromServer(tag));

    const auto [expectedText, expectedCursor] = searchAndCursor(completed);
    QCOMPARE(result.text, expectedText);
    QCOMPARE(result.cursor, expectedCursor);
}

}  // namespace photobook::tag_editor

QTEST_GUILESS_MAIN(photobook::tag_editor::TestTagRules)

#include "TestTagRules.moc"
