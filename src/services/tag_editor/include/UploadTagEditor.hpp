/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>

#include <QAbstractListModel>
#include <QStringList>

#include "Connection.hpp"
#include "TagRowsEditor.hpp"
#include "TagSuggestions.hpp"

namespace photobook::tag_editor
{

class Namespaces;
class NamespaceRows;

// The tags of an upload, given to every file of it, laid out like the tag editors: a row per namespace, category
// and rating as a dropdown starting at "–". A tag the server would refuse (restricted, invalid, not allowed) is
// refused at once, with the reason. A restriction on dates cannot be judged before the server has read a file's
// date; the server decides those.
class UploadTagEditor : public TagRowsEditor
{
    Q_OBJECT

public:
    explicit UploadTagEditor(connection::Connection* connection, QObject* parent = nullptr);
    ~UploadTagEditor() override;

    Q_INVOKABLE void load();  // the server's namespaces and their values, e.g. when the upload page opens
    Q_INVOKABLE void clear();

    // The tags to send, with what is typed and not added yet; nothing if one of those is refused.
    std::optional<QStringList> tagsToUpload();
    void setBusy(bool busy);  // while uploading: the tags stay as they are

    bool addTag(const QString& namespaceName, const QString& typedName) override;
    bool setSingleValue(const QString& namespaceName, const QString& typedName) override;  // empty: none
    void keepValue(const QString& namespaceName) override;  // none: new files have nothing to keep
    bool removeTag(const QString& tag) override;
    bool restoreTag(const QString& tag) override;  // nothing to restore: there are no tags before

    bool isBusy() const override;
    bool hasChanges() const override;  // any tags chosen or typed
    QAbstractListModel* namespaceRows() const override;
    QString errorMessage() const override;
    TagSuggestions* suggestions() const override;

private:
    // Takes the tags if the server would allow a file with them; otherwise says why.
    bool applyIfAllowed(const QStringList& tags);
    QStringList withoutNamespace(const QString& namespaceName) const;
    void updateRows();
    void setErrorMessage(const QString& message);

    connection::Connection* m_connection;
    std::unique_ptr<TagSuggestions> m_suggestions;
    std::unique_ptr<Namespaces> m_namespaces;
    std::unique_ptr<NamespaceRows> m_rows;
    QStringList m_tags;  // as the server writes tags, e.g. "general:beach"
    bool m_busy = false;
    QString m_errorMessage;
};

}  // namespace photobook::tag_editor
