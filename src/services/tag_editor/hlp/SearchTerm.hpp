/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>
#include <vector>

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QSet>
#include <QString>

#include "hlp/TagNames.hpp"

namespace pictorio::tag_editor
{

// One term of the search language (data-model.md §3), e.g. a restricted term such as "rating:nsfw|questionable".
// Matches an image whose tags are known, without asking the server: the same rules as the server's own check,
// so a restricted tag is refused before anything is sent.
class SearchTerm
{
public:
    struct Node
    {
        enum class Kind
        {
            Tag,    // a tag, or every tag of the namespace whose name starts with `name` ("" means any)
            Date,   // taken on a day from `firstDay` to `lastDay`
            Not,    // the one operand does not match
            All,    // every operand matches
            AnyOf,  // at least one operand matches
        };

        Kind kind = Kind::All;
        TagName tag;
        bool anyNameStarting = false;
        QDate firstDay;
        QDate lastDay;
        std::vector<Node> operands;
    };

    // Nothing if the text is not one valid term
    static std::optional<SearchTerm> parse(const QString& text, const QSet<QString>& namespaceNames);

    // takenAt: invalid if not known; a date in the term then does not match.
    bool matches(const QList<TagName>& tags, const QDateTime& takenAt) const;

    // Those of the tags that the term names, e.g. to say which tags an admin restricted
    QList<TagName> mentionedAmong(const QList<TagName>& tags) const;

private:
    explicit SearchTerm(Node root);

    Node m_root;
};

}  // namespace pictorio::tag_editor
