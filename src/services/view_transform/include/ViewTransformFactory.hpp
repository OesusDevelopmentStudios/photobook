/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "ViewTransform.hpp"

namespace photobook::view_transform
{

std::unique_ptr<ViewTransform> createViewTransform();

}  // namespace photobook::view_transform
