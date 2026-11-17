/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "UploaderFactory.hpp"

#include "impl/KioTrash.hpp"

namespace pictorio::uploader
{

std::unique_ptr<Uploader> createUploader(connection::Connection* connection, tag_editor::UploadTagEditor* tagEditor)
{
    return std::make_unique<Uploader>(connection, tagEditor, std::make_unique<KioTrash>());
}

}  // namespace pictorio::uploader
