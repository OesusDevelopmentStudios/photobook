/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TagEditor.hpp"

#include <algorithm>

#include <KLocalizedString>
#include <QJsonArray>
#include <QVariantMap>

#include "hlp/Restrictions.hpp"
#include "hlp/TagMessages.hpp"
#include "hlp/TagNames.hpp"
#include "impl/NamespaceRows.hpp"
#include "impl/Namespaces.hpp"

namespace
{

using photobook::tag_editor::TagEditor;
using photobook::tag_editor::tagFromServer;
using photobook::tag_editor::TagName;

QVariantMap tagEntry(const TagName& tag, TagEditor::Change change, int count = -1)
{
    return {
        {QStringLiteral("tag"), tag.text()},
        {QStringLiteral("name"), tag.name},
        {QStringLiteral("change"), static_cast<int>(change)},  // a number, which QML compares with TagRowsEditor.Change
        {QStringLiteral("count"), count},                      // images with the tag; -1 until known
    };
}

}  // namespace

namespace photobook::tag_editor
{

TagEditor::TagEditor(connection::Connection* connection, QObject* parent)
    : TagRowsEditor(parent)
    , m_connection(connection)
    , m_suggestions(std::make_unique<TagSuggestions>(connection))
    , m_namespaces(std::make_unique<Namespaces>(connection))
    , m_rows(std::make_unique<NamespaceRows>())
{
    connect(m_namespaces.get(), &Namespaces::changed, this, &TagEditor::updateRows);

    // The namespaces at each login, so that the image's tags have their place; counts belong to one server.
    connect(m_connection, &connection::Connection::stateChanged, this, [this]() {
        if (m_connection->state() == connection::Connection::State::LoggedIn)
        {
            m_namespaces->load();
        }
        else
        {
            m_counts.clear();
        }
    });

    if (m_connection->state() == connection::Connection::State::LoggedIn)
    {
        m_namespaces->load();
    }
}

TagEditor::~TagEditor() = default;

void TagEditor::setImage(const std::optional<server_client::Image>& image)
{
    m_image = image;
    if (m_editing)
    {
        endEditing();
    }

    Q_EMIT imageChanged();
    updateRows();
    if (m_image.has_value() && !m_counts.contains(m_image->id))
    {
        loadCounts(m_image->id);
    }
}

void TagEditor::startEditing()
{
    if (!canEdit() || m_editing)
    {
        return;
    }

    m_editing = true;
    m_namespaces->load();  // custom ones, and the allowed values; the built-in ones are known until then
    m_suggestions->setExcludedTags(m_image->tags);
    Q_EMIT editingChanged();
    updateRows();
}

bool TagEditor::addTag(const QString& namespaceName, const QString& typedName)
{
    if (!m_editing || m_saving)
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

    bool added = false;
    const QString text = tag->text();
    const bool singleValue = m_namespaces->singleValueNames().contains(namespaceName);
    if (m_removals.contains(text) || (singleValue && m_image->tags.contains(text)))
    {
        added = restoreTag(text);  // the image's own tag back
    }
    else if (m_image->tags.contains(text) || m_additions.contains(text))
    {
        setErrorMessage({});
        added = true;  // the image has it already
    }
    else
    {
        // In a single-value namespace the newest addition wins, as on the server.
        QStringList additions = m_additions;
        if (singleValue)
        {
            additions.removeIf(
                [&](const QString& other) { return tagFromServer(other).namespaceName == namespaceName; });
        }

        additions.append(text);
        added = applyIfAllowed(additions, m_removals);
    }

    if (added)
    {
        forgetTypedName(namespaceName);
    }

    return added;
}

bool TagEditor::setSingleValue(const QString& namespaceName, const QString& typedName)
{
    if (!m_editing || m_saving)
    {
        return false;
    }

    if (!typedName.trimmed().isEmpty())
    {
        return addTag(namespaceName, typedName);
    }

    // None: the image's tag in the namespace goes, and any added one.
    QStringList additions = m_additions;
    additions.removeIf([&](const QString& added) { return tagFromServer(added).namespaceName == namespaceName; });
    QStringList removals = m_removals;
    for (const QString& tag : m_image->tags)
    {
        if (tagFromServer(tag).namespaceName == namespaceName && !removals.contains(tag))
        {
            removals.append(tag);
        }
    }

    return applyIfAllowed(additions, removals);
}

void TagEditor::keepValue(const QString& namespaceName)
{
    if (!m_editing || m_saving)
    {
        return;
    }

    const auto inNamespace = [&](const QString& tag) { return tagFromServer(tag).namespaceName == namespaceName; };
    QStringList additions = m_additions;
    additions.removeIf(inNamespace);
    QStringList removals = m_removals;
    removals.removeIf(inNamespace);
    applyIfAllowed(additions, removals);
}

bool TagEditor::removeTag(const QString& tag)
{
    if (!m_editing || m_saving)
    {
        return false;
    }

    if (m_additions.contains(tag))
    {
        QStringList additions = m_additions;
        additions.removeAll(tag);
        return applyIfAllowed(additions, m_removals);
    }

    if (m_image->tags.contains(tag) && !m_removals.contains(tag))
    {
        return applyIfAllowed(m_additions, m_removals + QStringList{tag});
    }

    return false;
}

bool TagEditor::restoreTag(const QString& tag)
{
    if (!m_editing || m_saving || !m_image->tags.contains(tag))
    {
        return false;
    }

    QStringList removals = m_removals;
    removals.removeAll(tag);

    // A tag replaced by an addition in a single-value namespace comes back when that addition goes.
    QStringList additions = m_additions;
    const QString namespaceName = tagFromServer(tag).namespaceName;
    if (m_namespaces->singleValueNames().contains(namespaceName))
    {
        additions.removeIf([&](const QString& added) { return tagFromServer(added).namespaceName == namespaceName; });
    }

    return applyIfAllowed(additions, removals);
}

void TagEditor::save()
{
    if (!m_editing || m_saving)
    {
        return;
    }

    // Typed, not added yet: added now, as the user expects it saved. Refused: nothing is saved.
    if (!addTypedNames())
    {
        return;
    }

    if (!hasChanges())
    {
        endEditing();
        return;
    }

    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        setErrorMessage(i18nc("@info", "Not connected to the server."));
        return;
    }

