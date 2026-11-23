/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/SearchTerm.hpp"

#include <algorithm>
#include <utility>

#include <QRegularExpression>

namespace
{

using photobook::tag_editor::generalNamespace;
using photobook::tag_editor::searchDateFields;
using photobook::tag_editor::SearchTerm;
using photobook::tag_editor::TagName;
using Node = SearchTerm::Node;

const QString operatorCharacters = QStringLiteral(",|()");

Node negation(Node operand)
{
    Node node;
    node.kind = Node::Kind::Not;
    node.operands.push_back(std::move(operand));
    return node;
}

// One operand stays itself; several are joined.
Node joined(Node::Kind kind, std::vector<Node> operands)
{
    if (operands.size() == 1)
    {
        return std::move(operands.front());
    }

    Node node;
    node.kind = kind;
    node.operands = std::move(operands);
    return node;
}

// Reads one term, as the server's parser does:
//   term     := [ "-" ] [ prefix ":" ] or_expr
//   or_expr  := and_expr ( "|" and_expr )*
//   and_expr := operand ( "," operand )*
//   operand  := [ "-" ] [ prefix ":" ] ( "(" or_expr ")" | value )
// A value belongs to the nearest prefix to its left that encloses it, else to "general".
class TermParser
{
public:
    TermParser(const QString& text, const QSet<QString>& namespaceNames)
        : m_text(text)
        , m_prefixes(namespaceNames + searchDateFields)
    {}

    std::optional<Node> parseTerm()
    {
        const bool negated = consume(QLatin1Char('-'));
        const QString prefix = readPrefix();
        std::optional<Node> node = orExpression(prefix.isEmpty() ? generalNamespace : prefix);
        if (!node.has_value() || m_position < m_text.size())
        {
            return std::nullopt;
        }

        return negated ? negation(std::move(*node)) : std::move(*node);
    }

private:
    std::optional<Node> orExpression(const QString& namespaceName)
    {
        return expression(namespaceName, QLatin1Char('|'), Node::Kind::AnyOf, &TermParser::andExpression);
    }

    std::optional<Node> andExpression(const QString& namespaceName)
    {
        return expression(namespaceName, QLatin1Char(','), Node::Kind::All, &TermParser::operand);
    }

    // Operands read by readOperand, separated by the operator
    std::optional<Node> expression(const QString& namespaceName,
                                   QChar separator,
                                   Node::Kind kind,
                                   std::optional<Node> (TermParser::*readOperand)(const QString&))
    {
        std::vector<Node> operands;
        do
        {
            std::optional<Node> next = (this->*readOperand)(namespaceName);
            if (!next.has_value())
            {
                return std::nullopt;
            }

            operands.push_back(std::move(*next));
        } while (consume(separator));

        return joined(kind, std::move(operands));
    }

    std::optional<Node> operand(const QString& enclosingNamespace)
    {
        const bool negated = consume(QLatin1Char('-'));
        const QString prefix = readPrefix();
        const QString namespaceName = prefix.isEmpty() ? enclosingNamespace : prefix;
        std::optional<Node> node;
        if (consume(QLatin1Char('(')))
        {
            node = orExpression(namespaceName);
            if (!consume(QLatin1Char(')')))
            {
                return std::nullopt;
            }
        }
        else
        {
            node = value(namespaceName);
        }

        if (!node.has_value())
        {
            return std::nullopt;
        }

        return negated ? negation(std::move(*node)) : std::move(*node);
    }

    // "name:" if name is a namespace or a date field; anything else is part of a value.
    QString readPrefix()
    {
        qsizetype nameEnd = m_position;
        while (nameEnd < m_text.size() && !operatorCharacters.contains(m_text.at(nameEnd)) &&
               m_text.at(nameEnd) != QLatin1Char(':') && m_text.at(nameEnd) != QLatin1Char('*'))
        {
            ++nameEnd;
        }

        const QString name = m_text.mid(m_position, nameEnd - m_position).toLower();
        if (nameEnd < m_text.size() && m_text.at(nameEnd) == QLatin1Char(':') && m_prefixes.contains(name))
        {
            m_position = nameEnd + 1;
            return name;
        }

        return {};
    }

    std::optional<Node> value(const QString& namespaceName)
    {
        const qsizetype start = m_position;
        while (m_position < m_text.size() && !operatorCharacters.contains(m_text.at(m_position)))
        {
            ++m_position;
        }

        const QString text = m_text.mid(start, m_position - start);
        if (text.isEmpty())
        {
            return std::nullopt;
        }

        if (searchDateFields.contains(namespaceName))
        {
            return dateValue(namespaceName, text);
        }

        const qsizetype star = text.indexOf(QLatin1Char('*'));
        if (star >= 0 && star != text.size() - 1)
        {
            return std::nullopt;  // '*' only at the end
        }

        Node node;
        node.kind = Node::Kind::Tag;
        node.anyNameStarting = star >= 0;
        node.tag =
            TagName{namespaceName, text.chopped(star >= 0 ? 1 : 0).normalized(QString::NormalizationForm_C).toLower()};
        return node;
    }

