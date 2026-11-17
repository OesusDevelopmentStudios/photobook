# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

# Shared helpers. A static library, so that tests can link the same code as the application.
add_library(pictorio_utils STATIC
    utils/AppIdentity.cpp
    utils/CommandLine.cpp
    utils/FileHash.cpp
)

# Lets every target that links pictorio_utils write #include "utils/AppIdentity.hpp".
target_include_directories(pictorio_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(pictorio_utils PRIVATE
    PICTORIO_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(pictorio_utils
    PUBLIC
        Qt6::Core
    PRIVATE
        KF6::CoreAddons
        KF6::I18n
)

add_subdirectory(services/image_loader)
add_subdirectory(services/viewer)
add_subdirectory(services/view_transform)
add_subdirectory(services/folder_scanner)
add_subdirectory(services/navigator)
add_subdirectory(services/slideshow)
add_subdirectory(services/settings)
add_subdirectory(services/server_client)
add_subdirectory(services/token_store)
add_subdirectory(services/connection)
add_subdirectory(services/library)
add_subdirectory(services/library_view)
add_subdirectory(services/tag_editor)
add_subdirectory(services/uploader)
add_subdirectory(services/network_access)

# The user interface: the QML files and the C++ types they use, the AppManager and through it the services. A
# static library that is also a QML module, so the application and the QML tests use the same one. The URI is
# the app ID; main.cpp loads Main.qml from it with loadFromModule().
qt_add_library(pictorio_ui STATIC)

# QML modules go into folders named after their URI under one root, where qmllint and the QML tests find them.
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)

set(qmlFiles
    qml/Main.qml
    qml/ViewerPage.qml
    qml/SlideshowControls.qml
    qml/WelcomePage.qml
    qml/ConnectPage.qml
    qml/AccountPage.qml
    qml/LibraryPage.qml
    qml/LibrarySidebar.qml
    qml/LibraryTile.qml
    qml/ImageDetailsFooter.qml
    qml/ImageDetailsZone.qml
    qml/TagChip.qml
    qml/TagNameField.qml
    qml/TagValueBox.qml
    qml/TagRowContent.qml
    qml/BulkTagPanel.qml
    qml/UploadPage.qml
    qml/UploadIndicator.qml
)

# At the module's root, though they are in qml/ here: only files at the root see the module's C++ types.
foreach(qmlFile ${qmlFiles})
    get_filename_component(fileName ${qmlFile} NAME)
    set_source_files_properties(${qmlFile} PROPERTIES QT_RESOURCE_ALIAS ${fileName})
endforeach()

qt_add_qml_module(pictorio_ui
    URI io.github.oesusdevelopmentstudios.pictorio
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml  # where the QML engine looks for modules in the resources
    SOURCES
        app_manager/AppManager.hpp
        app_manager/AppManager.cpp
        app_manager/QmlForeignTypes.hpp
    QML_FILES ${qmlFiles}
)

# Qt gives the module's generated targets (resources, plugin) the folders of generated headers, and they can be
# built before those exist; with warnings as errors, a missing include folder would stop the build.
foreach(generatingTarget pictorio_ui pictorio_uiplugin)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${generatingTarget}_autogen/include)
endforeach()

# Qt's generated QML type registration includes the headers by file name only.
target_include_directories(pictorio_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/app_manager
)

target_link_libraries(pictorio_ui PUBLIC
    pictorio_utils
    pictorio_viewer
    pictorio_view_transform
    pictorio_navigator
    pictorio_slideshow
    pictorio_connection
    pictorio_network_access
    pictorio_library
    pictorio_library_view
    pictorio_tag_editor
    pictorio_uploader
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

# The application. main.cpp only wires things together; logic lives in the libraries above.
add_executable(pictorio main.cpp)

target_link_libraries(pictorio PRIVATE
    pictorio_ui
    pictorio_uiplugin  # the QML module's types, imported in main.cpp
    Qt6::Widgets
    KF6::CoreAddons
    KF6::I18n
    KF6::I18nQml
)

install(TARGETS pictorio ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
