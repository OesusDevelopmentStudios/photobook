/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TagEditor.hpp"

#include <algorithm>

#include <KLocalizedString>
#include <QJsonArray>
#include <QVariantMap>

#include "hlp/SearchTerm.hpp"
#include "hlp/TagNames.hpp"
#include "impl/NamespaceRows.hpp"

namespace
{

using pictorio::server_client::ApiError;
using pictorio::tag_editor::TagEditor;
using pictorio::tag_editor::tagFromServer;
using pictorio::tag_editor::TagName;

// The tags of one-tag namespaces offered in their dropdown: the most used ones
constexpr int knownValueCount = 50;

QList<TagName> tagNamesOf(const QStringList& texts)
{
    QList<TagName> tags;
    for (const QString& text : texts)
    {
        tags.append(tagFromServer(text));
    }

    return tags;
}

QStringList textsOf(const QList<TagName>& tags)
{
    QStringList texts;
    for (const TagName& tag : tags)
    {
        texts.append(tag.text());
    }

    return texts;
}

QString restrictedMessage(const QStringList& tags)
{
    return i18nc("@info %1 is a list of tags",
                 "An admin has restricted these tags for your account: %1",
                 tags.join(QStringLiteral(", ")));
}

// The server's refusal as a sentence for the user
QString describeError(const ApiError& error)
{
    if (error.code == QLatin1String("tag_restricted"))
    {
        QStringList tags;
        for (const QJsonValue& tag : error.problem.value(QStringLiteral("tags")).toArray())
        {
            tags.append(tag.toString());
        }

        return restrictedMessage(tags);
    }

    if (error.code == pictorio::server_client::networkErrorCode)
    {
        return i18nc("@info %1 is the reason, e.g. Connection refused", "The server does not answer: %1", error.detail);
    }

    return error.detail.isEmpty()
               ? error.title
               : i18nc("@info %1 is a title, %2 an explanation", "%1. %2", error.title, error.detail);
}

QVariantMap tagEntry(const TagName& tag, TagEditor::Change change)
{
    return {
        {QStringLiteral("tag"), tag.text()},
        {QStringLiteral("name"), tag.name},
        {QStringLiteral("change"), static_cast<int>(change)},  // a number, which QML compares with TagEditor.Change
    };
}

}  // namespace

