# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

include(ECMAddTests)

# Shared fakes for the tests, e.g. #include "fakes/FakeServer.hpp"
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# One Qt Test executable per source file. The CTest name is the file name without .cpp.

ecm_add_test(utils/TestAppIdentity.cpp
    LINK_LIBRARIES photobook_utils Qt6::Test KF6::CoreAddons KF6::I18n
)
target_compile_definitions(TestAppIdentity PRIVATE
    EXPECTED_VERSION="${PROJECT_VERSION}"
)

ecm_add_test(utils/TestCommandLine.cpp
    LINK_LIBRARIES photobook_utils Qt6::Test
)

ecm_add_test(utils/TestFileHash.cpp
    LINK_LIBRARIES photobook_utils Qt6::Test
)

ecm_add_test(utils/TestMachineId.cpp
    LINK_LIBRARIES photobook_utils Qt6::Test
)

ecm_add_test(services/image_loader/TestImageLoader.cpp
    LINK_LIBRARIES photobook_image_loader Qt6::Test KF6::I18n LibExiv2::LibExiv2
)
kde_target_enable_exceptions(TestImageLoader PRIVATE)  # exiv2 writes the test EXIF data

ecm_add_test(services/viewer/TestViewer.cpp
    LINK_LIBRARIES photobook_viewer Qt6::Test KF6::I18n
)

ecm_add_test(services/view_transform/TestViewTransform.cpp
    LINK_LIBRARIES photobook_view_transform Qt6::Test
)

ecm_add_test(services/folder_scanner/TestFolderScanner.cpp
    LINK_LIBRARIES photobook_folder_scanner Qt6::Test
)

ecm_add_test(services/navigator/TestNavigator.cpp
    LINK_LIBRARIES photobook_navigator Qt6::Test
)

ecm_add_test(services/navigator/TestLibraryNavigation.cpp
    LINK_LIBRARIES photobook_navigator photobook_library Qt6::Test
)

ecm_add_test(services/slideshow/TestSlideshow.cpp
    LINK_LIBRARIES photobook_slideshow Qt6::Test
)

ecm_add_test(services/settings/TestSettings.cpp
    LINK_LIBRARIES photobook_settings Qt6::Test KF6::ConfigCore
)

ecm_add_test(services/server_client/TestServerClient.cpp
    LINK_LIBRARIES photobook_server_client Qt6::Network Qt6::Test
)

ecm_add_test(services/token_store/TestTokenStore.cpp
    LINK_LIBRARIES photobook_token_store Qt6Keychain::Qt6Keychain Qt6::Test
)
# Tests the error mapping in hlp/; the keyring itself is not used in tests.
target_include_directories(TestTokenStore PRIVATE ${PROJECT_SOURCE_DIR}/src/services/token_store)

ecm_add_test(services/connection/TestConnection.cpp
    LINK_LIBRARIES photobook_connection Qt6::Test
)

ecm_add_test(services/library/TestLibrary.cpp
    LINK_LIBRARIES photobook_library Qt6::Test
)

ecm_add_test(services/library/TestAlbums.cpp
    LINK_LIBRARIES photobook_library Qt6::Test KF6::I18n
)

ecm_add_test(services/library_view/TestLibraryView.cpp
    LINK_LIBRARIES photobook_library_view Qt6::Test
)

ecm_add_test(services/tag_editor/TestTagRules.cpp
    LINK_LIBRARIES photobook_tag_editor Qt6::Test KF6::I18n
)
# Tests the rules in hlp/, before anything is sent to the server.
target_include_directories(TestTagRules PRIVATE ${PROJECT_SOURCE_DIR}/src/services/tag_editor)

ecm_add_test(services/tag_editor/TestTagSuggestions.cpp
    LINK_LIBRARIES photobook_tag_editor Qt6::Test
)

ecm_add_test(services/tag_editor/TestTagEditor.cpp
    LINK_LIBRARIES photobook_tag_editor Qt6::Test KF6::I18n
)

