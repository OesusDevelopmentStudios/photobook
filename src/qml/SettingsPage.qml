// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The app's configuration, one section per topic.
Kirigami.ScrollablePage {
    id: page

    required property Connection connection
    required property HiddenFilterEditor hiddenFilterEditor

    // The window asks first about unsaved filter changes, then goes back or logs out.
    signal leaveRequested()  // back, while there are unsaved filter changes
    signal logOutRequested()

    title: KI18n.i18nc("@title", "Settings")

    Component.onCompleted: hiddenFilterEditor.load()

    onBackRequested: event => {
        if (hiddenFilterEditor.hasChanges) {
            event.accepted = true;  // the page stays until the changes are saved or dropped
            leaveRequested();
        }
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: page.connection.keyringWarning.length > 0
            type: Kirigami.MessageType.Warning
            text: page.connection.keyringWarning
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: KI18n.i18nc("@title:group", "Account")
            }

            Controls.Label {
                Kirigami.FormData.label: KI18n.i18nc("@label", "Server:")
                text: page.connection.serverAddress
            }

            Controls.Label {
                Kirigami.FormData.label: KI18n.i18nc("@label", "Logged in as:")
                text: page.connection.username
            }

            Controls.Button {
                icon.name: "system-log-out"
                text: KI18n.i18nc("@action:button", "Log Out")
                objectName: "logOut"
                onClicked: page.logOutRequested()
            }

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: KI18n.i18nc("@title:group", "Hidden Filters")
            }

            HiddenFilterSettings {
                Layout.fillWidth: true
                editor: page.hiddenFilterEditor
            }
        }
    }
}