namespace pictorio::tag_editor
{

TagEditor::TagEditor(connection::Connection* connection, QObject* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_suggestions(std::make_unique<TagSuggestions>(connection))
    , m_rows(std::make_unique<NamespaceRows>())
    // The server's built-in namespaces, in its order, until its full list has arrived
    , m_namespaces{
          {QStringLiteral("category"), true, true, std::nullopt},
          {QStringLiteral("author"), false, true, std::nullopt},
          {QStringLiteral("character"), false, true, std::nullopt},
          {QStringLiteral("rating"), true, true, std::nullopt},
          {QStringLiteral("album"), false, true, std::nullopt},
          {QStringLiteral("general"), false, true, std::nullopt},
      }
{}

TagEditor::~TagEditor() = default;

bool TagEditor::NamespaceInfo::hasDropdown() const
{
    return singleValue && builtin;
}

void TagEditor::setImage(const std::optional<server_client::Image>& image)
{
    m_image = image;
    if (m_editing)
    {
        endEditing();
    }

    Q_EMIT imageChanged();
    updateRows();
}

void TagEditor::startEditing()
{
    if (!canEdit() || m_editing)
    {
        return;
    }

    m_editing = true;
    loadNamespaces();  // custom ones, and the allowed values; the built-in ones are known until then
    loadKnownValues();
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

    if (const std::optional<QString> problem = notAllowedProblem(namespaceName, tag->name))
    {
        setErrorMessage(*problem);
        return false;
    }

    bool added = false;
    const QString text = tag->text();
    const bool singleValue = singleValueNamespaces().contains(namespaceName);
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

    if (added && m_typedNames.remove(namespaceName) > 0)
    {
        m_suggestions->setTypedText({});
        Q_EMIT typedNamesChanged();
        announceHasChanges();
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
    if (singleValueNamespaces().contains(namespaceName))
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
            endEditing();
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

void TagEditor::setTypedName(const QString& namespaceName, const QString& text)
{
    if (!m_editing || m_typedNames.value(namespaceName) == text)
    {
        return;
    }

    m_typedNames.insert(namespaceName, text);
    m_suggestions->setNamespaceName(namespaceName);
    m_suggestions->setTypedText(text);
    announceHasChanges();
}

QString TagEditor::typedName(const QString& namespaceName) const
{
    return m_typedNames.value(namespaceName);
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
    const bool somethingTyped =
        std::ranges::any_of(m_typedNames, [](const QString& text) { return !text.trimmed().isEmpty(); });
    return !m_additions.isEmpty() || !m_removals.isEmpty() || somethingTyped;
}

bool TagEditor::isSaving() const
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
    const QList<TagName> after = tagNamesOf(tagsAfter(additions, removals));
    QStringList offending;
    for (const QString& text : m_connection->restrictedTags())
    {
        const std::optional<SearchTerm> term = SearchTerm::parse(text, namespaceNames());
        if (term.has_value() && term->matches(after, m_image->takenAt))
        {
            offending += textsOf(term->mentionedAmong(after));
        }
    }

    if (offending.isEmpty())
    {
        return std::nullopt;
    }

    offending.removeDuplicates();
    offending.sort();
    return restrictedMessage(offending);
}

// e.g. a rating that is not one of the server's rating values
std::optional<QString> TagEditor::notAllowedProblem(const QString& namespaceName, const QString& name) const
{
    const NamespaceInfo* info = namespaceInfo(namespaceName);
    if (info == nullptr || !info->allowedValues.has_value() || info->allowedValues->contains(name))
    {
        return std::nullopt;
    }

    return i18nc("@info %1 is a namespace, %2 a list of names",
                 "A %1 tag can only be one of: %2",
                 namespaceName,
                 info->allowedValues->join(QStringLiteral(", ")));
}

QStringList TagEditor::tagsAfter(const QStringList& additions, const QStringList& removals) const
{
    return textsOf(tagsAfterChange(
        tagNamesOf(m_image->tags), tagNamesOf(additions), tagNamesOf(removals), singleValueNamespaces()));
}

const TagEditor::NamespaceInfo* TagEditor::namespaceInfo(const QString& namespaceName) const
{
    const auto found = std::ranges::find(m_namespaces, namespaceName, &NamespaceInfo::name);
    return found == m_namespaces.end() ? nullptr : &*found;
}

QSet<QString> TagEditor::namespaceNames() const
{
    QSet<QString> names;
    for (const NamespaceInfo& info : m_namespaces)
    {
        names.insert(info.name);
    }

    return names;
}

QSet<QString> TagEditor::singleValueNamespaces() const
{
    QSet<QString> names;
    for (const NamespaceInfo& info : m_namespaces)
    {
        if (info.singleValue)
        {
            names.insert(info.name);
        }
    }

    return names;
}

void TagEditor::announceHasChanges()
{
    const bool changes = hasChanges();
    if (changes != m_hadChanges)
    {
        m_hadChanges = changes;
        Q_EMIT hasChangesChanged();
    }
}

bool TagEditor::addTypedNames()
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

void TagEditor::loadNamespaces()
{
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        return;
    }

    client->namespaces().then(this, [this](const server_client::Result<QList<server_client::Namespace>>& answer) {
        if (!answer.has_value())
        {
            return;  // the built-in ones are still known
        }

        m_namespaces.clear();
        for (const server_client::Namespace& info : *answer)
        {
            m_namespaces.append({info.name, info.singleValue, info.isBuiltin, info.allowedValues});
        }

        loadKnownValues();  // for one-tag namespaces only the server knew of
        updateRows();
    });
}

// For dropdowns without a fixed list, i.e. category's: the tags already in use
void TagEditor::loadKnownValues()
{
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        return;
    }

    for (const NamespaceInfo& info : m_namespaces)
    {
        if (!info.hasDropdown() || info.allowedValues.has_value() || m_knownValues.contains(info.name))
        {
            continue;
        }

        m_knownValues.insert(info.name, {});  // asked once
        client->tags({}, info.name, knownValueCount)
            .then(
                this,
                [this, namespaceName = info.name](const server_client::Result<QList<server_client::TagCount>>& answer) {
                    QStringList names;
                    for (const server_client::TagCount& known : answer.value_or(QList<server_client::TagCount>()))
                    {
                        names.append(tagFromServer(known.tag).name);
                    }

                    m_knownValues.insert(namespaceName, names);
                    updateRows();
                });
    }
}

void TagEditor::updateRows()
{
    if (!m_editing || !m_image.has_value())
    {
        m_rows->setRows({});
        return;
    }

    const QStringList after = tagsAfter(m_additions, m_removals);
    QList<NamespaceRows::Row> rows;
    for (const NamespaceInfo& info : m_namespaces)
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
            row.options = info.allowedValues.value_or(m_knownValues.value(info.name));
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

void TagEditor::clearTypedNames()
{
    if (!m_typedNames.isEmpty())
    {
        m_typedNames.clear();
        Q_EMIT typedNamesChanged();
    }
}

void TagEditor::endEditing()
{
    m_editing = false;
    m_additions.clear();
    m_removals.clear();
    clearTypedNames();
    m_suggestions->setTypedText({});
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

}  // namespace pictorio::tag_editor
