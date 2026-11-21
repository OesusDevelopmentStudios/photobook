/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "HiddenFilterEditor.hpp"

#include <KLocalizedString>
#include <QVariantMap>

#include "hlp/SearchTerm.hpp"
#include "hlp/TagMessages.hpp"
#include "impl/Namespaces.hpp"

namespace
{

using photobook::tag_editor::HiddenFilterEditor;

QVariantMap filterEntry(const QString& filter, HiddenFilterEditor::Change change)
{
    return {
        {QStringLiteral("tag"), filter},
        {QStringLiteral("name"), filter},
        {QStringLiteral("change"), static_cast<int>(change)},  // a number, which QML compares with TagRowsEditor.Change
    };
}

}  // namespace

namespace photobook::tag_editor
{

HiddenFilterEditor::HiddenFilterEditor(connection::Connection* connection, QObject* parent)
    : TagRowsEditor(parent)
    , m_connection(connection)
    , m_suggestions(std::make_unique<TagSuggestions>(connection))
    , m_namespaces(std::make_unique<Namespaces>(connection))
{}

HiddenFilterEditor::~HiddenFilterEditor() = default;

void HiddenFilterEditor::load()
{
    if (m_saving)
    {
        return;
    }

    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        setErrorMessage(i18nc("@info", "Not connected to the server."));
        return;
    }

    m_additions.clear();
    m_removals.clear();
    clearTypedNames();
    setErrorMessage({});
    setLoading(true);
    m_namespaces->load();
    const int loadNumber = ++m_loadNumber;
    client->preferences().then(this,
                               [this, loadNumber](const server_client::Result<server_client::Preferences>& answer) {
                                   if (loadNumber != m_loadNumber)
                                   {
                                       return;
                                   }

                                   setLoading(false);
                                   if (!answer.has_value())
                                   {
                                       setErrorMessage(describeError(answer.error()));
                                       return;
                                   }

                                   setSavedFilters(answer->hiddenFilters);
                               });
    changesChanged();
}

void HiddenFilterEditor::save()
{
    if (isBusy())
    {
        return;
    }

    // Typed, not added yet: added now, as the user expects it saved. Refused: nothing is saved.
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

    setErrorMessage({});
    setSaving(true);
    client->setPreferences(server_client::Preferences{filtersAfterChanges()})
        .then(this, [this](const server_client::Result<server_client::Preferences>& answer) {
            setSaving(false);
            if (!answer.has_value())
            {
                setErrorMessage(describeError(answer.error()));  // the changes stay, to fix and save again
                return;
            }

            setSavedFilters(answer->hiddenFilters);
            Q_EMIT saved();
        });
}

void HiddenFilterEditor::cancel()
{
    if (isBusy())
    {
        return;
    }

    m_additions.clear();
    m_removals.clear();
    clearTypedNames();
    setErrorMessage({});
    changesChanged();
}

bool HiddenFilterEditor::addTag(const QString& namespaceName, const QString& typedName)
{
    const QString filter = typedName.trimmed();
    if (isBusy() || !namespaceName.isEmpty() || filter.isEmpty())
    {
        return false;
    }

    // The same checks as the server's, so that Save is not refused for them.
    if (filter.startsWith(QLatin1Char('-')))
    {
        setErrorMessage(i18nc("@info", "Write the filter without a leading \"-\"."));
        return false;
    }

    if (filter.size() > maximumLength)
    {
        setErrorMessage(i18nc("@info", "A filter can be at most %1 characters long.", maximumLength));
        return false;
    }

    if (!SearchTerm::parse(filter, m_namespaces->names()).has_value())
    {
        setErrorMessage(i18nc("@info %1 is what the user typed",
                              "\"%1\" is not one search term, such as rating:nsfw, rating:nsfw|questionable or "
                              "character:*.",
                              filter));
        return false;
    }

    if (m_removals.contains(filter))
    {
        m_removals.removeAll(filter);
    }
    else if (!m_savedFilters.contains(filter) && !m_additions.contains(filter))
    {
        if (filtersAfterChanges().size() >= maximumCount)
        {
            setErrorMessage(i18nc("@info", "At most %1 filters can be hidden.", maximumCount));
            return false;
        }

        m_additions.append(filter);
    }

    setErrorMessage({});
    forgetTypedName(namespaceName);
    changesChanged();
    return true;
}

bool HiddenFilterEditor::setSingleValue(const QString& /*namespaceName*/, const QString& /*typedName*/)
{
    return false;
}

void HiddenFilterEditor::keepValue(const QString& /*namespaceName*/) {}

bool HiddenFilterEditor::removeTag(const QString& filter)
{
    if (isBusy())
    {
        return false;
    }

    if (m_additions.contains(filter))
    {
        m_additions.removeAll(filter);
    }
    else if (m_savedFilters.contains(filter) && !m_removals.contains(filter))
    {
        m_removals.append(filter);
    }
    else
    {
        return false;
    }

    setErrorMessage({});
    changesChanged();
    return true;
}

bool HiddenFilterEditor::restoreTag(const QString& filter)
{
    if (isBusy() || !m_removals.contains(filter))
    {
        return false;
    }

    m_removals.removeAll(filter);
    setErrorMessage({});
    changesChanged();
    return true;
}

// The saved ones in the server's order, then the added ones
QVariantList HiddenFilterEditor::filters() const
{
    QVariantList entries;
    for (const QString& filter : m_savedFilters)
    {
        entries.append(filterEntry(filter, m_removals.contains(filter) ? Change::Removed : Change::Kept));
    }

    for (const QString& filter : m_additions)
    {
        entries.append(filterEntry(filter, Change::Added));
    }

    return entries;
}

QStringList HiddenFilterEditor::filtersAfterChanges() const
{
    QStringList after = m_savedFilters;
    after.removeIf([this](const QString& filter) { return m_removals.contains(filter); });
    return after + m_additions;
}

bool HiddenFilterEditor::isLoading() const
{
    return m_loading;
}

bool HiddenFilterEditor::isSaving() const
{
    return m_saving;
}

bool HiddenFilterEditor::isBusy() const
{
    return m_loading || m_saving;
}

bool HiddenFilterEditor::hasChanges() const
{
    return !m_additions.isEmpty() || !m_removals.isEmpty() || hasTypedNames();
}

QAbstractListModel* HiddenFilterEditor::namespaceRows() const
{
    return nullptr;
}

QString HiddenFilterEditor::errorMessage() const
{
    return m_errorMessage;
}

TagSuggestions* HiddenFilterEditor::suggestions() const
{
    return m_suggestions.get();
}

void HiddenFilterEditor::setSavedFilters(const QStringList& filters)
{
    m_savedFilters = filters;
    m_additions.clear();
    m_removals.clear();
    changesChanged();
}

// After any change to the filters: the chips, the suggestions and hasChanges follow.
void HiddenFilterEditor::changesChanged()
{
    m_suggestions->setExcludedTags(filtersAfterChanges());
    Q_EMIT filtersChanged();
    announceHasChanges();
}

void HiddenFilterEditor::setLoading(bool loading)
{
    if (loading != m_loading)
    {
        m_loading = loading;
        Q_EMIT loadingChanged();
        Q_EMIT busyChanged();
    }
}

void HiddenFilterEditor::setSaving(bool saving)
{
    if (saving != m_saving)
    {
        m_saving = saving;
        Q_EMIT savingChanged();
        Q_EMIT busyChanged();
    }
}

void HiddenFilterEditor::setErrorMessage(const QString& message)
{
    if (message != m_errorMessage)
    {
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }
}

}  // namespace photobook::tag_editor
