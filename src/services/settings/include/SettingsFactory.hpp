/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QString>

#include "Settings.hpp"

namespace photobook::settings
{

// In the application's config file, ~/.config/photobookrc
std::unique_ptr<Settings> createSettings();

std::unique_ptr<Settings> createSettings(const QString& configFilePath);

}  // namespace photobook::settings
