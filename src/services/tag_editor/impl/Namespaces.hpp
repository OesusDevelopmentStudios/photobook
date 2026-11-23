/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QStringList>

#include "Connection.hpp"

namespace photobook::tag_editor
{

// The server's namespaces as editing tags needs them: which hold one tag, which have a dropdown and what it
// offers. The built-in ones are known before the server's list has arrived.
class Namespaces : public QObject
{
    Q_OBJECT

public:
    struct Info
    {
        QString name;
        bool singleValue = false;
        bool builtin = false;
        std::optional<QStringList> allowedValues;  // nothing: any name
        bool userEditable = true;                  // false: the server alone sets its tags, e.g. meta

        bool hasDropdown() const;  // the built-in one-tag namespaces; custom ones have an add field
    };

    // The connection is not owned and must outlive the namespaces.
    explicit Namespaces(connection::Connection* connection, QObject* parent = nullptr);

    // The server's list, and the values in use of dropdowns without a fixed list (category's)
    void load();

    const QList<Info>& all() const;  // in the server's order: built-in ones first
    QList<Info> editable() const;    // those users tag with: an editor's rows; not meta
    const Info* find(const QString& name) const;
    QSet<QString> names() const;
    QSet<QString> singleValueNames() const;
    QStringList knownValues(const QString& name) const;  // most used first

    // e.g. a rating that is not one of the server's rating values
    std::optional<QString> notAllowedProblem(const QString& namespaceName, const QString& name) const;

Q_SIGNALS:
    void changed();

private:
    void loadKnownValues();

    connection::Connection* m_connection;
    QList<Info> m_infos;
    QHash<QString, QStringList> m_knownValues;
};

}  // namespace photobook::tag_editor
