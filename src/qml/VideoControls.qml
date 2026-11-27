// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the speed menu's items use the controls' id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// A video's controls: play/pause, where it is, the scrub bar, its length, the speed and the sound. Playing and
// seeking are asked for, not done: the viewer drives Qt's MediaPlayer. Speed and sound are the player's settings.
Rectangle {
    id: controls

    required property VideoPlayer videoPlayer
    property bool playing: false
    property real position: 0  // milliseconds
    property real duration: 0
    property bool seekable: false

    signal playPauseRequested
    signal seekRequested(real position)

    // "1×", "0.25×", "1.5×"
    function speedText(speed: real): string {
        return KI18n.i18nc("@action:button %1 is a playback speed, e.g. 1.5", "%1×",
                           speed.toLocaleString(Qt.locale(), "g", 3));
    }

    implicitHeight: row.implicitHeight + Kirigami.Units.smallSpacing * 2
    color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.85)

    RowLayout {
        id: row

        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.smallSpacing
        anchors.rightMargin: Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.smallSpacing

        Controls.ToolButton {
            objectName: "playPauseButton"
            icon.name: controls.playing ? "media-playback-pause" : "media-playback-start"
            text: controls.playing ? KI18n.i18nc("@action:button", "Pause") : KI18n.i18nc("@action:button", "Play")
            display: Controls.AbstractButton.IconOnly
            Controls.ToolTip.text: text
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: controls.playPauseRequested()
        }

        Controls.Label {
            objectName: "positionLabel"
            text: controls.videoPlayer.timeText(scrubBar.pressed ? scrubBar.value : controls.position)
            font.features: {"tnum": 1}  // the width stays while the numbers change
        }

        Controls.Slider {
            id: scrubBar

            objectName: "scrubBar"
            Layout.fillWidth: true
            from: 0
            to: Math.max(controls.duration, 1)
            enabled: controls.seekable && controls.duration > 0
            Accessible.name: KI18n.i18nc("@label:slider", "Position in the video")
            onMoved: controls.seekRequested(value)

            // Follows the video, except while dragged
            Binding on value {
                when: !scrubBar.pressed
                value: controls.position
            }
        }

        Controls.Label {
            objectName: "durationLabel"
            text: controls.videoPlayer.timeText(controls.duration)
            font.features: {"tnum": 1}
        }

        Controls.ToolButton {
            objectName: "speedButton"
            text: controls.speedText(controls.videoPlayer.speed)
            Controls.ToolTip.text: KI18n.i18nc("@info:tooltip", "Playback speed")
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: speedMenu.popup(this, 0, -speedMenu.implicitHeight)

            Controls.Menu {
                id: speedMenu

                objectName: "speedMenu"

                Repeater {
                    model: controls.videoPlayer.speeds

                    Controls.MenuItem {
                        required property real modelData

                        text: controls.speedText(modelData)
                        checkable: true
                        checked: modelData === controls.videoPlayer.speed
                        onTriggered: controls.videoPlayer.speed = modelData
                    }
                }
            }
        }

        Controls.ToolButton {
            objectName: "muteButton"
            readonly property bool silent: controls.videoPlayer.muted || controls.videoPlayer.volume === 0

            icon.name: silent ? "audio-volume-muted" : "audio-volume-high"
            text: controls.videoPlayer.muted ? KI18n.i18nc("@action:button", "Unmute")
                                             : KI18n.i18nc("@action:button", "Mute")
            display: Controls.AbstractButton.IconOnly
            Controls.ToolTip.text: text
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
            onClicked: controls.videoPlayer.muted = !controls.videoPlayer.muted
        }

        Controls.Slider {
            objectName: "volumeSlider"
            Layout.preferredWidth: Kirigami.Units.gridUnit * 5
            from: 0
            to: 1
            value: controls.videoPlayer.volume
            Accessible.name: KI18n.i18nc("@label:slider", "Volume")
            onMoved: {
                controls.videoPlayer.volume = value;
                if (value > 0) {
                    controls.videoPlayer.muted = false;  // turning it up means to hear it
                }
            }
        }
    }
}
