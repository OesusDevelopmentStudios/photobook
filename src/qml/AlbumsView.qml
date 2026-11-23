// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the delegate below use the view's ids.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The Albums tab: its sidebar and the albums as tiles, with the library's tile size.
RowLayout {
    id: view

    required property Albums albums
    required property LibraryView libraryView

    signal albumOpenRequested(int row)

    function focusFilter() {
        libraryView.sidebarVisible = true;
        sidebar.focusFilter();
    }

    // As in the library: the album at the top stays in view while the tiles change size.
    function resizeTiles(resize) {
        const topIndex = grid.indexAt(grid.cellWidth / 2, grid.contentY + grid.cellHeight / 2);
        resize();
        grid.forceLayout();
        if (topIndex > 0) {
            grid.positionViewAtIndex(topIndex, GridView.Beginning);
        }
    }

    spacing: 0

    AlbumsSidebar {
        id: sidebar

        Layout.fillHeight: true
        Layout.preferredWidth: Kirigami.Units.gridUnit * 14
        visible: view.libraryView.sidebarVisible
        albums: view.albums
        libraryView: view.libraryView
        onTileSizeRequested: gridUnits => view.resizeTiles(() => view.libraryView.tileSize = gridUnits)
    }

    Kirigami.Separator {
        Layout.fillHeight: true
        visible: sidebar.visible
    }

    // In the colours of a view, as the library's grid
    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        Kirigami.Theme.inherit: false
        color: Kirigami.Theme.backgroundColor

        GridView {
            id: grid

            // As the library's grid; each tile also has room for the album's name and size under the cover.
            readonly property real tileWidth: view.libraryView.tileSize * Kirigami.Units.gridUnit
            readonly property int columns: Math.max(1, Math.floor(width / tileWidth))
            readonly property real thumbnailSize: tileWidth - Kirigami.Units.smallSpacing * 2
            readonly property bool largeThumbnails:
                view.libraryView.needsLargeThumbnail(thumbnailSize * Screen.devicePixelRatio)

            objectName: "albumGrid"
            anchors.fill: parent
            anchors.leftMargin: Kirigami.Units.smallSpacing
            anchors.rightMargin: Kirigami.Units.smallSpacing
            topMargin: Kirigami.Units.smallSpacing
            bottomMargin: Kirigami.Units.smallSpacing
            model: view.albums
            clip: true
            cellWidth: width / columns
            cellHeight: cellWidth + Kirigami.Units.gridUnit * 2.5
            reuseItems: true

            Controls.ScrollBar.vertical: Controls.ScrollBar {
                id: scrollBar
            }

            // Ctrl + wheel resizes the tiles, as in the library
            WheelHandler {
                target: null
                acceptedModifiers: Qt.ControlModifier
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: event => view.resizeTiles(
                    () => view.libraryView.resizeTilesBySteps(event.angleDelta.y / 120))  // 120: one notch
            }

            delegate: AlbumTile {
                width: grid.cellWidth
                height: grid.cellHeight
                thumbnailSize: grid.thumbnailSize
                useLargeThumbnail: grid.largeThumbnails
                onOpenRequested: row => view.albumOpenRequested(row)
            }
        }

        EdgeFade {
            anchors.top: grid.top
            anchors.left: grid.left
            anchors.right: grid.right
            anchors.rightMargin: scrollBar.width
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
            visible: view.albums.errorMessage.length > 0
            type: Kirigami.MessageType.Error
            text: view.albums.errorMessage
            actions: [
                Kirigami.Action {
                    icon.name: "view-refresh"
                    text: KI18n.i18nc("@action:button", "Try Again")
                    onTriggered: view.albums.reload()
                }
            ]
        }

        Controls.BusyIndicator {
            anchors.centerIn: parent
            running: view.albums.loading && view.albums.totalCount === 0
            visible: running
        }

        Kirigami.PlaceholderMessage {
            objectName: "noAlbums"
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: view.albums.count === 0 && !view.albums.loading && view.albums.errorMessage.length === 0
            icon.name: "folder-pictures"
            text: view.albums.totalCount > 0 ? KI18n.i18nc("@info:placeholder", "No album matches")
                                             : KI18n.i18nc("@info:placeholder", "No albums yet")
            explanation: view.albums.totalCount > 0
                         ? view.albums.filterText
                         : KI18n.i18nc("@info:placeholder", "Add an album: tag to images to make one.")
        }
    }
}
