/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "BulkTagEditor.hpp"

#include <algorithm>
#include <map>
#include <memory>

#include <KLocalizedString>
#include <QVariantMap>

#include "hlp/Restrictions.hpp"
#include "hlp/TagMessages.hpp"
#include "hlp/TagNames.hpp"
#include "impl/NamespaceRows.hpp"
#include "impl/Namespaces.hpp"

namespace
{

using pictorio::tag_editor::TagName;
using Change = pictorio::tag_editor::TagRowsEditor::Change;

QVariantMap tagEntry(const TagName& tag, Change change, int count)
{
    return {
        {QStringLiteral("tag"), tag.text()},
        {QStringLiteral("name"), tag.name},
        {QStringLiteral("change"), static_cast<int>(change)},
        {QStringLiteral("count"), count},  // selected images that have it
    };
}

}  // namespace

namespace pictorio::tag_editor
{

BulkTagEditor::BulkTagEditor(connection::Connection* connection, QObject* parent)
    : TagRowsEditor(parent)
    , m_connection(connection)
    , m_suggestions(std::make_unique<TagSuggestions>(connection))
    , m_namespaces(std::make_unique<Namespaces>(connection))
    , m_rows(std::make_unique<NamespaceRows>())
{
    connect(m_namespaces.get(), &Namespaces::changed, this, &BulkTagEditor::updateRows);
}

BulkTagEditor::~BulkTagEditor() = default;

void BulkTagEditor::setImages(const QList<server_client::Image>& images)
{
    if (m_images.isEmpty() && !images.isEmpty())
    {
        m_namespaces->load();  // a new selection: the server's namespaces as they are now
    }

    m_images = images;
    Q_EMIT imagesChanged();
    updateRows();
}

bool BulkTagEditor::addTag(const QString& namespaceName, const QString& typedName)
{
    if (m_applying || m_images.isEmpty())
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
    if (m_removals.contains(text))
    {
        added = restoreTag(text);
    }
    else if (m_additions.contains(text) || countWith(text) == imageCount())
    {
        setErrorMessage({});
        added = true;  // every image has it already
    }
    else
    {
        // In a single-value namespace the newest addition wins, as on the server.
        const bool singleValue = m_namespaces->singleValueNames().contains(namespaceName);
        const QStringList others = singleValue ? withoutNamespace(m_additions, namespaceName) : m_additions;
        added = applyIfAllowed(others + QStringList{text}, m_removals);
    }

    if (added && m_typedNames.remove(namespaceName) > 0)
    {
        m_suggestions->setTypedText({});
        Q_EMIT typedNamesChanged();
        announceHasChanges();
    }

    return added;
}

bool BulkTagEditor::setSingleValue(const QString& namespaceName, const QString& typedName)
{
    if (m_applying || m_images.isEmpty())
    {
        return false;
    }

    // None for any image: every tag of the namespace goes.
    if (typedName.trimmed().isEmpty())
    {
        QStringList removals = withoutNamespace(m_removals, namespaceName);
        for (const server_client::Image& image : std::as_const(m_images))
        {
            for (const QString& tag : image.tags)
            {
                if (tagFromServer(tag).namespaceName == namespaceName && !removals.contains(tag))
                {
                    removals.append(tag);
                }
            }
        }

        return applyIfAllowed(withoutNamespace(m_additions, namespaceName), removals);
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

    // The same for every image; one that has it already keeps it.
    QStringList additions = withoutNamespace(m_additions, namespaceName);
    if (countWith(tag->text()) < imageCount())
    {
        additions.append(tag->text());
    }

    return applyIfAllowed(additions, withoutNamespace(m_removals, namespaceName));
}

void BulkTagEditor::keepValue(const QString& namespaceName)
{
    if (!m_applying)
    {
        applyIfAllowed(withoutNamespace(m_additions, namespaceName), withoutNamespace(m_removals, namespaceName));
    }
}

bool BulkTagEditor::removeTag(const QString& tag)
{
    if (m_applying)
    {
        return false;
    }

    if (m_additions.contains(tag))
    {
        QStringList additions = m_additions;
        additions.removeAll(tag);
        return applyIfAllowed(additions, m_removals);
    }

    if (countWith(tag) > 0 && !m_removals.contains(tag))
    {
        return applyIfAllowed(m_additions, m_removals + QStringList{tag});
    }

    return false;
}

bool BulkTagEditor::restoreTag(const QString& tag)
{
    if (m_applying)
    {
        return false;
    }

    QStringList removals = m_removals;
    removals.removeAll(tag);

    // A tag replaced by an addition in a single-value namespace stays when that addition goes.
    const QString namespaceName = tagFromServer(tag).namespaceName;
    const bool singleValue = m_namespaces->singleValueNames().contains(namespaceName);
    return applyIfAllowed(singleValue ? withoutNamespace(m_additions, namespaceName) : m_additions, removals);
}

void BulkTagEditor::apply()
{
    if (m_applying || m_images.isEmpty())
    {
        return;
    }

    if (imageCount() > maximumImageCount)
    {
        setErrorMessage(i18nc("@info %1 is the most images, %2 how many are selected",
                              "At most %1 images can be changed at once; %2 are selected.",
                              maximumImageCount,
                              imageCount()));
        return;
    }

    // Typed, not added yet: added now, as the user expects it applied. Refused: nothing is sent.
    if (!addTypedNames() || !hasChanges())
    {
        return;
    }

    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        setErrorMessage(i18nc("@info", "Not connected to the server."));
        return;
    }

    QList<int> imageIds;
    for (const server_client::Image& image : std::as_const(m_images))
    {
        imageIds.append(image.id);
    }

    setErrorMessage({});
    setApplying(true);
    client->changeTagsOfImages(imageIds, server_client::TagChanges{m_additions, m_removals})
        .then(this, [this, imageIds](const server_client::Result<int>& answer) {
            setApplying(false);
            if (!answer.has_value())
            {
                setErrorMessage(describeError(answer.error()));  // the changes stay, to fix and apply again
                return;
            }

            dropChanges();
            Q_EMIT applied();
            fetchAgain(imageIds);
        });
}

void BulkTagEditor::clear()
{
    if (!m_applying)
    {
        dropChanges();
    }
}

void BulkTagEditor::setTypedName(const QString& namespaceName, const QString& text)
{
    if (m_typedNames.value(namespaceName) == text)
    {
        return;
    }

    m_typedNames.insert(namespaceName, text);
    m_suggestions->setNamespaceName(namespaceName);
    m_suggestions->setTypedText(text);
    announceHasChanges();
}

QString BulkTagEditor::typedName(const QString& namespaceName) const
{
    return m_typedNames.value(namespaceName);
}

int BulkTagEditor::imageCount() const
{
    return static_cast<int>(m_images.size());
}

bool BulkTagEditor::hasChanges() const
{
    const bool somethingTyped =
        std::ranges::any_of(m_typedNames, [](const QString& text) { return !text.trimmed().isEmpty(); });
    return !m_additions.isEmpty() || !m_removals.isEmpty() || somethingTyped;
}

bool BulkTagEditor::isApplying() const
{
    return m_applying;
}

bool BulkTagEditor::isBusy() const
{
    return m_applying;
}

QAbstractListModel* BulkTagEditor::namespaceRows() const
{
    return m_rows.get();
}

QString BulkTagEditor::errorMessage() const
{
    return m_errorMessage;
}

TagSuggestions* BulkTagEditor::suggestions() const
{
    return m_suggestions.get();
}

int BulkTagEditor::maximumImageCountForQml() const
{
    return maximumImageCount;
}

bool BulkTagEditor::applyIfAllowed(const QStringList& additions, const QStringList& removals)
{
    if (const std::optional<QString> problem = restrictionProblem(additions, removals))
    {
        setErrorMessage(*problem);
        return false;
    }

    m_additions = additions;
    m_removals = removals;
    setErrorMessage({});
    updateRows();
    announceHasChanges();
    return true;
}

// As the server checks it: every image must stay visible to the user with its tags after the change.
std::optional<QString> BulkTagEditor::restrictionProblem(const QStringList& additions,
                                                         const QStringList& removals) const
{
    const QList<TagName> added = tagNamesOf(additions);
    const QList<TagName> removed = tagNamesOf(removals);
    QStringList offending;
    for (const server_client::Image& image : m_images)
    {
        const QList<TagName> after =
            tagsAfterChange(tagNamesOf(image.tags), added, removed, m_namespaces->singleValueNames());
        offending += restrictedTagsAmong(m_connection->restrictedTags(), m_namespaces->names(), after, image.takenAt);
    }

    if (offending.isEmpty())
    {
        return std::nullopt;
    }

    offending.removeDuplicates();
    offending.sort();
    return restrictedMessage(offending);
}

QStringList BulkTagEditor::withoutNamespace(const QStringList& tags, const QString& namespaceName) const
{
    QStringList others = tags;
    others.removeIf([&](const QString& tag) { return tagFromServer(tag).namespaceName == namespaceName; });
    return others;
}

int BulkTagEditor::countWith(const QString& tag) const
{
    return static_cast<int>(
        std::ranges::count_if(m_images, [&](const server_client::Image& image) { return image.tags.contains(tag); }));
}

bool BulkTagEditor::addTypedNames()
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

// The server answers a bulk change only with how many images changed: each one is asked for again. The rows
// follow once all have arrived.
void BulkTagEditor::fetchAgain(const QList<int>& imageIds)
{
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        return;
    }

