# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

include(ECMAddTests)

# Shared fakes for the tests, e.g. #include "fakes/FakeServer.hpp"
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# One Qt Test executable per source file. The CTest name is the file name without .cpp.

ecm_add_test(utils/TestAppIdentity.cpp
    LINK_LIBRARIES pictorio_utils Qt6::Test KF6::CoreAddons KF6::I18n
)
target_compile_definitions(TestAppIdentity PRIVATE
    EXPECTED_VERSION="${PROJECT_VERSION}"
)

ecm_add_test(utils/TestCommandLine.cpp
    LINK_LIBRARIES pictorio_utils Qt6::Test
)

ecm_add_test(services/image_loader/TestImageLoader.cpp
    LINK_LIBRARIES pictorio_image_loader Qt6::Test KF6::I18n LibExiv2::LibExiv2
)
kde_target_enable_exceptions(TestImageLoader PRIVATE)  # exiv2 writes the test EXIF data

ecm_add_test(services/viewer/TestViewer.cpp
    LINK_LIBRARIES pictorio_viewer Qt6::Test KF6::I18n
)

ecm_add_test(services/view_transform/TestViewTransform.cpp
    LINK_LIBRARIES pictorio_view_transform Qt6::Test
)

ecm_add_test(services/folder_scanner/TestFolderScanner.cpp
    LINK_LIBRARIES pictorio_folder_scanner Qt6::Test
)

ecm_add_test(services/navigator/TestNavigator.cpp
    LINK_LIBRARIES pictorio_navigator Qt6::Test
)

ecm_add_test(services/navigator/TestLibraryNavigation.cpp
    LINK_LIBRARIES pictorio_navigator pictorio_library Qt6::Test
)

ecm_add_test(services/slideshow/TestSlideshow.cpp
    LINK_LIBRARIES pictorio_slideshow Qt6::Test
)

ecm_add_test(services/settings/TestSettings.cpp
    LINK_LIBRARIES pictorio_settings Qt6::Test KF6::ConfigCore
)

ecm_add_test(services/server_client/TestServerClient.cpp
    LINK_LIBRARIES pictorio_server_client Qt6::Network Qt6::Test
)

ecm_add_test(services/token_store/TestTokenStore.cpp
    LINK_LIBRARIES pictorio_token_store Qt6Keychain::Qt6Keychain Qt6::Test
)
# Tests the error mapping in hlp/; the keyring itself is not used in tests.
target_include_directories(TestTokenStore PRIVATE ${PROJECT_SOURCE_DIR}/src/services/token_store)

ecm_add_test(services/connection/TestConnection.cpp
    LINK_LIBRARIES pictorio_connection Qt6::Test
)

ecm_add_test(services/library/TestLibrary.cpp
    LINK_LIBRARIES pictorio_library Qt6::Test
)

ecm_add_test(services/library_view/TestLibraryView.cpp
    LINK_LIBRARIES pictorio_library_view Qt6::Test
)

ecm_add_test(services/tag_editor/TestTagRules.cpp
    LINK_LIBRARIES pictorio_tag_editor Qt6::Test KF6::I18n
)
# Tests the rules in hlp/, before anything is sent to the server.
target_include_directories(TestTagRules PRIVATE ${PROJECT_SOURCE_DIR}/src/services/tag_editor)

ecm_add_test(services/tag_editor/TestTagSuggestions.cpp
    LINK_LIBRARIES pictorio_tag_editor Qt6::Test
)

ecm_add_test(services/network_access/TestNetworkAccess.cpp
    LINK_LIBRARIES pictorio_network_access Qt6::Test
)

ecm_add_test(packaging/TestDesktopFile.cpp
    LINK_LIBRARIES pictorio_image_loader Qt6::Gui Qt6::Test KF6::I18n
)
target_compile_definitions(TestDesktopFile PRIVATE
    DESKTOP_FILE="${PROJECT_SOURCE_DIR}/packaging/io.github.oesusdevelopmentstudios.pictorio.desktop"
)
