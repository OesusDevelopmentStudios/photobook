/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QStringList>
#include <QVariantList>

#include "Connection.hpp"
#include "TagRowsEditor.hpp"
#include "TagSuggestions.hpp"

namespace photobook::tag_editor
{

class Namespaces;

// The user's hidden-by-default filters: search terms whose images the library leaves out unless a search names
// them, e.g. "rating:nsfw" or "character:*". Kept on the server for the user. Changes are collected and sent
// together with save(); cancel() drops them. A term the server would refuse is refused at once, with the reason.
//
// It has one add field and no namespace rows, so that the chips and the add field of the tag rows work with it:
// filters are added with addTag() and an empty namespace name, and are whole terms, not names.
class HiddenFilterEditor : public TagRowsEditor
{
    Q_OBJECT

    // As the chips of the tag rows need them: {"tag", "name", "change"}, where tag and name are the filter
    Q_PROPERTY(QVariantList filters READ filters NOTIFY filtersChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool saving READ isSaving NOTIFY savingChanged)

public:
    static constexpr int maximumCount = 50;    // as the server allows
    static constexpr int maximumLength = 200;  // of one filter, as the server allows

    explicit HiddenFilterEditor(connection::Connection* connection, QObject* parent = nullptr);
    ~HiddenFilterEditor() override;

    Q_INVOKABLE void load();  // the user's filters, e.g. when the settings open; unsaved changes are dropped
    Q_INVOKABLE void save();  // also adds what is typed and not added yet
    Q_INVOKABLE void cancel();

    // namespaceName must be empty: typedName is a whole term, e.g. "rating:nsfw|questionable".
    bool addTag(const QString& namespaceName, const QString& typedName) override;
    bool setSingleValue(const QString& namespaceName, const QString& typedName) override;  // no dropdowns: false
    void keepValue(const QString& namespaceName) override;                                 // no dropdowns
    bool removeTag(const QString& filter) override;
    bool restoreTag(const QString& filter) override;

    QVariantList filters() const;
    QStringList filtersAfterChanges() const;
    bool isLoading() const;
    bool isSaving() const;
    bool isBusy() const override;
    bool hasChanges() const override;
    QAbstractListModel* namespaceRows() const override;  // none
    QString errorMessage() const override;
    TagSuggestions* suggestions() const override;

Q_SIGNALS:
    void filtersChanged();
    void loadingChanged();
    void savingChanged();
    void saved();

private:
    void setSavedFilters(const QStringList& filters);
    void changesChanged();
    void setLoading(bool loading);
    void setSaving(bool saving);
    void setErrorMessage(const QString& message);

    connection::Connection* m_connection;
    std::unique_ptr<TagSuggestions> m_suggestions;
    std::unique_ptr<Namespaces> m_namespaces;  // a filter may name a custom namespace
    QStringList m_savedFilters;
    QStringList m_additions;
    QStringList m_removals;
    bool m_loading = false;
    bool m_saving = false;
    int m_loadNumber = 0;  // answers to an older load are ignored
    QString m_errorMessage;
};

}  // namespace photobook::tag_editor
