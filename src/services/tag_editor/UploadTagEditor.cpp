/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "UploadTagEditor.hpp"

#include <algorithm>

#include <QVariantMap>

#include "hlp/Restrictions.hpp"
#include "hlp/TagMessages.hpp"
#include "hlp/TagNames.hpp"
#include "impl/NamespaceRows.hpp"
#include "impl/Namespaces.hpp"

namespace photobook::tag_editor
{

UploadTagEditor::UploadTagEditor(connection::Connection* connection, QObject* parent)
    : TagRowsEditor(parent)
    , m_connection(connection)
    , m_suggestions(std::make_unique<TagSuggestions>(connection))
    , m_namespaces(std::make_unique<Namespaces>(connection))
    , m_rows(std::make_unique<NamespaceRows>())
{
    connect(m_namespaces.get(), &Namespaces::changed, this, &UploadTagEditor::updateRows);
    updateRows();
}

UploadTagEditor::~UploadTagEditor() = default;

void UploadTagEditor::load()
{
    m_namespaces->load();
}

void UploadTagEditor::clear()
{
    if (m_busy)
    {
        return;
    }

    m_tags.clear();
    clearTypedNames();
    m_suggestions->setExcludedTags({});
    setErrorMessage({});
    updateRows();
    announceHasChanges();
}

std::optional<QStringList> UploadTagEditor::tagsToUpload()
{
    if (!addTypedNames())
    {
        return std::nullopt;  // the reason is in errorMessage
    }

    return m_tags;
}

void UploadTagEditor::setBusy(bool busy)
{
    if (busy != m_busy)
    {
        m_busy = busy;
        Q_EMIT busyChanged();
    }
}

bool UploadTagEditor::addTag(const QString& namespaceName, const QString& typedName)
{
    if (m_busy)
    {
        return false;
    }

    const std::expected<TagName, QString> tag = tagFromTypedName(namespaceName, typedName);
    if (!tag.has_value())
    {
        setErrorMessage(tag.error());
        return false;
    }

    if (const std::optional<QString> problem = m_namespaces->notAllowedProblem(namespaceName, tag->name))
    {
        setErrorMessage(*problem);
        return false;
    }

    bool added = true;
    if (m_tags.contains(tag->text()))
    {
        setErrorMessage({});  // chosen already
    }
    else
    {
        // In a single-value namespace the newest tag wins, as on the server.
        const bool singleValue = m_namespaces->singleValueNames().contains(namespaceName);
        added = applyIfAllowed((singleValue ? withoutNamespace(namespaceName) : m_tags) + QStringList{tag->text()});
    }

    if (added)
    {
        forgetTypedName(namespaceName);
    }

    return added;
}

bool UploadTagEditor::setSingleValue(const QString& namespaceName, const QString& typedName)
{
    if (typedName.trimmed().isEmpty())
    {
        keepValue(namespaceName);
        return !m_busy;
    }

    return addTag(namespaceName, typedName);
}

void UploadTagEditor::keepValue(const QString& namespaceName)
{
    if (!m_busy)
    {
        applyIfAllowed(withoutNamespace(namespaceName));
    }
}

bool UploadTagEditor::removeTag(const QString& tag)
{
    if (m_busy || !m_tags.contains(tag))
    {
        return false;
    }

    QStringList tags = m_tags;
    tags.removeAll(tag);
    return applyIfAllowed(tags);
}

bool UploadTagEditor::restoreTag(const QString& /*tag*/)
{
    return false;
}

bool UploadTagEditor::isBusy() const
{
    return m_busy;
}

bool UploadTagEditor::hasChanges() const
{
    return !m_tags.isEmpty() || hasTypedNames();
}

QAbstractListModel* UploadTagEditor::namespaceRows() const
{
    return m_rows.get();
}

QString UploadTagEditor::errorMessage() const
{
    return m_errorMessage;
}

TagSuggestions* UploadTagEditor::suggestions() const
{
    return m_suggestions.get();
}

// As the server checks an upload: the new image must be visible to the user with these tags.
bool UploadTagEditor::applyIfAllowed(const QStringList& tags)
{
    const QStringList restricted =
        restrictedTagsAmong(m_connection->restrictedTags(), m_namespaces->names(), tagNamesOf(tags), QDateTime());
    if (!restricted.isEmpty())
    {
        setErrorMessage(restrictedMessage(restricted));
        return false;
    }

    m_tags = tags;
    m_suggestions->setExcludedTags(tags);
    setErrorMessage({});
    updateRows();
    announceHasChanges();
    return true;
}

QStringList UploadTagEditor::withoutNamespace(const QString& namespaceName) const
{
    QStringList others = m_tags;
    others.removeIf([&](const QString& tag) { return tagFromServer(tag).namespaceName == namespaceName; });
    return others;
}

void UploadTagEditor::updateRows()
{
    QList<NamespaceRows::Row> rows;
    for (const Namespaces::Info& info : m_namespaces->editable())
    {
        QStringList names;
        QVariantList tags;
        for (const QString& text : m_tags)
        {
            const TagName tag = tagFromServer(text);
            if (tag.namespaceName == info.name)
            {
                names.append(tag.name);
            }
        }

        std::ranges::sort(names);
        for (const QString& name : names)
        {
            tags.append(QVariantMap{
                {QStringLiteral("tag"), TagName{info.name, name}.text()},
                {QStringLiteral("name"), name},
                {QStringLiteral("change"), static_cast<int>(Change::Added)},
            });
        }

        NamespaceRows::Row row;
        row.namespaceName = info.name;
        row.dropdown = info.hasDropdown();
        row.fixedValues = info.allowedValues.has_value();
        row.tags = tags;
        if (row.dropdown)
        {
            row.value = names.value(0);
            row.changed = !row.value.isEmpty();
            row.options = info.allowedValues.value_or(m_namespaces->knownValues(info.name));
            if (!row.value.isEmpty() && !row.options.contains(row.value))
            {
                row.options.append(row.value);  // a new value typed in
            }
        }

        rows.append(row);
    }

    m_rows->setRows(rows);
}

void UploadTagEditor::setErrorMessage(const QString& message)
{
    if (message != m_errorMessage)
    {
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }
}

}  // namespace photobook::tag_editor
