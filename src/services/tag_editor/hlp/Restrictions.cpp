/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/Restrictions.hpp"

#include <optional>

#include "hlp/SearchTerm.hpp"

namespace pictorio::tag_editor
{

QStringList restrictedTagsAmong(const QStringList& restrictedTerms,
                                const QSet<QString>& namespaceNames,
                                const QList<TagName>& tagsAfter,
                                const QDateTime& takenAt)
{
    QStringList offending;
    for (const QString& text : restrictedTerms)
    {
        const std::optional<SearchTerm> term = SearchTerm::parse(text, namespaceNames);
        if (term.has_value() && term->matches(tagsAfter, takenAt))
        {
            offending += textsOf(term->mentionedAmong(tagsAfter));
        }
    }

    offending.removeDuplicates();
    offending.sort();
    return offending;
}

}  // namespace pictorio::tag_editor
