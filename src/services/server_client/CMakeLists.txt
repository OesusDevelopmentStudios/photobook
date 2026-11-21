# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# The photobook-db REST API (see include/ServerClient.hpp).
add_library(photobook_server_client STATIC
    include/ApiTypes.hpp
    include/HttpTransport.hpp
    include/ServerClient.hpp
    include/ServerClientFactory.hpp
    impl/QtHttpTransport.hpp
    impl/QtHttpTransport.cpp
    impl/RestServerClient.hpp
    impl/RestServerClient.cpp
    hlp/ApiJson.hpp
    hlp/ApiJson.cpp
    hlp/JsonReader.hpp
    hlp/JsonReader.cpp
    hlp/ProblemDetails.hpp
    hlp/ProblemDetails.cpp
    HttpTransport.cpp
    ServerClient.cpp
    ServerClientFactory.cpp
)

# Users of the service see only include/; impl/ and hlp/ stay private to the service.
target_include_directories(photobook_server_client
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(photobook_server_client
    PUBLIC
        Qt6::Core
    PRIVATE
        Qt6::Network
)
