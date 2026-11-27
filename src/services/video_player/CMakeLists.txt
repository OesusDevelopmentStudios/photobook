# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Headers are listed so that AUTOMOC finds the Q_OBJECT classes in include/.
add_library(photobook_video_player STATIC
    include/VideoPlayer.hpp
    include/VideoPlayerFactory.hpp
    VideoPlayer.cpp
    VideoPlayerFactory.cpp
)

target_include_directories(photobook_video_player PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(photobook_video_player
    PUBLIC
        photobook_connection
        photobook_settings
        Qt6::Core
    PRIVATE
        KF6::I18n
)

# VideoPlayer is exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(photobook_video_player)
