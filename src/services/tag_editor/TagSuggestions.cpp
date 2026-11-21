/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TagSuggestions.hpp"

namespace
{

// More than shown, so some are left after the excluded ones are taken out
constexpr int askedCount = 20;

}  // namespace

namespace photobook::tag_editor
{

TagSuggestions::TagSuggestions(connection::Connection* connection, QObject* parent)
    : QAbstractListModel(parent)
    , m_connection(connection)
{
    m_typingPause.setSingleShot(true);
    m_typingPause.setInterval(typingPause);
    connect(&m_typingPause, &QTimer::timeout, this, &TagSuggestions::askServer);
}

void TagSuggestions::setTypedText(const QString& text)
{
    if (text == m_typedText)
    {
        return;
    }

    m_typedText = text;
    ++m_askNumber;  // an answer still on its way is for the old text
    if (text.trimmed().isEmpty())
    {
        m_typingPause.stop();
        setSuggestions({});
    }
    else
    {
        m_typingPause.start();
    }
}

QString TagSuggestions::namespaceName() const
{
    return m_namespaceName;
}

// The old suggestions are for another namespace: gone at once, and the typed text is asked for again.
void TagSuggestions::setNamespaceName(const QString& namespaceName)
{
    if (namespaceName == m_namespaceName)
    {
        return;
    }

    m_namespaceName = namespaceName;
    ++m_askNumber;
    setSuggestions({});
    if (!m_typedText.trimmed().isEmpty())
    {
        m_typingPause.start();
    }

    Q_EMIT namespaceNameChanged();
}

void TagSuggestions::setExcludedTags(const QStringList& tags)
{
    m_excludedTags = tags;
}

int TagSuggestions::count() const
{
    return static_cast<int>(m_suggestions.size());
}

QString TagSuggestions::nameAt(int row) const
{
    return row >= 0 && row < count() ? data(index(row), NameRole).toString() : QString();
}

QString TagSuggestions::tagAt(int row) const
{
    return row >= 0 && row < count() ? data(index(row), TagRole).toString() : QString();
}

int TagSuggestions::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant TagSuggestions::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
    {
        return {};
    }

    const server_client::TagCount& suggestion = m_suggestions.at(index.row());
    switch (role)
    {
    case TagRole:
    case Qt::DisplayRole:
        return suggestion.tag;
    case NameRole:
        return suggestion.tag.mid(suggestion.tag.indexOf(QLatin1Char(':')) + 1);
    case ImageCountRole:
        return suggestion.imageCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> TagSuggestions::roleNames() const
{
    return {{TagRole, "tag"}, {NameRole, "name"}, {ImageCountRole, "imageCount"}};
}

// Without an answer there are no suggestions; typing on works as before.
void TagSuggestions::askServer()
{
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        return;
    }

    const int askNumber = m_askNumber;
    client->tags(m_typedText, m_namespaceName, askedCount)
        .then(this, [this, askNumber](const server_client::Result<QList<server_client::TagCount>>& answer) {
            if (askNumber == m_askNumber)
            {
                setSuggestions(answer.value_or(QList<server_client::TagCount>()));
            }
        });
}

void TagSuggestions::setSuggestions(const QList<server_client::TagCount>& suggestions)
{
    QList<server_client::TagCount> shown;
    for (const server_client::TagCount& suggestion : suggestions)
    {
        if (shown.size() < maximumCount && !m_excludedTags.contains(suggestion.tag))
        {
            shown.append(suggestion);
        }
    }

    if (shown.isEmpty() && m_suggestions.isEmpty())
    {
        return;
    }

    beginResetModel();
    m_suggestions = shown;
    endResetModel();
    Q_EMIT countChanged();
}

}  // namespace photobook::tag_editor
