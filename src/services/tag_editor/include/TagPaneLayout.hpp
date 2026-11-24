/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QObject>
#include <QStringList>

#include "Settings.hpp"

namespace photobook::tag_editor
{

// The viewer's tag pane as the user left it, kept between runs: open or not, and which namespaces' sections are
// folded to their heading. A folded namespace stays folded for every image, also while editing.
class TagPaneLayout : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(QStringList foldedNamespaces READ foldedNamespaces NOTIFY foldedNamespacesChanged)

public:
    // The settings are not owned and must outlive the layout.
    explicit TagPaneLayout(settings::Settings* settings, QObject* parent = nullptr);

    bool isOpen() const;  // open at first
    void setOpen(bool open);

    QStringList foldedNamespaces() const;  // none at first
    Q_INVOKABLE bool isFolded(const QString& namespaceName) const;
    Q_INVOKABLE void setFolded(const QString& namespaceName, bool folded);

Q_SIGNALS:
    void openChanged();
    void foldedNamespacesChanged();

private:
    settings::Settings* m_settings;
    bool m_open;
    QStringList m_foldedNamespaces;
};

}  // namespace photobook::tag_editor
