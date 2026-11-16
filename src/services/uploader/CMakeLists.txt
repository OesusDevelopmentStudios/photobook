# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Sending files to the server's library (see include/Uploader.hpp).
add_library(pictorio_uploader STATIC
    include/Uploader.hpp
    include/UploaderFactory.hpp
    Uploader.cpp
    UploaderFactory.cpp
)

target_include_directories(pictorio_uploader PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pictorio_uploader
    PUBLIC
        pictorio_connection
        pictorio_tag_editor
        Qt6::Core
    PRIVATE
        pictorio_folder_scanner
        pictorio_utils
        Qt6::Concurrent  # hashes files and lists folders in the background
        KF6::I18n
)

# Uploader is exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(pictorio_uploader)