    auto stillToCome = std::make_shared<qsizetype>(imageIds.size());
    for (const int imageId : imageIds)
    {
        client->image(imageId).then(
            this, [this, stillToCome](const server_client::Result<server_client::Image>& answer) {
                if (answer.has_value())
                {
                    const auto shown = std::ranges::find(m_images, answer->id, &server_client::Image::id);
                    if (shown != m_images.end())
                    {
                        *shown = *answer;
                    }

                    Q_EMIT imageUpdated(*answer);
                }

                if (--*stillToCome == 0)
                {
                    updateRows();
                }
            });
    }
}

void BulkTagEditor::updateRows()
{
    if (m_images.isEmpty())
    {
        m_rows->setRows({});
        return;
    }

    const QSet<QString> singleValue = m_namespaces->singleValueNames();
    QList<NamespaceRows::Row> rows;
    for (const Namespaces::Info& info : m_namespaces->all())
    {
        // The tags any selected image has in the namespace, and those to be added, by name. In a single-value
        // namespace an added tag replaces the others.
        const bool addingHere = withoutNamespace(m_additions, info.name).size() < m_additions.size();
        const bool replacing = addingHere && singleValue.contains(info.name);
        std::map<QString, Change> changes;
        for (const server_client::Image& image : m_images)
        {
            for (const QString& text : image.tags)
            {
                if (tagFromServer(text).namespaceName == info.name)
                {
                    const bool replaced = replacing && !m_additions.contains(text);
                    changes[text] = m_removals.contains(text) || replaced ? Change::Removed : Change::Kept;
                }
            }
        }

        QString addedValue;
        for (const QString& text : m_additions)
        {
            if (tagFromServer(text).namespaceName == info.name)
            {
                changes[text] = Change::Added;
                addedValue = tagFromServer(text).name;
            }
        }

        QVariantList tags;
        QStringList presentNames;
        bool allRemoved = true;
        for (const auto& [text, change] : changes)
        {
            const TagName tag = tagFromServer(text);
            const int count = countWith(text);
            tags.append(tagEntry(tag, change, count));
            if (count > 0)
            {
                presentNames.append(tag.name);
                allRemoved = allRemoved && change == Change::Removed;
            }
        }

        NamespaceRows::Row row;
        row.namespaceName = info.name;
        row.dropdown = info.hasDropdown();
        row.fixedValues = info.allowedValues.has_value();
        row.keepable = true;
        row.tags = tags;
        if (row.dropdown)
        {
            row.options = info.allowedValues.value_or(m_namespaces->knownValues(info.name));
            for (const QString& name : presentNames + QStringList{addedValue})
            {
                if (!name.isEmpty() && !row.options.contains(name))
                {
                    row.options.append(name);
                }
            }

            // A value for all, none for all, or kept as is
            row.value = addedValue;
            row.changed = !addedValue.isEmpty() || (!presentNames.isEmpty() && allRemoved);
        }

        rows.append(row);
    }

    m_rows->setRows(rows);
}

void BulkTagEditor::dropChanges()
{
    m_additions.clear();
    m_removals.clear();
    if (!m_typedNames.isEmpty())
    {
        m_typedNames.clear();
        Q_EMIT typedNamesChanged();
    }

    m_suggestions->setTypedText({});
    setErrorMessage({});
    updateRows();
    announceHasChanges();
}

void BulkTagEditor::announceHasChanges()
{
    const bool changes = hasChanges();
    if (changes != m_hadChanges)
    {
        m_hadChanges = changes;
        Q_EMIT hasChangesChanged();
    }
}

void BulkTagEditor::setApplying(bool applying)
{
    if (applying != m_applying)
    {
        m_applying = applying;
        Q_EMIT applyingChanged();
        Q_EMIT busyChanged();
    }
}

void BulkTagEditor::setErrorMessage(const QString& message)
{
    if (message != m_errorMessage)
    {
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }
}

}  // namespace pictorio::tag_editor
