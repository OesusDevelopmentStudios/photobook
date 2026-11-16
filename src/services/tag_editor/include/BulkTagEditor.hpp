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
#include "TagSuggestions.hpp"

namespace pictorio::tag_editor
{

class Namespaces;
class NamespaceRows;

// Changing the tags of the images selected in the library grid, all at once, laid out like the single-image
// editor: a row per namespace, with how many selected images have each tag. Category and rating start at
// "keep as is". Changes are collected and sent together with apply(), as one change for all images; clear()
// drops them. They stay when the selection changes, and apply to the images selected then. A change that
// would leave any selected image restricted is refused at once. After applying, each image is asked for
// again, as the server answers only how many changed.
class BulkTagEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int imageCount READ imageCount NOTIFY imagesChanged)
    Q_PROPERTY(int maximumImageCount READ maximumImageCountForQml CONSTANT)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY hasChangesChanged)
    Q_PROPERTY(bool applying READ isApplying NOTIFY applyingChanged)
    Q_PROPERTY(QAbstractListModel* namespaceRows READ namespaceRows CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(pictorio::tag_editor::TagSuggestions* suggestions READ suggestions CONSTANT)

public:
    static constexpr int maximumImageCount = 1000;  // the most the server changes at once

    // The connection is not owned and must outlive the editor.
    explicit BulkTagEditor(connection::Connection* connection, QObject* parent = nullptr);
    ~BulkTagEditor() override;

    // The selected images, in the order of the results. The changes so far stay.
    void setImages(const QList<server_client::Image>& images);

    // A name typed in a namespace's row; added to every selected image. False if refused; errorMessage says why.
    Q_INVOKABLE bool addTag(const QString& namespaceName, const QString& typedName);
    // For category or rating: this value for every image; empty: none for any.
    Q_INVOKABLE bool setSingleValue(const QString& namespaceName, const QString& typedName);
    Q_INVOKABLE void keepValue(const QString& namespaceName);  // "keep as is": no change to that namespace
    // A tag as the server writes it: removed from every image that has it, or an added one dropped
    Q_INVOKABLE bool removeTag(const QString& tag);
    Q_INVOKABLE bool restoreTag(const QString& tag);  // not removed after all
    Q_INVOKABLE void apply();
    Q_INVOKABLE void clear();

    // What is typed in a namespace's field and not added yet; it counts as a change, and apply() adds it.
    Q_INVOKABLE void setTypedName(const QString& namespaceName, const QString& text);
    Q_INVOKABLE QString typedName(const QString& namespaceName) const;

    int imageCount() const;
    bool hasChanges() const;
    bool isApplying() const;
    QAbstractListModel* namespaceRows() const;
    QString errorMessage() const;
    TagSuggestions* suggestions() const;

Q_SIGNALS:
    void imagesChanged();
    void hasChangesChanged();
    void typedNamesChanged();  // e.g. cleared once added
    void applyingChanged();
    void errorMessageChanged();
    void applied();                                        // the server has changed the images
    void imageUpdated(const server_client::Image& image);  // one of them, as the server has it now

private:
    int maximumImageCountForQml() const;  // QML cannot read static members

    // Applies the change if every selected image stays allowed with its tags after it; otherwise says why.
    bool applyIfAllowed(const QStringList& additions, const QStringList& removals);
    std::optional<QString> restrictionProblem(const QStringList& additions, const QStringList& removals) const;
    QStringList withoutNamespace(const QStringList& tags, const QString& namespaceName) const;
    int countWith(const QString& tag) const;  // selected images that have the tag
    bool addTypedNames();
    void fetchAgain(const QList<int>& imageIds);
    void updateRows();
    void dropChanges();
    void announceHasChanges();
    void setApplying(bool applying);
    void setErrorMessage(const QString& message);

    connection::Connection* m_connection;
    std::unique_ptr<TagSuggestions> m_suggestions;
    std::unique_ptr<Namespaces> m_namespaces;
    std::unique_ptr<NamespaceRows> m_rows;
    QList<server_client::Image> m_images;
    bool m_applying = false;
    bool m_hadChanges = false;  // to announce hasChanges only when it changes
    QStringList m_additions;    // as the server writes tags, e.g. "general:beach"
    QStringList m_removals;
    QHash<QString, QString> m_typedNames;  // by namespace
    QString m_errorMessage;
};

}  // namespace pictorio::tag_editor
