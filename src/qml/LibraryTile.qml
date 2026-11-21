// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the details below use the tile's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// One image in the library grid, its thumbnail cropped square. Hovered, it is highlighted and shows its
// details; clicked, it opens, or while selecting it is selected or not; Ctrl+ and Shift+clicked, it is selected
// as in Dolphin; right-clicked, it asks for its menu. Kept light, as many are made while scrolling: the details
// exist only while the tile is hovered, and the menu is the grid's.
Item {
    id: tile

    required property int index
    required property url thumbnailUrl
    required property url largeThumbnailUrl
    required property string fileName
    required property date takenAt
    required property int imageWidth
    required property int imageHeight
    required property bool selected
    property bool selecting: false  // images are being chosen for changing their tags

    // The size the grid's tiles were chosen at. The tile may stretch a little to fill its row, but the
    // thumbnail is decoded at this size, so stretching the window does not load every thumbnail again.
    property real thumbnailSize: 0
    property bool useLargeThumbnail: false  // the small one would look blurry

    // Room around the thumbnail: the gap to the next tile, and where the hover highlight shows. The grid
    // counts on it being smallSpacing.
    readonly property real padding: Kirigami.Units.smallSpacing

    signal openRequested(int row)
    signal selectionToggled(int row)                       // Ctrl+click, or a click while selecting
    signal rangeSelected(int row, bool addToSelection)  // Shift+click; with Ctrl, added to the selection
    signal menuRequested(int row)                          // right-click

    HoverHandler {
        id: hover
    }

    // Taps, not the start of a scroll: the grid takes over a drag. Each handler takes only its modifiers.
    TapHandler {
        acceptedModifiers: Qt.NoModifier
        onTapped: tile.selecting ? tile.selectionToggled(tile.index) : tile.openRequested(tile.index)
    }

    TapHandler {
        acceptedModifiers: Qt.ControlModifier
        onTapped: tile.selectionToggled(tile.index)
    }

    TapHandler {
        acceptedModifiers: Qt.ShiftModifier
        onTapped: tile.rangeSelected(tile.index, false)
    }

    TapHandler {
        acceptedModifiers: Qt.ControlModifier | Qt.ShiftModifier
        onTapped: tile.rangeSelected(tile.index, true)
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: tile.menuRequested(tile.index)
    }

    // As Breeze highlights a hovered or a selected item in a list
    Rectangle {
        anchors.fill: parent
        visible: hover.hovered || tile.selected
        radius: Kirigami.Units.cornerRadius
        color: tile.selected ? Qt.alpha(Kirigami.Theme.highlightColor, 0.5) : Qt.alpha(Kirigami.Theme.hoverColor, 0.3)
        border.color: tile.selected ? Kirigami.Theme.highlightColor : Kirigami.Theme.hoverColor
        border.width: tile.selected ? 2 : 1
    }

    // Shows until the thumbnail has loaded
    Rectangle {
        id: frame

        anchors.fill: parent
        anchors.margins: tile.padding
        color: Kirigami.Theme.alternateBackgroundColor

        // No clip needed: a cropped image stays inside its own bounds.
        Image {
            anchors.fill: parent
            source: tile.useLargeThumbnail ? tile.largeThumbnailUrl : tile.thumbnailUrl
            sourceSize: Qt.size(tile.thumbnailSize, tile.thumbnailSize)  // keeps memory low with large thumbnails
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
        }
    }

    // Selected: tinted, so a selection reads at a glance next to a hovered tile
    Rectangle {
        anchors.fill: frame
        visible: tile.selected
        color: Qt.alpha(Kirigami.Theme.highlightColor, 0.25)
    }

    // Selected: a check in the corner as well
    Rectangle {
        anchors.top: frame.top
        anchors.left: frame.left
        anchors.margins: Kirigami.Units.smallSpacing
        width: Kirigami.Units.iconSizes.small + Kirigami.Units.smallSpacing
        height: width
        radius: width / 2
        visible: tile.selected
        color: Kirigami.Theme.highlightColor

        Kirigami.Icon {
            anchors.centerIn: parent
            width: Kirigami.Units.iconSizes.small
            height: width
            source: "checkmark"
            color: Kirigami.Theme.highlightedTextColor
        }
    }

    Loader {
        anchors.left: frame.left
        anchors.right: frame.right
        anchors.bottom: frame.bottom
        active: hover.hovered

        sourceComponent: Rectangle {
            implicitHeight: details.implicitHeight + Kirigami.Units.smallSpacing * 2
            color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.8)

            ColumnLayout {
                id: details

                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                spacing: 0

                Controls.Label {
                    Layout.fillWidth: true
                    text: tile.fileName
                    elide: Text.ElideMiddle  // keeps the file's extension in view
                }

                Controls.Label {
                    Layout.fillWidth: true
                    text: KI18n.i18nc("@info %1 is the date taken, %2 × %3 the size in pixels", "%1 · %2 × %3",
                                      tile.takenAt.toLocaleDateString(Qt.locale(), Locale.ShortFormat),
                                      tile.imageWidth, tile.imageHeight)
                    font: Kirigami.Theme.smallFont
                    elide: Text.ElideRight
                }
            }
        }
    }
}
