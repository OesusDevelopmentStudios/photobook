/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "UploaderFactory.hpp"

#include <KSandbox>

#include "impl/KioTrash.hpp"
#include "impl/PortalTrash.hpp"

namespace pictorio::uploader
{

std::unique_ptr<Uploader> createUploader(connection::Connection* connection, tag_editor::UploadTagEditor* tagEditor)
{
    std::unique_ptr<Trash> trash;
    if (KSandbox::isFlatpak())
    {
        trash = std::make_unique<PortalTrash>();
    }
    else
    {
        trash = std::make_unique<KioTrash>();
    }

    return std::make_unique<Uploader>(connection, tagEditor, std::move(trash));
}

}  // namespace pictorio::uploader
