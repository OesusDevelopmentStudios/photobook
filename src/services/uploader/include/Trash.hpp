/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>

#include <QFuture>
#include <QString>

namespace photobook::uploader
{

// Files can be restored from there: the uploader removes originals only this way.
class Trash
{
public:
    virtual ~Trash();

    // The reason, if the file is still where it was.
    virtual QFuture<std::expected<void, QString>> moveToTrash(const QString& filePath) = 0;
};

}  // namespace photobook::uploader
