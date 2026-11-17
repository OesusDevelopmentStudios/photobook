/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <expected>

#include <QFuture>
#include <QString>

namespace pictorio::image_actions
{

// The user's file manager, e.g. Dolphin
class FileManager
{
public:
    virtual ~FileManager() = default;

    // A window of the file's folder with the file selected. The reason if none opened.
    virtual QFuture<std::expected<void, QString>> showFile(const QString& filePath) = 0;
};

}  // namespace pictorio::image_actions
