/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QVariantList>

namespace pictorio::tag_editor
{

// The namespaces while an image's tags are edited, one row each, for the edit view. The rows stay while their
// content changes, so a field the user is typing in keeps its focus.
class NamespaceRows : public QAbstractListModel
{
    Q_OBJECT

public:
    struct Row
    {
        QString namespaceName;
        bool dropdown = false;     // a built-in one-tag namespace: one tag or none, chosen from `options`
        bool fixedValues = false;  // only `options` may be chosen, e.g. the ratings
        bool keepable = false;     // the dropdown offers "keep as is" first, for many images at once
        QStringList options;       // for a dropdown: the names to choose from
        QString value;             // for a dropdown: the name after the changes, or the shared one; empty: none
        bool changed = false;      // for a dropdown: the value differs from the image's, or is not kept as is
        QVariantList tags;         // {"tag", "name", "change"} of each tag

        bool operator==(const Row&) const = default;
    };

    enum Role
    {
        NamespaceRole = Qt::UserRole + 1,
        DropdownRole,
        FixedValuesRole,
        KeepableRole,
        OptionsRole,
        ValueRole,
        ChangedRole,
        TagsRole,
    };

    using QAbstractListModel::QAbstractListModel;

    // Rows for the same namespaces change in place; other namespaces start over.
    void setRows(const QList<Row>& rows);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    bool hasSameNamespaces(const QList<Row>& rows) const;

    QList<Row> m_rows;
};

}  // namespace pictorio::tag_editor
