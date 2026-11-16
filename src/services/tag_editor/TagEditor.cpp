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

namespace
{

using pictorio::server_client::ApiError;
using pictorio::tag_editor::TagEditor;
using pictorio::tag_editor::tagFromServer;
using pictorio::tag_editor::TagName;

// The server's built-in namespaces, known before its full list has arrived
const QSet<QString> builtinNamespaces{
    QStringLiteral("category"),
    QStringLiteral("author"),
    QStringLiteral("character"),
    QStringLiteral("rating"),
    QStringLiteral("album"),
    QStringLiteral("general"),
};
const QSet<QString> builtinSingleValueNamespaces{QStringLiteral("category"), QStringLiteral("rating")};

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
        {QStringLiteral("change"), QVariant::fromValue(change)},
    };
}

}  // namespace

namespace pictorio::tag_editor
{

TagEditor::TagEditor(connection::Connection* connection, QObject* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_suggestions(std::make_unique<TagSuggestions>(connection))
    , m_namespaceNames(builtinNamespaces)
    , m_singleValueNamespaces(builtinSingleValueNamespaces)
{}

TagEditor::~TagEditor() = default;

void TagEditor::setImage(const std::optional<server_client::Image>& image)
{
    m_image = image;
    if (m_editing)
    {
        endEditing();
    }

    Q_EMIT imageChanged();
    Q_EMIT changesChanged();
}

void TagEditor::startEditing()
{
    if (!canEdit() || m_editing)
    {
        return;
    }

    m_editing = true;
    loadNamespaces();  // custom ones and which hold one value; the built-in ones are known until then
    m_suggestions->setExcludedTags(m_image->tags);
    Q_EMIT editingChanged();
    Q_EMIT changesChanged();
}

bool TagEditor::addTag(const QString& typedText)
{
    if (!m_editing || m_saving)
    {
        return false;
    }

    const std::expected<TagName, QString> tag = tagFromTypedText(typedText, m_namespaceNames);
    if (!tag.has_value())
    {
        setErrorMessage(tag.error());
        return false;
    }

    const QString text = tag->text();
    if (m_removals.contains(text))
    {
        return restoreTag(text);
    }

    if (m_image->tags.contains(text) || m_additions.contains(text))
    {
        setErrorMessage({});
        return true;  // the image has it already
    }

    // In a single-value namespace the newest addition wins, as on the server.
    QStringList additions = m_additions;
    if (m_singleValueNamespaces.contains(tag->namespaceName))
    {
        additions.removeIf(
            [&](const QString& added) { return tagFromServer(added).namespaceName == tag->namespaceName; });
    }

    additions.append(text);
    return applyIfAllowed(additions, m_removals);
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
    if (m_singleValueNamespaces.contains(namespaceName))
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
    return !m_additions.isEmpty() || !m_removals.isEmpty();
}

bool TagEditor::isSaving() const
{
    return m_saving;
}

QVariantList TagEditor::tagGroups() const
{
    if (!m_image.has_value())
    {
        return {};
    }

    const QStringList after = tagsAfter(m_additions, m_removals);
    QList<QPair<TagName, Change>> entries;
    for (const QString& text : m_image->tags)
    {
        entries.append({tagFromServer(text), after.contains(text) ? Change::Kept : Change::Removed});
    }

    for (const QString& text : m_additions)
    {
        entries.append({tagFromServer(text), Change::Added});
    }

    std::ranges::stable_sort(entries, [](const auto& first, const auto& second) {
        return std::pair(first.first.namespaceName, first.first.name) <
               std::pair(second.first.namespaceName, second.first.name);
    });

    QVariantList groups;
    QString groupNamespace;
    QVariantList groupTags;
    const auto finishGroup = [&]() {
        if (!groupTags.isEmpty())
        {
            groups.append(
                QVariantMap{{QStringLiteral("namespace"), groupNamespace}, {QStringLiteral("tags"), groupTags}});
        }
    };

    for (const auto& [tag, change] : entries)
    {
        if (tag.namespaceName != groupNamespace)
        {
            finishGroup();
            groupNamespace = tag.namespaceName;
            groupTags.clear();
        }

        groupTags.append(tagEntry(tag, change));
    }

    finishGroup();
    return groups;
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
    Q_EMIT changesChanged();
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
        const std::optional<SearchTerm> term = SearchTerm::parse(text, m_namespaceNames);
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

QStringList TagEditor::tagsAfter(const QStringList& additions, const QStringList& removals) const
{
    return textsOf(tagsAfterChange(
        tagNamesOf(m_image->tags), tagNamesOf(additions), tagNamesOf(removals), m_singleValueNamespaces));
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

        m_namespaceNames.clear();
        m_singleValueNamespaces.clear();
        for (const server_client::Namespace& info : *answer)
        {
            m_namespaceNames.insert(info.name);
            if (info.singleValue)
            {
                m_singleValueNamespaces.insert(info.name);
            }
        }
    });
}

void TagEditor::endEditing()
{
    m_editing = false;
    m_additions.clear();
    m_removals.clear();
    m_suggestions->setTypedText({});
    setErrorMessage({});
    Q_EMIT editingChanged();
    Q_EMIT changesChanged();
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
