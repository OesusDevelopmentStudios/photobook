/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "VideoPlayerFactory.hpp"

namespace photobook::video_player
{

std::unique_ptr<VideoPlayer> createVideoPlayer(connection::Connection* connection, settings::Settings* settings)
{
    return std::make_unique<VideoPlayer>(connection, settings);
}

}  // namespace photobook::video_player
