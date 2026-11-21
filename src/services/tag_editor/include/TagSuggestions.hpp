/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <chrono>

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QTimer>

#include "ApiTypes.hpp"
#include "Connection.hpp"

namespace pictorio::tag_editor
{

// Tags that start with what the user is typing, most used first, for the list under a tag field. The server
// is asked once typing pauses, not at every key, and never suggests the user's restricted tags.
class TagSuggestions : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString namespaceName READ namespaceName WRITE setNamespaceName NOTIFY namespaceNameChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role
    {
        TagRole = Qt::UserRole + 1,  // e.g. "author:sample_artist"
        NameRole,                    // without the namespace, e.g. "sample_artist"
        ImageCountRole,
    };

    static constexpr int maximumCount = 10;
    static constexpr std::chrono::milliseconds typingPause{250};

    // The connection is not owned and must outlive the suggestions.
    explicit TagSuggestions(connection::Connection* connection, QObject* parent = nullptr);

    void setTypedText(const QString& text);  // empty: no suggestions, at once

    // Empty: tags in any namespace
    QString namespaceName() const;
    void setNamespaceName(const QString& namespaceName);

    // Tags not to suggest, e.g. those the image already has ("general:summer")
    void setExcludedTags(const QStringList& tags);

    int count() const;
    Q_INVOKABLE QString nameAt(int row) const;  // without the namespace; empty for a row that is not there
    Q_INVOKABLE QString tagAt(int row) const;   // with the namespace, e.g. "rating:nsfw"; empty likewise
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void namespaceNameChanged();
    void countChanged();

private:
    void askServer();
    void setSuggestions(const QList<server_client::TagCount>& suggestions);

    connection::Connection* m_connection;
    QTimer m_typingPause;
    QString m_typedText;
    QString m_namespaceName;
    QStringList m_excludedTags;
    QList<server_client::TagCount> m_suggestions;
    int m_askNumber = 0;  // answers to older typing are ignored
};

}  // namespace pictorio::tag_editor
