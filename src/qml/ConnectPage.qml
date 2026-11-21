// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// Finding a pictorio-db server and logging in. What the address means, and what an error says, is decided
// by the Connection; this page only shows it.
Kirigami.ScrollablePage {
    id: page

    required property Connection connection

    readonly property int connectionState: connection.state
    readonly property bool serverKnown: connectionState === Connection.State.ServerFound
                                        || connectionState === Connection.State.LoggingIn
                                        || connectionState === Connection.State.Offline
    readonly property bool busy: connectionState === Connection.State.CheckingServer
                                 || connectionState === Connection.State.LoggingIn
                                 || connectionState === Connection.State.RestoringLogin

    title: KI18n.i18nc("@title", "Connect to Server")

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: page.connection.errorMessage.length > 0
            type: page.connectionState === Connection.State.Offline ? Kirigami.MessageType.Warning
                                                          : Kirigami.MessageType.Error
            text: page.connection.errorMessage
            actions: [
                Kirigami.Action {
                    visible: page.connectionState === Connection.State.Offline
                    icon.name: "view-refresh"
                    text: KI18n.i18nc("@action:button", "Try Again")
                    onTriggered: page.connection.restoreLogin()
                }
            ]
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            RowLayout {
                Kirigami.FormData.label: KI18n.i18nc("@label:textbox", "Server:")

                Controls.TextField {
                    id: addressField

                    Layout.preferredWidth: Kirigami.Units.gridUnit * 16
                    text: page.connection.serverAddress
                    placeholderText: KI18n.i18nc("@info:placeholder", "e.g. raspberrypi.local:8737")
                    enabled: !page.serverKnown && !page.busy
                    inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
                    focus: !page.serverKnown
                    onAccepted: page.connection.checkServer(text)
                }

                Controls.Button {
                    visible: !page.serverKnown
                    enabled: !page.busy && addressField.text.trim().length > 0
                    text: KI18n.i18nc("@action:button Check that the address is a pictorio-db server", "Check")
                    onClicked: page.connection.checkServer(addressField.text)
                }

                Controls.Button {
                    visible: page.serverKnown
                    enabled: !page.busy
                    text: KI18n.i18nc("@action:button Choose another server", "Change")
                    onClicked: page.connection.changeServer()
                }

                Controls.BusyIndicator {
                    visible: page.busy
                    running: visible
                    implicitHeight: addressField.implicitHeight
                }
            }

            Controls.Label {
                visible: page.serverKnown && page.connection.serverDescription.length > 0
                text: "✓ " + page.connection.serverDescription
            }

            Kirigami.InlineMessage {
                Layout.fillWidth: true
                visible: page.serverKnown && page.connection.plainHttp
                type: Kirigami.MessageType.Warning
                text: KI18n.i18nc("@info", "This server uses plain HTTP: your password and your images travel "
                                  + "unencrypted. That is fine in your home network; from the internet, use HTTPS.")
            }

            Controls.TextField {
                id: usernameField

                Kirigami.FormData.label: KI18n.i18nc("@label:textbox", "Username:")
                visible: page.serverKnown
                enabled: !page.busy
                text: page.connection.username
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                onAccepted: passwordField.forceActiveFocus()
            }

            Kirigami.PasswordField {
                id: passwordField

                Kirigami.FormData.label: KI18n.i18nc("@label:textbox", "Password:")
                visible: page.serverKnown
                enabled: !page.busy
                focus: page.serverKnown
                onAccepted: logInButton.clicked()
            }

            Controls.Button {
                id: logInButton

                visible: page.serverKnown
                enabled: !page.busy && usernameField.text.length > 0 && passwordField.text.length > 0
                icon.name: "go-next"
                text: KI18n.i18nc("@action:button", "Log In")
                onClicked: {
                    if (enabled) {
                        page.connection.logIn(usernameField.text, passwordField.text);
                        passwordField.text = "";  // not kept around once sent
                    }
                }
            }
        }
    }
}