    // e.g. 2025, 2025-07, 2025-07-14, or a range 2025-01-01..2025-06-30
    static std::optional<Node> dateValue(const QString& field, const QString& text)
    {
        const qsizetype dots = text.indexOf(QStringLiteral(".."));
        const std::optional<std::pair<QDate, QDate>> first = period(field, dots < 0 ? text : text.left(dots));
        const std::optional<std::pair<QDate, QDate>> last = dots < 0 ? first : period(field, text.mid(dots + 2));
        if (!first.has_value() || !last.has_value() || last->second < first->first)
        {
            return std::nullopt;
        }

        Node node;
        node.kind = Node::Kind::Date;
        node.firstDay = first->first;
        node.lastDay = last->second;
        return node;
    }

    // The first and last day of a year, a month or a day
    static std::optional<std::pair<QDate, QDate>> period(const QString& field, const QString& text)
    {
        static const QRegularExpression year(QStringLiteral("^(\\d{4})$"));
        static const QRegularExpression month(QStringLiteral("^(\\d{4})-(\\d{2})$"));
        static const QRegularExpression day(QStringLiteral("^(\\d{4})-(\\d{2})-(\\d{2})$"));
        const QRegularExpression& pattern = field == QLatin1String("year")    ? year
                                            : field == QLatin1String("month") ? month
                                                                              : day;
        const QRegularExpressionMatch match = pattern.match(text);
        if (!match.hasMatch())
        {
            return std::nullopt;
        }

        const int yearNumber = match.captured(1).toInt();
        if (field == QLatin1String("year"))
        {
            return std::pair{QDate(yearNumber, 1, 1), QDate(yearNumber, 12, 31)};
        }

        const QDate firstDay(
            yearNumber, match.captured(2).toInt(), field == QLatin1String("month") ? 1 : match.captured(3).toInt());
        if (!firstDay.isValid())
        {
            return std::nullopt;
        }

        const QDate lastDay = field == QLatin1String("month") ? firstDay.addMonths(1).addDays(-1) : firstDay;
        return std::pair{firstDay, lastDay};
    }

    bool consume(QChar character)
    {
        if (m_position < m_text.size() && m_text.at(m_position) == character)
        {
            ++m_position;
            return true;
        }

        return false;
    }

    const QString m_text;
    const QSet<QString> m_prefixes;
    qsizetype m_position = 0;
};

bool tagMatches(const Node& match, const TagName& tag)
{
    if (tag.namespaceName != match.tag.namespaceName)
    {
        return false;
    }

    return match.anyNameStarting ? tag.name.startsWith(match.tag.name) : tag.name == match.tag.name;
}

bool nodeMatches(const Node& node, const QList<TagName>& tags, const QDateTime& takenAt)
{
    const auto operandMatches = [&](const Node& operand) { return nodeMatches(operand, tags, takenAt); };
    switch (node.kind)
    {
    case Node::Kind::Tag:
        return std::ranges::any_of(tags, [&](const TagName& tag) { return tagMatches(node, tag); });
    case Node::Kind::Date:
        return takenAt.isValid() && node.firstDay <= takenAt.date() && takenAt.date() <= node.lastDay;
    case Node::Kind::Not:
        return !nodeMatches(node.operands.front(), tags, takenAt);
    case Node::Kind::All:
        return std::ranges::all_of(node.operands, operandMatches);
    case Node::Kind::AnyOf:
        return std::ranges::any_of(node.operands, operandMatches);
    }

    return false;
}

void collectTagMatches(const Node& node, QList<const Node*>& matches)
{
    if (node.kind == Node::Kind::Tag)
    {
        matches.append(&node);
    }

    for (const Node& operand : node.operands)
    {
        collectTagMatches(operand, matches);
    }
}

}  // namespace

namespace photobook::tag_editor
{

SearchTerm::SearchTerm(Node root)
    : m_root(std::move(root))
{}

std::optional<SearchTerm> SearchTerm::parse(const QString& text, const QSet<QString>& namespaceNames)
{
    const QString term = text.trimmed();
    if (term.isEmpty() || std::ranges::any_of(term, [](QChar character) { return character.isSpace(); }))
    {
        return std::nullopt;
    }

    std::optional<Node> root = TermParser(term, namespaceNames).parseTerm();
    if (!root.has_value())
    {
        return std::nullopt;
    }

    return SearchTerm(std::move(*root));
}

bool SearchTerm::matches(const QList<TagName>& tags, const QDateTime& takenAt) const
{
    return nodeMatches(m_root, tags, takenAt);
}

QList<TagName> SearchTerm::mentionedAmong(const QList<TagName>& tags) const
{
    QList<const Node*> namedTags;
    collectTagMatches(m_root, namedTags);

    QList<TagName> mentioned;
    for (const TagName& tag : tags)
    {
        if (std::ranges::any_of(namedTags, [&](const Node* match) { return tagMatches(*match, tag); }))
        {
            mentioned.append(tag);
        }
    }

    return mentioned;
}

}  // namespace photobook::tag_editor
