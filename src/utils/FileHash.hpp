/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QString>

namespace photobook::utils
{

// Lowercase hex, as the server writes it. Read in chunks, so a large file does not fill the memory. Nothing if
// the file cannot be read.
std::optional<QString> sha256Of(const QString& filePath);

}  // namespace photobook::utils
