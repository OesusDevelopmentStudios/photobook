/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/SearchOperand.hpp"

#include <algorithm>

#include "hlp/SearchTerm.hpp"

namespace
{

const QString operatorCharacters = QStringLiteral(",|()");

bool endsOperand(QChar character)
{
    return character.isSpace() || operatorCharacters.contains(character);
}

// The prefix ("rating" of "rating:") that starts at position and whose ':' comes before the cursor; or empty.
QString prefixAt(const QString& search, qsizetype position, qsizetype cursor, const QSet<QString>& prefixes)
{
    const qsizetype colon = search.indexOf(QLatin1Char(':'), position);
    if (colon < 0 || colon >= cursor)
    {
        return {};
    }

    const QString name = search.mid(position, colon - position);
    if (std::ranges::any_of(name, endsOperand) || name.contains(QLatin1Char('*')))
    {
        return {};
    }

    return prefixes.contains(name.toLower()) ? name.toLower() : QString();
}

}  // namespace

namespace photobook::tag_editor
{

std::optional<SearchOperand>
searchOperandAt(const QString& search, qsizetype cursor, const QSet<QString>& namespaceNames)
{
    cursor = std::clamp<qsizetype>(cursor, 0, search.size());
    const QSet<QString> prefixes = namespaceNames + searchDateFields;

    qsizetype position = cursor;
    while (position > 0 && !search.at(position - 1).isSpace())
    {
        --position;  // to the start of the term
    }

    // The term's own prefix holds for all of it, an operand's only for that operand, a group's for what is in it.
    QStringList groups{QString()};  // the prefix in force in the term and in each group still open
    QString inForce;                // in the operand being read
    bool termStart = true;
    bool inValue = false;  // past the operand's '-' and prefix
    qsizetype operandStart = position;
    while (position < cursor)
    {
        const QChar character = search.at(position);
        if (operatorCharacters.contains(character))
        {
            if (character == QLatin1Char('('))
            {
                groups.append(inForce);
            }
            else
            {
                if (character == QLatin1Char(')') && groups.size() > 1)
                {
                    groups.removeLast();
                }

                inForce = groups.last();
            }

            termStart = false;
            inValue = false;
            operandStart = ++position;
            continue;
        }

        if (!inValue)
        {
            if (character == QLatin1Char('-'))  // NOT; a '-' inside a name is part of it
            {
                operandStart = ++position;
                continue;
            }

            if (const QString prefix = prefixAt(search, position, cursor, prefixes); !prefix.isEmpty())
            {
                inForce = prefix;
                if (termStart)
                {
                    groups.first() = prefix;
                }

                termStart = false;
                position += prefix.size() + 1;
                operandStart = position;
                continue;
            }

            termStart = false;
            inValue = true;
        }

        ++position;
    }

    qsizetype end = cursor;
    while (end < search.size() && !endsOperand(search.at(end)))
    {
        ++end;
    }

    if (searchDateFields.contains(inForce) || search.mid(operandStart, end - operandStart).contains(QLatin1Char('*')))
    {
        return std::nullopt;
    }

    SearchOperand operand;
    operand.start = operandStart;
    operand.end = end;
    operand.typed = search.mid(operandStart, cursor - operandStart);
    operand.namespaceName = inForce;
    operand.endsTerm = end == search.size() || search.at(end).isSpace();
    return operand;
}

CompletedSearch completedSearch(const QString& search, const SearchOperand& operand, const TagName& tag)
{
    const QString namespaceInForce = operand.namespaceName.isEmpty() ? generalNamespace : operand.namespaceName;
    const QString written = tag.namespaceName == namespaceInForce ? tag.name : tag.text();
    const bool addSpace = operand.endsTerm && operand.end == search.size();

    CompletedSearch completed;
    completed.text =
        search.left(operand.start) + written + (addSpace ? QStringLiteral(" ") : QString()) + search.mid(operand.end);
    completed.cursor = operand.start + written.size() + (operand.endsTerm ? 1 : 0);  // past the space
    return completed;
}

}  // namespace photobook::tag_editor
