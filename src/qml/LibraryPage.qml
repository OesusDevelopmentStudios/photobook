// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the delegate below use the page's ids.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.ki18n

// The images on the server: a sidebar with the search, and a grid of square thumbnails, newest first. More
// images load as the grid nears its end.
Kirigami.Page {
    id: page

    required property Library library
    required property LibraryView libraryView
    required property BulkTagEditor bulkTagEditor
    required property Navigator navigator
    required property Connection connection
    required property ImageActions imageActions

    signal openImageRequested()
    signal openFolderRequested()
    signal serverPageRequested()
    signal uploadRequested()
    signal imageOpenRequested(int row)
    signal imageCopyRequested(int row)
    signal imageFolderRequested(int row)  // the image's, in the file manager
    signal stopSelectingRequested()       // Done or Esc; the window asks first about tag changes not applied

    readonly property int connectionState: connection.state
    readonly property bool loggedIn: connectionState === Connection.State.LoggedIn

    title: KI18n.i18nc("@title", "Library")
    padding: 0

    // From the background colour to transparent, over the edge of the grid. Takes no input, so tiles under
    // it are still hovered.
    component EdgeFade: Rectangle {
        property bool shown: false

        height: Kirigami.Units.gridUnit * 2
        opacity: shown ? 1 : 0
        gradient: Gradient {
            GradientStop {
                position: 0
                color: Kirigami.Theme.backgroundColor
            }
            GradientStop {
                position: 1
                color: Qt.alpha(Kirigami.Theme.backgroundColor, 0)
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: Kirigami.Units.shortDuration
            }
        }
    }

    // Back from the viewer, the last image viewed there is in view, also if the viewer stepped far on.
    onIsCurrentPageChanged: {
        if (isCurrentPage && navigator.browsingLibrary && navigator.currentIndex >= 0) {
            grid.positionViewAtIndex(navigator.currentIndex, GridView.Contain);
        }
    }

    // A new tile size reflows the grid; the image at the top stays in view instead of jumping away.
    function resizeTiles(resize) {
        const topIndex = grid.indexAt(grid.cellWidth / 2, grid.contentY + grid.cellHeight / 2);
        resize();
        grid.forceLayout();
        if (topIndex > 0) {
            grid.positionViewAtIndex(topIndex, GridView.Beginning);
        }
    }

    // The sidebar's toggle sits at the left, next to the sidebar it shows.
    titleDelegate: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        Controls.ToolButton {
            action: sidebarAction
            display: Controls.AbstractButton.IconOnly
            visible: page.loggedIn

            Controls.ToolTip.text: text
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            text: page.title
            elide: Text.ElideRight
            maximumLineCount: 1
        }
    }

    // Only while the library is shown: the viewer may be on top of it.
    Kirigami.Action {
        id: sidebarAction

        icon.name: "sidebar-show-symbolic"
        text: KI18n.i18nc("@action:intoolbar", "Show Sidebar")
        shortcut: "F9"  // as Dolphin's panels
        checkable: true
        checked: page.libraryView.sidebarVisible
        enabled: page.loggedIn && page.isCurrentPage
        onToggled: page.libraryView.sidebarVisible = checked
    }

    Kirigami.Action {
        shortcut: "Escape"
        enabled: page.loggedIn && page.isCurrentPage && page.library.selecting
        onTriggered: page.stopSelectingRequested()
    }

    Kirigami.Action {
        shortcut: StandardKey.Find
        enabled: page.loggedIn && page.isCurrentPage
        onTriggered: {
            page.libraryView.sidebarVisible = true;
            sidebar.focusSearch();
        }
    }

    // A tile's right-click menu, one for the whole grid. It acts on that tile alone, whatever is selected.
    Components.ConvergentContextMenu {
        id: tileMenu

        property int row: -1

        // While selecting, a click only selects; an image opens once done.
        Kirigami.Action {
            icon.name: "document-open"
            text: KI18n.i18nc("@action:inmenu", "Open")
            visible: !page.library.selecting
            onTriggered: page.imageOpenRequested(tileMenu.row)
        }

        Kirigami.Action {
            icon.name: "edit-copy"
            text: KI18n.i18nc("@action:inmenu", "Copy Image")
            enabled: !page.imageActions.busy
            onTriggered: page.imageCopyRequested(tileMenu.row)
        }

        // A library image's file is only reachable when the server runs on this computer.
        Kirigami.Action {
            icon.name: "document-open-folder"
            text: KI18n.i18nc("@action:inmenu", "Open Containing Folder")
            visible: page.connection.localLibraryDir.length > 0
            onTriggered: page.imageFolderRequested(tileMenu.row)
        }
    }

    actions: [
        Kirigami.Action {
            icon.name: "zoom-in"
            text: KI18n.i18nc("@action:inmenu", "Larger Tiles")
            shortcut: StandardKey.ZoomIn
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: page.loggedIn
            enabled: page.isCurrentPage && page.libraryView.tileSize < page.libraryView.maximumTileSize
            onTriggered: page.resizeTiles(() => page.libraryView.resizeTilesBySteps(1))
        },
        Kirigami.Action {
            icon.name: "zoom-out"
            text: KI18n.i18nc("@action:inmenu", "Smaller Tiles")
            shortcut: StandardKey.ZoomOut
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: page.loggedIn
            enabled: page.isCurrentPage && page.libraryView.tileSize > page.libraryView.minimumTileSize
            onTriggered: page.resizeTiles(() => page.libraryView.resizeTilesBySteps(-1))
        },
        Kirigami.Action {
            icon.name: "cloud-upload"
            text: KI18n.i18nc("@action:inmenu", "Upload…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: page.loggedIn
            onTriggered: page.uploadRequested()
        },
        Kirigami.Action {
            icon.name: "document-open"
            text: KI18n.i18nc("@action:inmenu", "Open Image…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.openImageRequested()
        },
        Kirigami.Action {
            icon.name: "folder-open"
            text: KI18n.i18nc("@action:inmenu", "Open Folder…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.openFolderRequested()
        },
        Kirigami.Action {
            icon.name: page.loggedIn ? "configure" : "network-server"
            text: page.loggedIn ? KI18n.i18nc("@action:inmenu", "Settings…")
                                : KI18n.i18nc("@action:inmenu", "Connect to Server…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.serverPageRequested()
        }
    ]

    RowLayout {
        anchors.fill: parent
        spacing: 0
        visible: page.loggedIn

        LibrarySidebar {
            id: sidebar

            Layout.fillHeight: true
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14
            // Also while selecting images: the panel for their tags is there.
            visible: page.libraryView.sidebarVisible || page.library.selecting
            library: page.library
            libraryView: page.libraryView
            bulkTagEditor: page.bulkTagEditor
            onTileSizeRequested: gridUnits => page.resizeTiles(() => page.libraryView.tileSize = gridUnits)
            onStopSelectingRequested: page.stopSelectingRequested()
        }

        Kirigami.Separator {
            Layout.fillHeight: true
            visible: sidebar.visible
        }

        // The grid in the colours of a view, like the file list in Dolphin
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
            color: Kirigami.Theme.backgroundColor

            GridView {
                id: grid

                // As many tiles of the chosen size as fit; they stretch a little to fill the row.
                readonly property real tileWidth: page.libraryView.tileSize * Kirigami.Units.gridUnit
                readonly property int columns: Math.max(1, Math.floor(width / tileWidth))
                readonly property real thumbnailSize: tileWidth - Kirigami.Units.smallSpacing * 2  // the tile's padding
                readonly property bool largeThumbnails:
                    page.libraryView.needsLargeThumbnail(thumbnailSize * Screen.devicePixelRatio)

                // Room at the edges as between the tiles; at the top and bottom it scrolls with them.
                anchors.fill: parent
                anchors.leftMargin: Kirigami.Units.smallSpacing
                anchors.rightMargin: Kirigami.Units.smallSpacing
                topMargin: Kirigami.Units.smallSpacing
                bottomMargin: Kirigami.Units.smallSpacing
                model: page.loggedIn ? page.library : null
                clip: true
                cellWidth: width / columns
                cellHeight: cellWidth
                cacheBuffer: cellHeight * 2  // rows above and below the window are made in advance
                reuseItems: true  // tiles scrolled away are reused, not made anew

                Controls.ScrollBar.vertical: Controls.ScrollBar {
                    id: scrollBar
                }

                // The next page loads while two screens of images are still ahead.
                function loadMoreIfNearTheEnd() {
                    if (contentY + height * 3 > contentHeight) {
                        page.library.loadMore();
                    }
                }

                onContentYChanged: loadMoreIfNearTheEnd()
                onCountChanged: loadMoreIfNearTheEnd()  // a short page may not fill the window
                onHeightChanged: loadMoreIfNearTheEnd()

                // Ctrl + wheel resizes the tiles, as zooming in Dolphin. A mouse wheel may arrive from a device
                // reported as a touchpad.
                WheelHandler {
                    target: null
                    acceptedModifiers: Qt.ControlModifier
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onWheel: event => page.resizeTiles(
                        () => page.libraryView.resizeTilesBySteps(event.angleDelta.y / 120))  // 120: one notch
                }

                delegate: LibraryTile {
                    width: grid.cellWidth
                    height: grid.cellHeight
                    thumbnailSize: grid.thumbnailSize
                    useLargeThumbnail: grid.largeThumbnails
                    selecting: page.library.selecting
                    onOpenRequested: row => page.imageOpenRequested(row)
                    onSelectionToggled: row => page.library.toggleSelected(row)
                    onRangeSelected: (row, addToSelection) => page.library.selectRange(row, addToSelection)
                    onMenuRequested: row => {
                        tileMenu.row = row;
                        tileMenu.popup();
                    }
                }

                footer: Item {
                    width: grid.width
                    height: page.library.loading ? Kirigami.Units.gridUnit * 3 : 0

                    Controls.BusyIndicator {
                        anchors.centerIn: parent
                        running: page.library.loading
                        visible: running
                    }
                }
            }

            // Tiles scroll away into the background at the edges, and come out of it. Each fade shows only
            // while there is more to scroll to on its side, so the first and last rows are never covered.
            EdgeFade {
                anchors.top: grid.top
                anchors.left: grid.left
                anchors.right: grid.right
                anchors.rightMargin: scrollBar.width  // the scroll bar stays sharp
                shown: !grid.atYBeginning
            }

            EdgeFade {
                anchors.bottom: grid.bottom
                anchors.left: grid.left
                anchors.right: grid.right
                anchors.rightMargin: scrollBar.width
                shown: !grid.atYEnd
                rotation: 180  // opaque at the bottom
            }

            Kirigami.InlineMessage {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Kirigami.Units.smallSpacing
                visible: page.library.errorMessage.length > 0
                type: Kirigami.MessageType.Error
                text: page.library.errorMessage
                actions: [
                    Kirigami.Action {
                        icon.name: "view-refresh"
                        text: KI18n.i18nc("@action:button", "Try Again")
                        onTriggered: page.library.reload()
                    }
                ]
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: page.library.count === 0 && !page.library.loading
                         && page.library.errorMessage.length === 0
                icon.name: "image-x-generic"
                text: page.library.searchText.length > 0
                      ? KI18n.i18nc("@info:placeholder", "Nothing matches")
                      : KI18n.i18nc("@info:placeholder", "No images in the library yet")
                explanation: page.library.searchText.length > 0 ? page.library.effectiveQuery : ""
            }
        }
    }

    Kirigami.LoadingPlaceholder {
        anchors.centerIn: parent
        visible: page.connectionState === Connection.State.RestoringLogin
        text: KI18n.i18nc("@info:placeholder", "Connecting to %1…", page.connection.serverAddress)
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: page.connectionState === Connection.State.Offline
        icon.name: "network-disconnect"
        text: KI18n.i18nc("@info:placeholder %1 is a server address", "%1 does not answer",
                          page.connection.serverAddress)
        explanation: page.connection.errorMessage
        helpfulAction: Kirigami.Action {
            icon.name: "view-refresh"
            text: KI18n.i18nc("@action:button", "Try Again")
            onTriggered: page.connection.restoreLogin()
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !page.loggedIn && page.connectionState !== Connection.State.RestoringLogin
                 && page.connectionState !== Connection.State.Offline
        icon.name: "network-server"
        text: KI18n.i18nc("@info:placeholder", "Not logged in")
        explanation: page.connection.errorMessage
        helpfulAction: Kirigami.Action {
            icon.name: "go-next"
            text: KI18n.i18nc("@action:button", "Log In…")
            onTriggered: page.serverPageRequested()
        }
    }
}
