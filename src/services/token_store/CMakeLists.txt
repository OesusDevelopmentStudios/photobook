# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Login tokens in the system keyring (see include/TokenStore.hpp).
add_library(pictorio_token_store STATIC
    include/TokenStore.hpp
    include/TokenStoreFactory.hpp
    impl/KeychainTokenStore.hpp
    impl/KeychainTokenStore.cpp
    hlp/KeychainErrors.hpp
    hlp/KeychainErrors.cpp
    TokenStore.cpp
    TokenStoreFactory.cpp
)

# Users of the service see only include/; impl/ and hlp/ stay private to the service.
target_include_directories(pictorio_token_store
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(pictorio_token_store
    PUBLIC
        Qt6::Core
    PRIVATE
        Qt6Keychain::Qt6Keychain
)
