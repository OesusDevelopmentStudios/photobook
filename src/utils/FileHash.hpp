/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QString>

namespace pictorio::utils
{

// The SHA-256 of a file's content as lowercase hex, as the server writes it, e.g. to tell that the server has
// the same file. Read in chunks, so a large file does not fill the memory. Nothing if the file cannot be read.
std::optional<QString> sha256Of(const QString& filePath);

}  // namespace pictorio::utils
