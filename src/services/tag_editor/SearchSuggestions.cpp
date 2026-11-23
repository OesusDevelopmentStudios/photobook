/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "SearchSuggestions.hpp"

#include "hlp/SearchOperand.hpp"
#include "impl/Namespaces.hpp"

namespace photobook::tag_editor
{

SearchSuggestions::SearchSuggestions(connection::Connection* connection, QObject* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_suggestions(std::make_unique<TagSuggestions>(connection))
    , m_namespaces(std::make_unique<Namespaces>(connection))
{
    // Read at each login, as namespaces may have been added since
    const auto loadWhenLoggedIn = [this] {
        if (m_connection->state() == connection::Connection::State::LoggedIn)
        {
            m_namespaces->load();
        }
    };
    connect(m_connection, &connection::Connection::stateChanged, this, loadWhenLoggedIn);
    loadWhenLoggedIn();

    // The text typed before the namespaces arrived may hold a prefix only they name.
    connect(m_namespaces.get(), &Namespaces::changed, this, &SearchSuggestions::suggestForTypedSearch);
}

SearchSuggestions::~SearchSuggestions() = default;

TagSuggestions* SearchSuggestions::suggestions() const
{
    return m_suggestions.get();
}

void SearchSuggestions::setTypedSearch(const QString& text, int cursorPosition)
{
    if (text == m_text && cursorPosition == m_cursorPosition)
    {
        return;
    }

    m_text = text;
    m_cursorPosition = cursorPosition;
    suggestForTypedSearch();
}

void SearchSuggestions::choose(int row)
{
    const QString tag = m_suggestions->tagAt(row);
    const std::optional<SearchOperand> operand = searchOperandAt(m_text, m_cursorPosition, m_namespaces->names());
    if (tag.isEmpty() || !operand.has_value())
    {
        return;
    }

    const CompletedSearch search = completedSearch(m_text, *operand, tagFromServer(tag));
    m_text = search.text;
    m_cursorPosition = static_cast<int>(search.cursor);
    m_suggestions->setTypedText({});
    Q_EMIT completed(m_text, m_cursorPosition);
}

void SearchSuggestions::suggestForTypedSearch()
{
    const std::optional<SearchOperand> operand = searchOperandAt(m_text, m_cursorPosition, m_namespaces->names());
    if (!operand.has_value())
    {
        m_suggestions->setTypedText({});
        return;
    }

    m_suggestions->setNamespaceName(operand->namespaceName);
    m_suggestions->setTypedText(operand->typed);
}

}  // namespace photobook::tag_editor
