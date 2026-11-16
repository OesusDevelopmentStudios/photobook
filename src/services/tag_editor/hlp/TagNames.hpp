/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>

#include <QList>
#include <QSet>
#include <QString>

namespace pictorio::tag_editor
{

// The namespace of a tag written without one: "summer" is "general:summer"
inline const QString generalNamespace = QStringLiteral("general");

struct TagName
{
    QString namespaceName;
    QString name;

    QString text() const;  // e.g. "general:summer"
    bool operator==(const TagName&) const = default;
};

// A name typed for a namespace, e.g. in that namespace's row, in the form the server saves it: "Sample Artist"
// in "author" is author:sample_artist. A ':' is part of the name. Or why the server would refuse it: the
// same rules as the server's, so the user hears of a mistake before anything is sent.
std::expected<TagName, QString> tagFromTypedName(const QString& namespaceName, const QString& typedName);

// A tag as the server writes it, always with its namespace, e.g. "category:photo"
TagName tagFromServer(const QString& text);

// Between the server's form ("general:summer") and the parts
QList<TagName> tagNamesOf(const QStringList& texts);
QStringList textsOf(const QList<TagName>& tags);

// An image's tags after a change, worked out as the server does: removals first, then additions; in a
// single-value namespace an added tag replaces the one there.
QList<TagName> tagsAfterChange(const QList<TagName>& current,
                               const QList<TagName>& additions,
                               const QList<TagName>& removals,
                               const QSet<QString>& singleValueNamespaces);

}  // namespace pictorio::tag_editor
