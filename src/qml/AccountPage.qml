// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The server this app is logged in to, and logging out.
Kirigami.ScrollablePage {
    id: page

    required property Connection connection

    title: KI18n.i18nc("@title", "Account")

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
                onClicked: page.connection.logOut()
            }
        }
    }
}
