/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>

#include "ApiTypes.hpp"
#include "Connection.hpp"
#include "TagSuggestions.hpp"

namespace pictorio::tag_editor
{

class Namespaces;
class NamespaceRows;

// Changing the tags of the library image on screen, laid out like the server's folder-import form: a row
// per namespace. A built-in one-tag namespace (category, rating) is set to a value or none; the others,
// custom ones included, get tags added and removed. Changes are collected and sent together with save(); cancel() drops
// them. A change the server would refuse (a restricted, invalid or not allowed tag) is refused at once, with the
// reason, before anything is sent; the server checks again when saving.
class TagEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool canEdit READ canEdit NOTIFY imageChanged)
    Q_PROPERTY(bool editing READ isEditing NOTIFY editingChanged)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY hasChangesChanged)
    Q_PROPERTY(bool saving READ isSaving NOTIFY savingChanged)
    Q_PROPERTY(QAbstractListModel* namespaceRows READ namespaceRows CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(pictorio::tag_editor::TagSuggestions* suggestions READ suggestions CONSTANT)

public:
    // What saving does to a tag, for showing it while editing
    enum class Change
    {
        Kept,
        Added,
        Removed,  // also a tag that an added one replaces in a single-value namespace
    };
    Q_ENUM(Change)

    // The connection is not owned and must outlive the editor.
    explicit TagEditor(connection::Connection* connection, QObject* parent = nullptr);
    ~TagEditor() override;

    // The library image on screen, or none (e.g. a local file). Ends editing; unsaved changes are dropped.
    void setImage(const std::optional<server_client::Image>& image);

    Q_INVOKABLE void startEditing();
    // A name typed in a namespace's row, e.g. "Sample Artist" for author. False if refused; errorMessage says why.
    Q_INVOKABLE bool addTag(const QString& namespaceName, const QString& typedName);
    // For a one-tag namespace: the tag it has after saving; empty: none.
    Q_INVOKABLE bool setSingleValue(const QString& namespaceName, const QString& typedName);
    // A tag as the server writes it, e.g. "general:summer": an added one is dropped, a kept one marked removed.
    Q_INVOKABLE bool removeTag(const QString& tag);
    Q_INVOKABLE bool restoreTag(const QString& tag);  // a removed one is kept again
    Q_INVOKABLE void save();
    Q_INVOKABLE void cancel();

    // What is typed in a namespace's field and not added yet. It counts as a change; save() adds it. The
    // suggestions follow the field typed in last.
    Q_INVOKABLE void setTypedName(const QString& namespaceName, const QString& text);
    Q_INVOKABLE QString typedName(const QString& namespaceName) const;

    bool canEdit() const;
    bool isEditing() const;
    bool hasChanges() const;
    bool isSaving() const;
    // While editing: a row per namespace, see NamespaceRows
    QAbstractListModel* namespaceRows() const;
    QString errorMessage() const;
    TagSuggestions* suggestions() const;

Q_SIGNALS:
    void imageChanged();
    void editingChanged();
    void hasChangesChanged();
    void typedNamesChanged();  // e.g. cleared once added
    void savingChanged();
    void errorMessageChanged();
    void saved(const server_client::Image& image);  // as the server answered: its tags are now the image's

private:
    // Applies the change if the server would allow the image's tags after it; otherwise says why.
    bool applyIfAllowed(const QStringList& additions, const QStringList& removals);
    std::optional<QString> restrictionProblem(const QStringList& additions, const QStringList& removals) const;
    QStringList tagsAfter(const QStringList& additions, const QStringList& removals) const;
    void announceHasChanges();
    bool addTypedNames();
    void updateRows();
    void clearTypedNames();
    void endEditing();
    void setSaving(bool saving);
    void setErrorMessage(const QString& message);

    connection::Connection* m_connection;
    std::unique_ptr<TagSuggestions> m_suggestions;
    std::unique_ptr<Namespaces> m_namespaces;
    std::unique_ptr<NamespaceRows> m_rows;
    std::optional<server_client::Image> m_image;
    bool m_editing = false;
    bool m_saving = false;
    bool m_hadChanges = false;  // to announce hasChanges only when it changes
    QStringList m_additions;    // as the server writes tags, e.g. "general:beach"
    QStringList m_removals;
    QHash<QString, QString> m_typedNames;  // by namespace
    QString m_errorMessage;
};

}  // namespace pictorio::tag_editor
