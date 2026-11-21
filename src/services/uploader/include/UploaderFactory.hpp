/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "Uploader.hpp"

namespace pictorio::uploader
{

// Originals go to the desktop's trash: through KIO, or the Trash portal in a Flatpak. connection and tagEditor
// are not owned; they must outlive the uploader.
std::unique_ptr<Uploader> createUploader(connection::Connection* connection, tag_editor::UploadTagEditor* tagEditor);

}  // namespace pictorio::uploader
