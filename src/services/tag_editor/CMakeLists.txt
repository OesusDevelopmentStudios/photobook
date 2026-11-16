# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Changing the tags of images on the server (see include/). hlp/ holds the rules a change is checked
# against before it is sent.
add_library(pictorio_tag_editor STATIC
    include/TagEditor.hpp
    include/TagSuggestions.hpp
    include/TagEditorFactory.hpp
    hlp/TagNames.hpp
    hlp/TagNames.cpp
    hlp/SearchTerm.hpp
    hlp/SearchTerm.cpp
    hlp/Restrictions.hpp
    hlp/Restrictions.cpp
    hlp/TagMessages.hpp
    hlp/TagMessages.cpp
    impl/Namespaces.hpp
    impl/Namespaces.cpp
    impl/NamespaceRows.hpp
    impl/NamespaceRows.cpp
    TagEditor.cpp
    TagSuggestions.cpp
    TagEditorFactory.cpp
)

# Users of the service see only include/; hlp/ stays private to the service.
target_include_directories(pictorio_tag_editor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pictorio_tag_editor
    PUBLIC
        pictorio_connection
        Qt6::Core
    PRIVATE
        KF6::I18n
)

# TagEditor and TagSuggestions are exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read their
# properties from these metatypes.
qt_extract_metatypes(pictorio_tag_editor)
