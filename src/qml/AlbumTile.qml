// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the parts below use the tile's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// An album of the Albums tab: its first image as the cover, under it its name and how many images it holds. Looks
// like a library tile when hovered.
Item {
    id: tile

    required property int index
    required property url coverUrl
    required property url largeCoverUrl
    required property string name
    required property int imageCount

    // As for a library tile: the cover is decoded at the chosen tile size, not at the stretched one.
    property real thumbnailSize: 0
    property bool useLargeThumbnail: false

    signal openRequested(int row)

    HoverHandler {
        id: hover
    }

    TapHandler {
        onTapped: tile.openRequested(tile.index)
    }

    Rectangle {
        anchors.fill: parent
        visible: hover.hovered
        radius: Kirigami.Units.cornerRadius
        color: Qt.alpha(Kirigami.Theme.hoverColor, 0.3)
        border.color: Kirigami.Theme.hoverColor
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.smallSpacing  // the gap to the next tile, as in the library
        spacing: 0

        // Shows until the cover has loaded
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: width
            color: Kirigami.Theme.alternateBackgroundColor

            Image {
                anchors.fill: parent
                source: tile.useLargeThumbnail ? tile.largeCoverUrl : tile.coverUrl
                sourceSize: Qt.size(tile.thumbnailSize, tile.thumbnailSize)
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                smooth: true
            }
        }

        Controls.Label {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.smallSpacing
            text: tile.name
            elide: Text.ElideRight
        }

        Controls.Label {
            Layout.fillWidth: true
            text: KI18n.i18ncp("@info", "%1 image", "%1 images", tile.imageCount)
            font: Kirigami.Theme.smallFont
            color: Kirigami.Theme.disabledTextColor
            elide: Text.ElideRight
        }
    }
}
