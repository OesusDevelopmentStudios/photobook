// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

Kirigami.Page {
    id: page

    required property Connection connection

    signal openImageRequested()
    signal openFolderRequested()
    signal serverPageRequested()
    signal localServerRequested()
    signal libraryRequested()

    readonly property bool loggedIn: connection.state === Connection.State.LoggedIn

    title: KI18n.i18nc("@title", "Welcome")

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - Kirigami.Units.gridUnit * 4, Kirigami.Units.gridUnit * 24)
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: Kirigami.Units.iconSizes.enormous
            implicitHeight: Kirigami.Units.iconSizes.enormous
            source: "io.github.oesusdevelopmentstudios.pictorio"
        }

        Kirigami.Heading {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: KI18n.i18nc("@title", "Welcome to Pictorio")
        }

        Controls.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            visible: page.loggedIn || page.connection.state === Connection.State.Offline
            text: page.loggedIn
                ? KI18n.i18nc("@info %1 is a user name, %2 a server address", "Logged in as %1 on %2.",
                              page.connection.username, page.connection.serverAddress)
                : KI18n.i18nc("@info %1 is a server address", "%1 does not answer right now.",
                              page.connection.serverAddress)
        }

        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14
            icon.name: "document-open"
            text: KI18n.i18nc("@action:button", "Open Image…")
            onClicked: page.openImageRequested()
        }

        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14
            icon.name: "folder-open"
            text: KI18n.i18nc("@action:button", "Open Folder…")
            onClicked: page.openFolderRequested()
        }

        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14
            icon.name: page.loggedIn ? "view-preview" : "network-server"
            text: page.loggedIn ? KI18n.i18nc("@action:button", "Library")
                                : KI18n.i18nc("@action:button", "Use a Remote Server…")
            onClicked: page.loggedIn ? page.libraryRequested() : page.serverPageRequested()
        }

        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14
            visible: !page.loggedIn
            icon.name: "computer"
            text: KI18n.i18nc("@action:button", "Use a Local Server…")
            onClicked: page.localServerRequested()
        }
    }
}
