/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "ImageActions.hpp"

namespace pictorio::image_actions
{

// The connection is not owned and must outlive the actions.
std::unique_ptr<ImageActions> createImageActions(connection::Connection* connection);

}  // namespace pictorio::image_actions
