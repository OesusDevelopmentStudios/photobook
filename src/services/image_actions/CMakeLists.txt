# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# What can be done with an image besides looking at it, e.g. copying it (see include/ImageActions.hpp).
add_library(pictorio_image_actions STATIC
    include/ImageActions.hpp
    include/ImageActionsFactory.hpp
    include/FileManager.hpp
    impl/KioFileManager.hpp
    impl/KioFileManager.cpp
    hlp/FileRotation.hpp
    hlp/FileRotation.cpp
    ImageActions.cpp
    ImageActionsFactory.cpp
)

target_include_directories(pictorio_image_actions
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pictorio_image_actions
    PUBLIC
        pictorio_connection
        Qt6::Core
    PRIVATE
        Qt6::Gui  # the clipboard
        KF6::I18n
        KF6::KIOGui  # shows a file in the file manager
        LibExiv2::LibExiv2  # writes the EXIF orientation when a rotation is saved
)

# exiv2 reports errors with exceptions, which KDE's compiler settings turn off by default.
kde_target_enable_exceptions(pictorio_image_actions PRIVATE)

# ImageActions is exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(pictorio_image_actions)
