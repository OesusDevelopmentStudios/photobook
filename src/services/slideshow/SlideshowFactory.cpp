/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "SlideshowFactory.hpp"

namespace photobook::slideshow
{

std::unique_ptr<Slideshow> createSlideshow(navigator::Navigator* navigator, settings::Settings* settings)
{
    return std::make_unique<Slideshow>(navigator, settings);
}

}  // namespace photobook::slideshow
