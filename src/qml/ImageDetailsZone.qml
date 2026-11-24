// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import org.kde.kirigami as Kirigami

// The bottom edge of the viewer: the pointer near it brings in the details footer. The footer is inside the
// zone: as a sibling on top, it would take the hover from the zone, hide, give it back, and flicker.
Item {
    id: zone

    required property Viewer viewer
    required property TagEditor tagEditor
    property TagPaneLayout tagPaneLayout: null
    property bool pinned: false  // shown without the pointer near, e.g. while the tag pane is open
    property real availableHeight: 0  // up to the toolbar, while the tags are edited
    readonly property ImageDetailsFooter footer: detailsFooter

    height: detailsFooter.implicitHeight + Kirigami.Units.gridUnit * 2

    HoverHandler {
        id: hover
    }

    ImageDetailsFooter {
        id: detailsFooter

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        viewer: zone.viewer
        tagEditor: zone.tagEditor
        tagPaneLayout: zone.tagPaneLayout
        shown: hover.hovered || expanded || zone.pinned  // expanded, it stays until collapsed
        availableHeight: zone.availableHeight
    }
}
