/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TagPaneLayout.hpp"

namespace photobook::tag_editor
{

TagPaneLayout::TagPaneLayout(settings::Settings* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_open(settings->tagPaneOpen().value_or(true))
    , m_foldedNamespaces(settings->foldedTagNamespaces().value_or(QStringList()))
{}

bool TagPaneLayout::isOpen() const
{
    return m_open;
}

void TagPaneLayout::setOpen(bool open)
{
    if (open != m_open)
    {
        m_open = open;
        m_settings->setTagPaneOpen(open);
        Q_EMIT openChanged();
    }
}

QStringList TagPaneLayout::foldedNamespaces() const
{
    return m_foldedNamespaces;
}

bool TagPaneLayout::isFolded(const QString& namespaceName) const
{
    return m_foldedNamespaces.contains(namespaceName);
}

void TagPaneLayout::setFolded(const QString& namespaceName, bool folded)
{
    if (folded == isFolded(namespaceName))
    {
        return;
    }

    if (folded)
    {
        m_foldedNamespaces.append(namespaceName);
    }
    else
    {
        m_foldedNamespaces.removeAll(namespaceName);
    }

    m_settings->setFoldedTagNamespaces(m_foldedNamespaces);
    Q_EMIT foldedNamespacesChanged();
}

}  // namespace photobook::tag_editor
