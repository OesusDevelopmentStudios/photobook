/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "ImageActionsFactory.hpp"

namespace pictorio::image_actions
{

std::unique_ptr<ImageActions> createImageActions(connection::Connection* connection)
{
    return std::make_unique<ImageActions>(connection);
}

}  // namespace pictorio::image_actions
