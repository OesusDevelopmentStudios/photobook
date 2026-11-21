/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "NavigatorFactory.hpp"

namespace photobook::navigator
{

std::unique_ptr<Navigator> createNavigator(std::unique_ptr<folder_scanner::FolderScanner> folderScanner)
{
    return std::make_unique<Navigator>(std::move(folderScanner));
}

}  // namespace photobook::navigator
