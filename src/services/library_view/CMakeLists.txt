# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# How the library page is laid out, kept between runs (see include/LibraryView.hpp).
add_library(photobook_library_view STATIC
    include/LibraryView.hpp
    include/LibraryViewFactory.hpp
    LibraryView.cpp
    LibraryViewFactory.cpp
)

target_include_directories(photobook_library_view PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(photobook_library_view
    PUBLIC
        photobook_settings
        Qt6::Core
    PRIVATE
        photobook_server_client  # the thumbnail sizes
)

# LibraryView is exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(photobook_library_view)