ecm_add_test(services/tag_editor/TestBulkTagEditor.cpp
    LINK_LIBRARIES photobook_tag_editor Qt6::Test KF6::I18n
)

ecm_add_test(services/tag_editor/TestUploadTagEditor.cpp
    LINK_LIBRARIES photobook_tag_editor Qt6::Test KF6::I18n
)

ecm_add_test(services/tag_editor/TestHiddenFilterEditor.cpp
    LINK_LIBRARIES photobook_tag_editor Qt6::Test KF6::I18n
)

ecm_add_test(services/uploader/TestUploader.cpp
    LINK_LIBRARIES photobook_uploader Qt6::Test KF6::I18n
)

ecm_add_test(services/image_actions/TestImageActions.cpp
    LINK_LIBRARIES photobook_image_actions Qt6::Gui Qt6::Test KF6::I18n LibExiv2::LibExiv2
)
kde_target_enable_exceptions(TestImageActions PRIVATE)  # exiv2, to give the test files an orientation
# The clipboard needs a GUI platform; offscreen keeps it in the process.
set_tests_properties(TestImageActions PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

ecm_add_test(services/network_access/TestNetworkAccess.cpp
    LINK_LIBRARIES photobook_network_access Qt6::Test
)

ecm_add_test(packaging/TestDesktopFile.cpp
    LINK_LIBRARIES photobook_image_loader Qt6::Gui Qt6::Test KF6::I18n
)
target_compile_definitions(TestDesktopFile PRIVATE
    DESKTOP_FILE="${PROJECT_SOURCE_DIR}/packaging/io.github.oesusdevelopmentstudios.photobook.desktop"
)

# Against a real photobook-db server, which e2e/run_end_to_end_tests.py starts; they skip themselves without it.
ecm_add_test(e2e/TestEndToEndLogin.cpp
    LINK_LIBRARIES photobook_connection Qt6::Test KF6::I18n
)

ecm_add_test(e2e/TestEndToEndLibrary.cpp
    LINK_LIBRARIES photobook_uploader photobook_library photobook_tag_editor Qt6::Gui Qt6::Test KF6::I18n
)

ecm_add_test(e2e/TestEndToEndDuplicates.cpp
    LINK_LIBRARIES photobook_uploader photobook_library photobook_tag_editor Qt6::Gui Qt6::Test KF6::I18n
)

ecm_add_test(e2e/TestEndToEndHiddenFilters.cpp
    LINK_LIBRARIES photobook_uploader photobook_library photobook_tag_editor Qt6::Gui Qt6::Test KF6::I18n
)

ecm_add_test(e2e/TestEndToEndRestrictions.cpp
    LINK_LIBRARIES photobook_uploader photobook_library photobook_tag_editor photobook_utils Qt6::Gui Qt6::Test KF6::I18n
)

ecm_add_test(e2e/TestEndToEndLocalLibrary.cpp
    LINK_LIBRARIES photobook_uploader photobook_library photobook_tag_editor photobook_utils Qt6::Gui Qt6::Test KF6::I18n
)

ecm_add_test(e2e/TestEndToEndRotation.cpp
    LINK_LIBRARIES photobook_uploader photobook_library photobook_tag_editor photobook_image_actions
        photobook_network_access Qt6::Gui Qt6::Network Qt6::Test KF6::I18n
)

# The QML tests, tst_*.qml in qml/: one Qt Quick Test executable that uses the application's QML module,
# offscreen. Qt's messages go to stderr, not to the system journal, so failures show in the test output.
add_executable(TestQml qml/TestQml.cpp qml/FakeTagRowsEditor.hpp qml/TestServices.hpp)
target_link_libraries(TestQml PRIVATE
    photobook_ui
    photobook_uiplugin
    Qt6::QuickTest
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::I18n
    KF6::I18nQml
)
target_compile_definitions(TestQml PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml")
add_test(NAME TestQml COMMAND TestQml)
set_tests_properties(TestQml PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_FORCE_STDERR_LOGGING=1")
