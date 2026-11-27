/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "VideoPlayer.hpp"

namespace photobook::video_player
{

// The connection and the settings are not owned and must outlive the player.
std::unique_ptr<VideoPlayer> createVideoPlayer(connection::Connection* connection, settings::Settings* settings);

}  // namespace photobook::video_player
