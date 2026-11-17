/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include "FileManager.hpp"

namespace pictorio::image_actions
{

// Through the org.freedesktop.FileManager1 D-Bus interface, which selects the file; where no file manager
// offers it, the folder opens in the default one instead.
class KioFileManager : public FileManager
{
public:
    QFuture<std::expected<void, QString>> showFile(const QString& filePath) override;
};

}  // namespace pictorio::image_actions
