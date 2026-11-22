// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

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
            id: accountForm

            Layout.fillWidth: true
            twinFormLayouts: [serverManagementForm, hiddenFiltersForm]

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
        }

        // Outside the form, so that it sits in the middle of the page rather than in the form's field column.
        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            icon.name: "system-log-out"
            text: KI18n.i18nc("@action:button", "Log Out")
            objectName: "logOut"
            onClicked: page.logOutRequested()
        }

        Kirigami.FormLayout {
            id: serverManagementForm

            Layout.fillWidth: true
            twinFormLayouts: [accountForm, hiddenFiltersForm]

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: KI18n.i18nc("@title:group", "Server Management")
            }
        }

        // Shown to every account: a convenient way to the web admin, which asks for the admin's password itself.
        Controls.Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: Kirigami.Units.gridUnit * 25
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: KI18n.i18nc("@info", "Accounts, namespaces, tags and imports are managed in the server's web "
                              + "admin. It needs the server's admin account.")
        }

        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            icon.name: "globe"
            text: KI18n.i18nc("@action:button", "Open the Web Admin")
            objectName: "openWebAdmin"
            onClicked: Qt.openUrlExternally(page.connection.webAdminUrl)
        }

        Kirigami.FormLayout {
            id: hiddenFiltersForm

            Layout.fillWidth: true
            twinFormLayouts: [accountForm, serverManagementForm]

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
