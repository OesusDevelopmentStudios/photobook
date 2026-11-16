// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The name and details of the image on screen, over the bottom of the image. The page decides when it shows;
// it slides in and out.
Rectangle {
    id: footer

    required property Viewer viewer
    property bool shown: false

    // The details after the name, e.g. "14/07/2025 · 4032 × 3024 · 3.1 MB"; what is not known is left out.
    readonly property string summary: {
        const parts = [];
        if (!isNaN(viewer.takenAt.getTime())) {
            parts.push(viewer.takenAt.toLocaleDateString(Qt.locale(), Locale.ShortFormat));
        }
        parts.push(KI18n.i18nc("@info %1 × %2 is a size in pixels", "%1 × %2",
                               viewer.imageDisplaySize.width, viewer.imageDisplaySize.height));
        if (viewer.fileSize > 0) {
            parts.push(Qt.locale().formattedDataSize(viewer.fileSize));
        }
        return parts.join(" · ");
    }

    // 0 hidden, 1 shown; in between while sliding
    property real reveal: shown ? 1 : 0

    implicitHeight: content.implicitHeight + Kirigami.Units.smallSpacing * 2
    visible: reveal > 0
    opacity: reveal
    transform: Translate {
        y: (1 - footer.reveal) * footer.height
    }
    color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.85)

    Behavior on reveal {
        NumberAnimation {
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
    }

    // The image underneath does not react to clicks and drags on the footer.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
    }

    RowLayout {
        id: content

        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.largeSpacing
        anchors.topMargin: Kirigami.Units.smallSpacing
        anchors.bottomMargin: Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.largeSpacing

        Controls.Label {
            Layout.fillWidth: true
            text: footer.viewer.fileName
            elide: Text.ElideMiddle  // keeps the file's extension in view
            font.bold: true
        }

        Controls.Label {
            text: footer.summary
        }
    }
}
