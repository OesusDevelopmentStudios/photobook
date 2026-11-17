# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# The connection to a pictorio-db server and the login (see include/Connection.hpp).
add_library(pictorio_connection STATIC
    include/Connection.hpp
    include/ConnectionFactory.hpp
    hlp/ServerAddress.hpp
    hlp/ServerAddress.cpp
    Connection.cpp
    ConnectionFactory.cpp
)

# Users of the service see only include/; hlp/ stays private to the service.
target_include_directories(pictorio_connection
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pictorio_connection
    PUBLIC
        Qt6::Core
        pictorio_server_client
        pictorio_settings
        pictorio_token_store
    PRIVATE
        KF6::I18n
        pictorio_utils  # this computer's machine ID
)

# Connection is exposed to QML (app_manager/QmlForeignTypes.hpp); QML tools read its properties from these metatypes.
qt_extract_metatypes(pictorio_connection)
