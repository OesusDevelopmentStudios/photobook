/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QObject>
#include <QString>

#include "Connection.hpp"
#include "TagSuggestions.hpp"

namespace photobook::tag_editor
{

class Namespaces;

// Tags for the library's search field: suggestions for the tag under the cursor, e.g. ratings for the "p" of
// "rating:private|p", and the search with a chosen one in its place. Tags that hidden filters hide are
// suggested too: searching for them is the way to see them. Dates ("year:20") get none.
class SearchSuggestions : public QObject
{
    Q_OBJECT

    Q_PROPERTY(photobook::tag_editor::TagSuggestions* suggestions READ suggestions CONSTANT)

public:
    // The connection is not owned and must outlive the suggestions.
    explicit SearchSuggestions(connection::Connection* connection, QObject* parent = nullptr);
    ~SearchSuggestions() override;

    TagSuggestions* suggestions() const;

    // The search as it is being typed, and where the cursor is in it
    Q_INVOKABLE void setTypedSearch(const QString& text, int cursorPosition);

    // Puts the suggestion of the row in place of the tag under the cursor; completed() gives the new search.
    // Nothing more is suggested until the search is typed on.
    Q_INVOKABLE void choose(int row);

Q_SIGNALS:
    void completed(const QString& text, int cursorPosition);

private:
    void suggestForTypedSearch();

    connection::Connection* m_connection;
    std::unique_ptr<TagSuggestions> m_suggestions;
    std::unique_ptr<Namespaces> m_namespaces;  // a prefix may name a custom namespace
    QString m_text;
    int m_cursorPosition = 0;
};

}  // namespace photobook::tag_editor
