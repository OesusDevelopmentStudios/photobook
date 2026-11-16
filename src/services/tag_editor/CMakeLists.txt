# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Changing the tags of images on the server. So far only the rules a change is checked against before it is
# sent; hlp/ stays private to the service.
add_library(pictorio_tag_editor STATIC
    hlp/TagNames.hpp
    hlp/TagNames.cpp
    hlp/SearchTerm.hpp
    hlp/SearchTerm.cpp
)

target_include_directories(pictorio_tag_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pictorio_tag_editor
    PUBLIC
        Qt6::Core
    PRIVATE
        KF6::I18n
)
