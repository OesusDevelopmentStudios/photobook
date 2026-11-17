// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Shapes
import org.kde.kirigami as Kirigami

// A running upload in the toolbar: the upload icon in a ring that fills as the files are done. Its action gives
// the icon, the text for the tooltip, and what a click does.
Controls.ToolButton {
    id: indicator

    property real progress: 0  // 0 to 1

    display: Controls.AbstractButton.IconOnly
    // Smaller than other toolbar icons, so the ring around it fits the button
    icon.width: Kirigami.Units.iconSizes.small
    icon.height: Kirigami.Units.iconSizes.small

    Controls.ToolTip.text: text
    Controls.ToolTip.visible: hovered
    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay

    Shape {
        id: ring

        readonly property real lineWidth: 2
        readonly property real radius: Math.min(width, height) / 2 - lineWidth

        anchors.centerIn: parent
        width: indicator.icon.width + Kirigami.Units.smallSpacing * 2 + lineWidth * 2
        height: width
        preferredRendererType: Shape.CurveRenderer  // smooth without multisampling

        // The whole ring, faint
        ShapePath {
            fillColor: "transparent"
            strokeColor: Qt.alpha(Kirigami.Theme.textColor, 0.2)
            strokeWidth: ring.lineWidth

            PathAngleArc {
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.radius
                radiusY: ring.radius
                startAngle: 0
                sweepAngle: 360
            }
        }

        // The part done, clockwise from the top
        ShapePath {
            fillColor: "transparent"
            strokeColor: Kirigami.Theme.highlightColor
            strokeWidth: ring.lineWidth
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                objectName: "doneArc"
                centerX: ring.width / 2
                centerY: ring.height / 2
                radiusX: ring.radius
                radiusY: ring.radius
                startAngle: -90
                sweepAngle: 360 * Math.max(0, Math.min(1, indicator.progress))
            }
        }
    }
}
