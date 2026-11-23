// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the tabs below use the bar's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The library page's tabs, Library and Albums, drawn as the web admin's: flat, as tall as the header, the current
// one tinted with the accent colour and underlined with it. Centred on centreOf (the page), not just in the room
// the header leaves them, as long as they fit there.
Item {
    id: bar

    required property LibraryView libraryView
    property Item centreOf: parent

    component HeaderTab: Controls.AbstractButton {
        id: tab

        property bool current: false

        height: bar.height
        leftPadding: Kirigami.Units.gridUnit
        rightPadding: Kirigami.Units.gridUnit
        Accessible.role: Accessible.PageTab
        Accessible.selected: current

        contentItem: Controls.Label {
            text: tab.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: tab.current || tab.hovered ? Qt.alpha(Kirigami.Theme.highlightColor, 0.22) : "transparent"
            border.width: tab.visualFocus ? 1 : 0
            border.color: Kirigami.Theme.highlightColor

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 3
                visible: tab.current
                color: Kirigami.Theme.highlightColor
            }
        }
    }

    implicitWidth: tabs.implicitWidth
    implicitHeight: tabs.implicitHeight

    Row {
        id: tabs

        // Where the middle of centreOf is, in the bar; recomputed as either moves or changes size.
        readonly property real centredX: {
            bar.width;  // a dependency: mapToItem() alone is not one
            const origin = bar.mapToItem(bar.centreOf, 0, 0).x;
            return (bar.centreOf.width - width) / 2 - origin;
        }

        x: Math.max(0, Math.min(bar.width - width, centredX))
        height: bar.height

        HeaderTab {
            objectName: "libraryTab"
            text: KI18n.i18nc("@title:tab", "Library")
            current: bar.libraryView.tab === LibraryView.Tab.Library
            onClicked: bar.libraryView.tab = LibraryView.Tab.Library
        }

        HeaderTab {
            objectName: "albumsTab"
            text: KI18n.i18nc("@title:tab", "Albums")
            current: bar.libraryView.tab === LibraryView.Tab.Albums
            onClicked: bar.libraryView.tab = LibraryView.Tab.Albums
        }
    }
}
