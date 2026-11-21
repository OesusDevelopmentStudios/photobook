/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QStringList>

#include "ApiTypes.hpp"
#include "Connection.hpp"
#include "TagRowsEditor.hpp"
#include "TagSuggestions.hpp"

namespace pictorio::tag_editor
{

class Namespaces;
class NamespaceRows;

// Changing the tags of the images selected in the library grid, all at once, laid out like the single-image
// editor: a row per namespace with the tags all selected images share; a tag only some have is not shown.
// Category and rating start at "keep as is", showing the value all share if they do. Changes are collected and
// sent together with apply(), as one change for all images; clear() drops them. They stay when the selection
// changes, and apply to the images selected then. A change that would leave any selected image restricted is
// refused at once. After applying, each image is asked for again, as the server answers only how many changed.
class BulkTagEditor : public TagRowsEditor
{
    Q_OBJECT

    Q_PROPERTY(int imageCount READ imageCount NOTIFY imagesChanged)
    Q_PROPERTY(int maximumImageCount READ maximumImageCountForQml CONSTANT)
    Q_PROPERTY(bool applying READ isApplying NOTIFY applyingChanged)

public:
    static constexpr int maximumImageCount = 1000;  // the most the server changes at once

    explicit BulkTagEditor(connection::Connection* connection, QObject* parent = nullptr);
    ~BulkTagEditor() override;

    // The selected images, in the order of the results. The changes so far stay.
    void setImages(const QList<server_client::Image>& images);

    Q_INVOKABLE void apply();  // also adds what is typed and not added yet
    Q_INVOKABLE void clear();

    // For every selected image; see TagRowsEditor
    bool addTag(const QString& namespaceName, const QString& typedName) override;
    bool setSingleValue(const QString& namespaceName, const QString& typedName) override;
    void keepValue(const QString& namespaceName) override;  // "keep as is"
    bool removeTag(const QString& tag) override;            // from every image that has it
    bool restoreTag(const QString& tag) override;

    int imageCount() const;
    bool isApplying() const;
    bool isBusy() const override;
    bool hasChanges() const override;
    QAbstractListModel* namespaceRows() const override;
    QString errorMessage() const override;
    TagSuggestions* suggestions() const override;

Q_SIGNALS:
    void imagesChanged();
    void applyingChanged();
    void applied();
    void imageUpdated(const server_client::Image& image);  // one of them, as the server has it now

private:
    int maximumImageCountForQml() const;  // QML cannot read static members

    // Applies the change if every selected image stays allowed with its tags after it; otherwise says why.
    bool applyIfAllowed(const QStringList& additions, const QStringList& removals);
    std::optional<QString> restrictionProblem(const QStringList& additions, const QStringList& removals) const;
    QStringList withoutNamespace(const QStringList& tags, const QString& namespaceName) const;
    int countWith(const QString& tag) const;  // selected images that have the tag
    void fetchAgain(const QList<int>& imageIds);
    void updateRows();
    void dropChanges();
    void setApplying(bool applying);
    void setErrorMessage(const QString& message);

    connection::Connection* m_connection;
    std::unique_ptr<TagSuggestions> m_suggestions;
    std::unique_ptr<Namespaces> m_namespaces;
    std::unique_ptr<NamespaceRows> m_rows;
    QList<server_client::Image> m_images;
    bool m_applying = false;
    QStringList m_additions;  // as the server writes tags, e.g. "general:beach"
    QStringList m_removals;
    QString m_errorMessage;
};

}  // namespace pictorio::tag_editor
