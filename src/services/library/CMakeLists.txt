# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# The images of a search on the server, for the library grid, and the albums, for the Albums tab (see include/).
add_library(photobook_library STATIC
    include/Albums.hpp
    include/Library.hpp
    include/LibraryFactory.hpp
    hlp/ServerErrors.hpp
    hlp/ServerErrors.cpp
    Albums.cpp
    Library.cpp
    LibraryFactory.cpp
)

target_include_directories(photobook_library
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(photobook_library
    PUBLIC
        Qt6::Core
        photobook_connection
        photobook_settings
    PRIVATE
        KF6::I18n
)

# Library and Albums are exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(photobook_library)
