# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Network access for images QML loads from the server: the login token and a disk cache
# (see include/NetworkAccess.hpp).
add_library(photobook_network_access STATIC
    include/NetworkAccess.hpp
    impl/AuthorizingNetworkAccessManager.hpp
    impl/AuthorizingNetworkAccessManager.cpp
    NetworkAccess.cpp
)

# Users of the service see only include/; impl/ stays private to the service.
target_include_directories(photobook_network_access
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(photobook_network_access PUBLIC
    Qt6::Network
    Qt6::Qml
)
