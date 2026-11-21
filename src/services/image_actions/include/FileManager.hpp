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

class FileManager
{
public:
    virtual ~FileManager() = default;

    // The file selected in its folder. The reason if no window opened.
    virtual QFuture<std::expected<void, QString>> showFile(const QString& filePath) = 0;
};

}  // namespace pictorio::image_actions
