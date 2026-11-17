/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include "Trash.hpp"

namespace pictorio::uploader
{

// The desktop's trash through the Trash portal, for the Flatpak: inside the sandbox KIO would use a trash in the
// app's own data folder, which Dolphin never shows.
class PortalTrash : public Trash
{
public:
    QFuture<std::expected<void, QString>> moveToTrash(const QString& filePath) override;
};

}  // namespace pictorio::uploader
