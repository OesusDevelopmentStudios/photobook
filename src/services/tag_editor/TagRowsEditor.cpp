/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TagRowsEditor.hpp"

#include <algorithm>

namespace photobook::tag_editor
{

TagRowsEditor::~TagRowsEditor() = default;

void TagRowsEditor::setTypedName(const QString& namespaceName, const QString& text)
{
    if (m_typedNames.value(namespaceName) == text)
    {
        return;
    }

    m_typedNames.insert(namespaceName, text);
    if (TagSuggestions* namespaceSuggestions = suggestions())
    {
        namespaceSuggestions->setNamespaceName(namespaceName);
        namespaceSuggestions->setTypedText(text);
    }

    announceHasChanges();
}

QString TagRowsEditor::typedName(const QString& namespaceName) const
{
    return m_typedNames.value(namespaceName);
}

bool TagRowsEditor::hasTypedNames() const
{
    return std::ranges::any_of(m_typedNames, [](const QString& text) { return !text.trimmed().isEmpty(); });
}

bool TagRowsEditor::addTypedNames()
{
    const QHash<QString, QString> typedNames = m_typedNames;
    for (auto typed = typedNames.cbegin(); typed != typedNames.cend(); ++typed)
    {
        if (!typed.value().trimmed().isEmpty() && !addTag(typed.key(), typed.value()))
        {
            return false;
        }
    }

    return true;
}

void TagRowsEditor::forgetTypedName(const QString& namespaceName)
{
    if (m_typedNames.remove(namespaceName) > 0)
    {
        clearSuggestions();
        Q_EMIT typedNamesChanged();
        announceHasChanges();
    }
}

void TagRowsEditor::clearTypedNames()
{
    if (!m_typedNames.isEmpty())
    {
        m_typedNames.clear();
        Q_EMIT typedNamesChanged();
    }

    clearSuggestions();
}

void TagRowsEditor::clearSuggestions()
{
    if (TagSuggestions* namespaceSuggestions = suggestions())
    {
        namespaceSuggestions->setTypedText({});
    }
}

void TagRowsEditor::announceHasChanges()
{
    const bool changes = hasChanges();
    if (changes != m_hadChanges)
    {
        m_hadChanges = changes;
        Q_EMIT hasChangesChanged();
    }
}

}  // namespace photobook::tag_editor
