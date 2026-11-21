# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Headers are listed so that AUTOMOC finds the Q_OBJECT classes in include/.
add_library(photobook_viewer STATIC
    include/Viewer.hpp
    include/ViewerFactory.hpp
    Viewer.cpp
    ViewerFactory.cpp
)

target_include_directories(photobook_viewer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(photobook_viewer PUBLIC
    photobook_image_loader
    Qt6::Core
)

# Viewer is exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(photobook_viewer)
