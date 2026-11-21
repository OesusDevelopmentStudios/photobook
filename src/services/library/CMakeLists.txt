# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# The images of a search on the server, for the library grid (see include/Library.hpp).
add_library(photobook_library STATIC
    include/Library.hpp
    include/LibraryFactory.hpp
    Library.cpp
    LibraryFactory.cpp
)

target_include_directories(photobook_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(photobook_library
    PUBLIC
        Qt6::Core
        photobook_connection
    PRIVATE
        KF6::I18n
)

# Library is exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(photobook_library)
