/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QSet>
#include <QString>

#include "hlp/TagNames.hpp"

namespace photobook::tag_editor
{

// The operand of a search that the cursor is in, the tag being typed: "p" in "rating:private|p". Read as the
// server's parser reads a term, but forgiving, since the search is still being typed: a group may be open.
struct SearchOperand
{
    qsizetype start = 0;    // in the search, after any '-' and prefix
    qsizetype end = 0;      // the next space, ',', '|', '(' or ')'; it may go on past the cursor
    QString typed;          // from its start to the cursor: what the suggestions start with
    QString namespaceName;  // the prefix in force, e.g. "rating" in "rating:(private|p"; empty: none ("general")
    bool endsTerm = false;  // nothing follows it in its term
};

// Nothing where no tag is being typed: in a date field ("year:20") or a value with '*'.
std::optional<SearchOperand>
searchOperandAt(const QString& search, qsizetype cursor, const QSet<QString>& namespaceNames);

struct CompletedSearch
{
    QString text;
    qsizetype cursor = 0;
};

// The search with the tag in place of the operand: without a namespace where the prefix in force gives it,
// and followed by a space at the end of its term, so the next term can be typed.
CompletedSearch completedSearch(const QString& search, const SearchOperand& operand, const TagName& tag);

}  // namespace photobook::tag_editor
