/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "ImageLoader.hpp"

namespace pictorio::image_loader
{

std::unique_ptr<ImageLoader> createImageLoader();

}  // namespace pictorio::image_loader
