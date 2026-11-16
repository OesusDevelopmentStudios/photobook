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
// details. Kept light, as many are made while scrolling: the details exist only while the tile is hovered.
Item {
    id: tile

    required property url thumbnailUrl
    required property string fileName
    required property date takenAt
    required property int imageWidth
    required property int imageHeight

    // Room around the thumbnail: the gap to the next tile, and where the hover highlight shows.
    readonly property real padding: Kirigami.Units.smallSpacing

    HoverHandler {
        id: hover
    }

    // As Breeze highlights a hovered item in a list
    Rectangle {
        anchors.fill: parent
        visible: hover.hovered
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.hoverColor, 0.3)
        border.color: Kirigami.Theme.hoverColor
        border.width: 1
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
            source: tile.thumbnailUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            smooth: true
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
