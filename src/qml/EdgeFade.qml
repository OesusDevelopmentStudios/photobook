// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import org.kde.kirigami as Kirigami

// From the background colour to transparent, over the edge of a grid. Takes no input, so tiles under it are still
// hovered.
Rectangle {
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
