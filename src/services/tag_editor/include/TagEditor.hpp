/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>
#include <optional>

#include <QAbstractListModel>
#include <QCache>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>

#include "ApiTypes.hpp"
#include "Connection.hpp"
#include "TagRowsEditor.hpp"
#include "TagSuggestions.hpp"

namespace photobook::tag_editor
{

class Namespaces;
class NamespaceRows;

// The tags of the library image on screen, for the viewer's tag pane, and changing them. Shown, there is a row for
// each namespace the image has tags in, in the server's order with those only the server sets (meta) last, each tag
// with how many images have it. While editing, there is a row for each namespace users tag with. A built-in
// one-tag namespace (category, rating) is set to a value or none; the others, custom ones included, get tags added
// and removed. Changes are collected and sent together with save(); cancel() drops them. A change the server would
// refuse (a restricted, invalid or not allowed tag) is refused at once, with the reason, before anything is sent; the
// server checks again when saving.
class TagEditor : public TagRowsEditor
{
    Q_OBJECT

    Q_PROPERTY(bool canEdit READ canEdit NOTIFY imageChanged)
    Q_PROPERTY(bool editing READ isEditing NOTIFY editingChanged)
    Q_PROPERTY(bool saving READ isSaving NOTIFY savingChanged)

public:
    explicit TagEditor(connection::Connection* connection, QObject* parent = nullptr);
    ~TagEditor() override;

    // The library image on screen, or none (e.g. a local file). Ends editing; unsaved changes are dropped.
    void setImage(const std::optional<server_client::Image>& image);

    Q_INVOKABLE void startEditing();
    Q_INVOKABLE void save();  // also adds what is typed and not added yet
    Q_INVOKABLE void cancel();

    bool addTag(const QString& namespaceName, const QString& typedName) override;
    bool setSingleValue(const QString& namespaceName, const QString& typedName) override;
    void keepValue(const QString& namespaceName) override;  // the image's own value again
    bool removeTag(const QString& tag) override;
    bool restoreTag(const QString& tag) override;

    bool canEdit() const;
    bool isEditing() const;
    bool isSaving() const;
    bool isBusy() const override;
    bool hasChanges() const override;
    QAbstractListModel* namespaceRows() const override;  // each tag also has its "count"; -1 until known
    QString errorMessage() const override;
    TagSuggestions* suggestions() const override;

Q_SIGNALS:
    void imageChanged();
    void editingChanged();
    void savingChanged();
    void saved(const server_client::Image& image);  // as the server answered: its tags are now the image's

private:
    // Applies the change if the server would allow the image's tags after it; otherwise says why.
    bool applyIfAllowed(const QStringList& additions, const QStringList& removals);
    std::optional<QString> restrictionProblem(const QStringList& additions, const QStringList& removals) const;
    QStringList tagsAfter(const QStringList& additions, const QStringList& removals) const;
    void updateRows();
    void showImageTags();  // the rows while not editing
    void loadCounts(int imageId);
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
    QStringList m_additions;  // as the server writes tags, e.g. "general:beach"
    QStringList m_removals;
    QString m_errorMessage;
    QCache<int, QHash<QString, int>> m_counts{500};  // by image id: each tag's image count, e.g. stepping back
};

}  // namespace photobook::tag_editor
