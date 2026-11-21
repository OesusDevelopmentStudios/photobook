/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QString>

#include "TagSuggestions.hpp"

namespace photobook::tag_editor
{

// What an edit view's rows use, the same for one image and for many: a row per namespace, with its dropdown
// or its tags and add field. The QML parts of a row work with either editor.
class TagRowsEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)  // sending the changes: no more changes meanwhile
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY hasChangesChanged)
    Q_PROPERTY(QAbstractListModel* namespaceRows READ namespaceRows CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(photobook::tag_editor::TagSuggestions* suggestions READ suggestions CONSTANT)

public:
    enum class Change
    {
        Kept,
        Added,
        Removed,  // also a tag that an added one replaces in a single-value namespace
    };
    Q_ENUM(Change)

    using QObject::QObject;
    ~TagRowsEditor() override;

    // A name typed in a namespace's row, e.g. "Sample Artist" for author. False if refused; errorMessage says why.
    Q_INVOKABLE virtual bool addTag(const QString& namespaceName, const QString& typedName) = 0;
    // For category or rating: this value after the changes; empty: none.
    Q_INVOKABLE virtual bool setSingleValue(const QString& namespaceName, const QString& typedName) = 0;
    Q_INVOKABLE virtual void keepValue(const QString& namespaceName) = 0;  // no change to that namespace
    // A tag as the server writes it, e.g. "general:summer": an added one is dropped, another marked removed.
    Q_INVOKABLE virtual bool removeTag(const QString& tag) = 0;
    Q_INVOKABLE virtual bool restoreTag(const QString& tag) = 0;  // a removed one is kept after all

    // What is typed in a namespace's field and not added yet. It counts as a change, and is added when the
    // changes are sent. The suggestions follow the field typed in last.
    Q_INVOKABLE void setTypedName(const QString& namespaceName, const QString& text);
    Q_INVOKABLE QString typedName(const QString& namespaceName) const;

    virtual bool isBusy() const = 0;
    virtual bool hasChanges() const = 0;
    virtual QAbstractListModel* namespaceRows() const = 0;  // a row per namespace, see NamespaceRows
    virtual QString errorMessage() const = 0;
    virtual TagSuggestions* suggestions() const = 0;  // may be none, e.g. in tests

Q_SIGNALS:
    void busyChanged();
    void hasChangesChanged();
    void errorMessageChanged();
    void typedNamesChanged();  // e.g. cleared once added

protected:
    bool hasTypedNames() const;  // anything but spaces
    // Adds what is typed in each field, with addTag(); stops at the first that is refused, which stays typed.
    bool addTypedNames();
    void forgetTypedName(const QString& namespaceName);  // it was added: its field empties
    void clearTypedNames();
    // Tells of hasChanges only when it changes; call after anything that may change it.
    void announceHasChanges();

private:
    void clearSuggestions();

    QHash<QString, QString> m_typedNames;  // by namespace
    bool m_hadChanges = false;
};

}  // namespace photobook::tag_editor
