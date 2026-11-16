/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVariantList>

#include "ApiTypes.hpp"
#include "Connection.hpp"
#include "TagSuggestions.hpp"

namespace pictorio::tag_editor
{

// Changing the tags of the library image on screen. Changes are collected while editing and sent together
// with save(); cancel() drops them. A change the server would refuse (a restricted or invalid tag) is refused
// at once, with the reason, before anything is sent; the server checks again when saving.
class TagEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool canEdit READ canEdit NOTIFY imageChanged)
    Q_PROPERTY(bool editing READ isEditing NOTIFY editingChanged)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY changesChanged)
    Q_PROPERTY(bool saving READ isSaving NOTIFY savingChanged)
    Q_PROPERTY(QVariantList tagGroups READ tagGroups NOTIFY changesChanged)
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
    // A tag as the user typed it, e.g. "Author:Sample Artist". False if refused; errorMessage says why.
    Q_INVOKABLE bool addTag(const QString& typedText);
    // A tag as the server writes it, e.g. "general:summer": an added one is dropped, a kept one marked removed.
    Q_INVOKABLE bool removeTag(const QString& tag);
    Q_INVOKABLE bool restoreTag(const QString& tag);  // a removed one is kept again
    Q_INVOKABLE void save();
    Q_INVOKABLE void cancel();

    bool canEdit() const;
    bool isEditing() const;
    bool hasChanges() const;  // also a tag typed but not added yet: save() adds it
    bool isSaving() const;
    // While editing: the tags by namespace, each with its change (a Change, as a number):
    // a list of {"namespace": "general", "tags": [{"tag": "general:beach", "name": "beach", "change": 1}]}
    QVariantList tagGroups() const;
    QString errorMessage() const;
    TagSuggestions* suggestions() const;

Q_SIGNALS:
    void imageChanged();
    void editingChanged();
    void changesChanged();
    void savingChanged();
    void errorMessageChanged();
    void saved(const server_client::Image& image);  // as the server answered: its tags are now the image's

private:
    // Applies the change if the server would allow the image's tags after it; otherwise says why.
    bool applyIfAllowed(const QStringList& additions, const QStringList& removals);
    std::optional<QString> restrictionProblem(const QStringList& additions, const QStringList& removals) const;
    QStringList tagsAfter(const QStringList& additions, const QStringList& removals) const;
    void loadNamespaces();
    void endEditing();
    void setSaving(bool saving);
    void setErrorMessage(const QString& message);

    connection::Connection* m_connection;
    std::unique_ptr<TagSuggestions> m_suggestions;
    std::optional<server_client::Image> m_image;
    bool m_editing = false;
    bool m_saving = false;
    QStringList m_additions;  // as the server writes tags, e.g. "general:beach"
    QStringList m_removals;
    QSet<QString> m_namespaceNames;
    QSet<QString> m_singleValueNamespaces;
    QString m_errorMessage;
};

}  // namespace pictorio::tag_editor
