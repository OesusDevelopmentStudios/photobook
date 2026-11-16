/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>

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

// A tag as the user typed it, in the form the server saves it: "Author:Sample Artist" is
// author:sample_artist, if "author" is a namespace. Or why the server would refuse it. The same rules as the
// server's, so the user hears of a mistake before anything is sent.
std::expected<TagName, QString> tagFromTypedText(const QString& typedText, const QSet<QString>& namespaceNames);

// A tag as the server writes it, always with its namespace, e.g. "category:photo"
TagName tagFromServer(const QString& text);

}  // namespace pictorio::tag_editor