    setErrorMessage({});
    setSaving(true);
    const int imageId = m_image->id;
    client->changeImageTags(imageId, server_client::TagChanges{m_additions, m_removals})
        .then(this, [this, imageId](const server_client::Result<server_client::Image>& answer) {
            setSaving(false);
            if (!m_image.has_value() || m_image->id != imageId)
            {
                return;  // another image is shown by now
            }

            if (!answer.has_value())
            {
                setErrorMessage(describeError(answer.error()));  // the changes stay, to fix and save again
                return;
            }

            m_image = *answer;
            m_counts.remove(imageId);  // its tags changed, so the counts did
            endEditing();
            loadCounts(imageId);
            Q_EMIT saved(*answer);
        });
}

void TagEditor::cancel()
{
    if (m_editing && !m_saving)
    {
        endEditing();
    }
}

bool TagEditor::canEdit() const
{
    return m_image.has_value();
}

bool TagEditor::isEditing() const
{
    return m_editing;
}

bool TagEditor::hasChanges() const
{
    return !m_additions.isEmpty() || !m_removals.isEmpty() || hasTypedNames();
}

bool TagEditor::isSaving() const
{
    return m_saving;
}

bool TagEditor::isBusy() const
{
    return m_saving;
}

QAbstractListModel* TagEditor::namespaceRows() const
{
    return m_rows.get();
}

QString TagEditor::errorMessage() const
{
    return m_errorMessage;
}

TagSuggestions* TagEditor::suggestions() const
{
    return m_suggestions.get();
}

bool TagEditor::applyIfAllowed(const QStringList& additions, const QStringList& removals)
{
    if (const std::optional<QString> problem = restrictionProblem(additions, removals))
    {
        setErrorMessage(*problem);
        return false;
    }

    m_additions = additions;
    m_removals = removals;
    m_suggestions->setExcludedTags(tagsAfter(additions, removals));
    setErrorMessage({});
    updateRows();
    announceHasChanges();
    return true;
}

// As the server checks it: the image must stay visible to the user with its tags after the change. The
// message names the image's tags that the restrictions which apply mention.
std::optional<QString> TagEditor::restrictionProblem(const QStringList& additions, const QStringList& removals) const
{
    const QStringList offending = restrictedTagsAmong(m_connection->restrictedTags(),
                                                      m_namespaces->names(),
                                                      tagNamesOf(tagsAfter(additions, removals)),
                                                      m_image->takenAt);
    if (offending.isEmpty())
    {
        return std::nullopt;
    }

    return restrictedMessage(offending);
}

QStringList TagEditor::tagsAfter(const QStringList& additions, const QStringList& removals) const
{
    return textsOf(tagsAfterChange(
        tagNamesOf(m_image->tags), tagNamesOf(additions), tagNamesOf(removals), m_namespaces->singleValueNames()));
}

