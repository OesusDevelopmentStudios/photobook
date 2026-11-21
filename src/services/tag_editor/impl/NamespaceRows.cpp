/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/NamespaceRows.hpp"

namespace photobook::tag_editor
{

void NamespaceRows::setRows(const QList<Row>& rows)
{
    if (!hasSameNamespaces(rows))
    {
        beginResetModel();
        m_rows = rows;
        endResetModel();
        return;
    }

    for (int row = 0; row < m_rows.size(); ++row)
    {
        if (m_rows.at(row) != rows.at(row))
        {
            m_rows[row] = rows.at(row);
            Q_EMIT dataChanged(index(row), index(row));
        }
    }
}

int NamespaceRows::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant NamespaceRows::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
    {
        return {};
    }

    const Row& row = m_rows.at(index.row());
    switch (role)
    {
    case NamespaceRole:
        return row.namespaceName;
    case DropdownRole:
        return row.dropdown;
    case FixedValuesRole:
        return row.fixedValues;
    case KeepableRole:
        return row.keepable;
    case OptionsRole:
        return row.options;
    case ValueRole:
        return row.value;
    case ChangedRole:
        return row.changed;
    case TagsRole:
        return row.tags;
    default:
        return {};
    }
}

QHash<int, QByteArray> NamespaceRows::roleNames() const
{
    return {
        {NamespaceRole, "namespaceName"},
        {DropdownRole, "dropdown"},
        {FixedValuesRole, "fixedValues"},
        {KeepableRole, "keepable"},
        {OptionsRole, "options"},
        {ValueRole, "value"},
        {ChangedRole, "changed"},
        {TagsRole, "tags"},
    };
}

bool NamespaceRows::hasSameNamespaces(const QList<Row>& rows) const
{
    if (rows.size() != m_rows.size())
    {
        return false;
    }

    for (int row = 0; row < rows.size(); ++row)
    {
        if (rows.at(row).namespaceName != m_rows.at(row).namespaceName)
        {
            return false;
        }
    }

    return true;
}

}  // namespace photobook::tag_editor
