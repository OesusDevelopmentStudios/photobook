// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The app never installs or starts anything itself, so the Flatpak keeps its sandbox: it shows the install
// command to copy into a terminal, and links to the guide for doing it by hand.
Kirigami.ScrollablePage {
    id: page

    // The latest release's script; the guide explains the same steps by hand.
    readonly property string installCommand:
        "curl -fsSL https://github.com/OesusDevelopmentStudios/photobook-db/releases/latest/download/install.sh | sh"
    readonly property url guideUrl:
        "https://github.com/OesusDevelopmentStudios/photobook-db/blob/master/docs/local-server.md"
    // The port chosen when installing; the script's default until changed. Plain HTTP: the server has no HTTPS.
    readonly property bool portValid: portField.acceptableInput
    readonly property string serverAddress: "http://localhost:" + portField.text
    readonly property url webAdminUrl: serverAddress + "/admin"

    signal connectRequested(string address)

    title: KI18n.i18nc("@title", "Use a Local Server")

    function copyCommand() {
        commandField.selectAll();
        commandField.copy();
        commandField.deselect();
        copiedMessage.visible = true;
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: KI18n.i18nc("@info", "Run the photobook-db server on this computer, as a service of your user. "
                              + "It needs Podman.")
        }

        Kirigami.Heading {
            level: 3
            text: KI18n.i18nc("@title:group", "1. Install the Server")
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: KI18n.i18nc("@info", "Copy this command into a terminal. It asks where the images go, each "
                              + "question with a default, and never needs root.")
        }

        RowLayout {
            Layout.fillWidth: true

            Controls.TextField {
                id: commandField

                Layout.fillWidth: true
                readOnly: true
                text: page.installCommand
                font.family: "monospace"
            }

            Controls.Button {
                objectName: "copyCommand"
                icon.name: "edit-copy"
                text: KI18n.i18nc("@action:button", "Copy")
                onClicked: page.copyCommand()
            }
        }

        Kirigami.InlineMessage {
            id: copiedMessage

            objectName: "copiedMessage"
            Layout.fillWidth: true
            visible: false  // once copied
            type: Kirigami.MessageType.Positive
            text: KI18n.i18nc("@info", "Copied. Paste it into a terminal with Ctrl+Shift+V.")
        }

        Controls.Button {
            icon.name: "help-contents"
            text: KI18n.i18nc("@action:button", "Set It Up by Hand: Open the Guide")
            onClicked: Qt.openUrlExternally(page.guideUrl)
        }

        RowLayout {
            Layout.fillWidth: true

            Controls.Label {
                text: KI18n.i18nc("@label:textbox The port number the server listens on", "Port:")
            }

            Controls.TextField {
                id: portField

                objectName: "port"
                text: "8737"
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 1
                    top: 65535
                }
            }

            Controls.Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                color: Kirigami.Theme.disabledTextColor
                text: KI18n.i18nc("@info", "The one you chose when installing; 8737 unless you changed it.")
            }
        }

        Kirigami.Heading {
            level: 3
            text: KI18n.i18nc("@title:group", "2. Create the Accounts")
        }

        Controls.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: KI18n.i18nc("@info", "At its end the command shows a setup code. With it, create the admin "
                              + "account in the web admin, then an account for each person who uses the library.")
        }

        Controls.Button {
            icon.name: "globe"
            text: KI18n.i18nc("@action:button", "Open the Web Admin")
            enabled: page.portValid
            onClicked: Qt.openUrlExternally(page.webAdminUrl)
        }

        Kirigami.Heading {
            level: 3
            text: KI18n.i18nc("@title:group", "3. Connect")
        }

        Controls.Button {
            objectName: "connect"
            icon.name: "network-connect"
            text: KI18n.i18nc("@action:button %1 is a server address", "Connect to %1", page.serverAddress)
            enabled: page.portValid
            onClicked: page.connectRequested(page.serverAddress)
        }
    }
}