void TagEditor::updateRows()
{
    if (!m_image.has_value())
    {
        m_rows->setRows({});
        return;
    }

    if (!m_editing)
    {
        showImageTags();
        return;
    }

    const QStringList after = tagsAfter(m_additions, m_removals);
    QList<NamespaceRows::Row> rows;
    for (const Namespaces::Info& info : m_namespaces->editable())
    {
        QList<QPair<TagName, Change>> entries;
        QString valueBefore;
        QString valueAfter;
        for (const QString& text : m_image->tags + m_additions)
        {
            const TagName tag = tagFromServer(text);
            if (tag.namespaceName != info.name)
            {
                continue;
            }

            const bool own = m_image->tags.contains(text);
            entries.append({tag, own ? (after.contains(text) ? Change::Kept : Change::Removed) : Change::Added});
            if (own)
            {
                valueBefore = tag.name;
            }

            if (after.contains(text))
            {
                valueAfter = tag.name;
            }
        }

        std::ranges::sort(entries, {}, [](const auto& entry) { return entry.first.name; });
        QVariantList tags;
        for (const auto& [tag, change] : entries)
        {
            tags.append(tagEntry(tag, change));
        }

        NamespaceRows::Row row;
        row.namespaceName = info.name;
        row.dropdown = info.hasDropdown();
        row.fixedValues = info.allowedValues.has_value();
        row.tags = tags;
        if (row.dropdown)
        {
            row.options = info.allowedValues.value_or(m_namespaces->knownValues(info.name));
            for (const QString& name : {valueBefore, valueAfter})
            {
                if (!name.isEmpty() && !row.options.contains(name))
                {
                    row.options.append(name);
                }
            }

            row.value = valueAfter;
            row.changed = valueAfter != valueBefore;
        }

        rows.append(row);
    }

    m_rows->setRows(rows);
}

void TagEditor::showImageTags()
{
    // The server's order; a namespace it has not listed yet goes before those only the server sets.
    QStringList order;
    QStringList serverOnly;
    for (const Namespaces::Info& info : m_namespaces->all())
    {
        if (info.userEditable)
        {
            order.append(info.name);
        }
        else
        {
            serverOnly.append(info.name);
        }
    }

    for (const TagName& tag : tagNamesOf(m_image->tags))
    {
        if (!order.contains(tag.namespaceName) && !serverOnly.contains(tag.namespaceName))
        {
            order.append(tag.namespaceName);
        }
    }

    const QHash<QString, int>* counts = m_counts.object(m_image->id);
    QList<NamespaceRows::Row> rows;
    for (const QString& namespaceName : order + serverOnly)
    {
        QList<TagName> tags;
        for (const TagName& tag : tagNamesOf(m_image->tags))
        {
            if (tag.namespaceName == namespaceName)
            {
                tags.append(tag);
            }
        }

        if (tags.isEmpty())
        {
            continue;
        }

        std::ranges::sort(tags, {}, &TagName::name);
        NamespaceRows::Row row;
        row.namespaceName = namespaceName;
        for (const TagName& tag : tags)
        {
            row.tags.append(tagEntry(tag, Change::Kept, counts != nullptr ? counts->value(tag.text(), -1) : -1));
        }

        rows.append(row);
    }

    m_rows->setRows(rows);
}

// Without an answer the tags are shown without counts.
void TagEditor::loadCounts(int imageId)
{
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        return;
    }

    client->imageTagCounts(imageId).then(
        this, [this, imageId](const server_client::Result<QList<server_client::TagCount>>& answer) {
            if (!answer.has_value())
            {
                return;
            }

            auto* counts = new QHash<QString, int>();
            for (const server_client::TagCount& tagCount : *answer)
            {
                counts->insert(tagCount.tag, tagCount.imageCount);
            }

            m_counts.insert(imageId, counts);  // the cache owns it now
            if (m_image.has_value() && m_image->id == imageId && !m_editing)
            {
                showImageTags();
            }
        });
}

void TagEditor::endEditing()
{
    m_editing = false;
    m_additions.clear();
    m_removals.clear();
    clearTypedNames();
    m_suggestions->setNamespaceName({});
    setErrorMessage({});
    Q_EMIT editingChanged();
    updateRows();
    announceHasChanges();
}

void TagEditor::setSaving(bool saving)
{
    if (saving != m_saving)
    {
        m_saving = saving;
        Q_EMIT savingChanged();
        Q_EMIT busyChanged();
    }
}

void TagEditor::setErrorMessage(const QString& message)
{
    if (message != m_errorMessage)
    {
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }
}

}  // namespace photobook::tag_editor
