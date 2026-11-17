# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Sending files to the server's library (see include/Uploader.hpp).
add_library(pictorio_uploader STATIC
    include/Uploader.hpp
    include/UploaderFactory.hpp
    include/Trash.hpp
    impl/KioTrash.hpp
    impl/KioTrash.cpp
    impl/PortalTrash.hpp
    impl/PortalTrash.cpp
    Trash.cpp
    Uploader.cpp
    UploaderFactory.cpp
)

# Users of the service see only include/; impl/ stays private to the service.
target_include_directories(pictorio_uploader
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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
        KF6::KIOCore      # moves originals to the trash…
        Qt6::DBus         # …or the Trash portal does, in a Flatpak
        KF6::CoreAddons   # KSandbox: are we in a Flatpak?
)

# Uploader is exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(pictorio_uploader)
