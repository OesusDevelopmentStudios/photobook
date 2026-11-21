/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QStringList>

#include "TagRowsEditor.hpp"

namespace photobook::tests
{

// Records what the rows of an edit view ask the editor to do, e.g. "removeTag general:beach", and agrees to all.
// Registered for the QML tests in TestQml.cpp.
class FakeTagRowsEditor : public tag_editor::TagRowsEditor
{
    Q_OBJECT

    Q_PROPERTY(QStringList calls MEMBER m_calls NOTIFY callsChanged)

public:
    using TagRowsEditor::TagRowsEditor;

    bool addTag(const QString& namespaceName, const QString& typedName) override
    {
        record(QStringLiteral("addTag %1 %2").arg(namespaceName, typedName));
        return true;
    }

    bool setSingleValue(const QString& namespaceName, const QString& typedName) override
    {
        record(QStringLiteral("setSingleValue %1 %2").arg(namespaceName, typedName).trimmed());
        return true;
    }

    void keepValue(const QString& namespaceName) override
    {
        record(QStringLiteral("keepValue %1").arg(namespaceName));
    }

    bool removeTag(const QString& tag) override
    {
        record(QStringLiteral("removeTag %1").arg(tag));
        return true;
    }

    bool restoreTag(const QString& tag) override
    {
        record(QStringLiteral("restoreTag %1").arg(tag));
        return true;
    }

    bool isBusy() const override
    {
        return false;
    }

    bool hasChanges() const override
    {
        return false;
    }

    QAbstractListModel* namespaceRows() const override
    {
        return nullptr;
    }

    QString errorMessage() const override
    {
        return {};
    }

    tag_editor::TagSuggestions* suggestions() const override
    {
        return nullptr;
    }

Q_SIGNALS:
    void callsChanged();

private:
    void record(const QString& call)
    {
        m_calls.append(call);
        Q_EMIT callsChanged();
    }

    QStringList m_calls;
};

}  // namespace photobook::tests
