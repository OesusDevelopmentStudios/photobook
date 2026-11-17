// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the chips below use the settings' id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The user's hidden filters as chips, with an add field, as the tag rows have them. Changes are sent together
// with Save; Cancel drops them.
ColumnLayout {
    id: settings

    required property HiddenFilterEditor editor

    spacing: Kirigami.Units.smallSpacing

    Controls.Label {
        Layout.fillWidth: true
        text: KI18n.i18nc("@info",
                          "The library leaves out images that match these filters, unless a search names them.")
        wrapMode: Text.Wrap
    }

    Flow {
        Layout.fillWidth: true
        visible: chips.count > 0
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            id: chips

            objectName: "filterChips"
            model: settings.editor.filters

            delegate: TagChip {
                editor: settings.editor
            }
        }
    }

    Controls.Label {
        objectName: "noFilters"
        visible: chips.count === 0 && !settings.editor.loading
        text: KI18n.i18nc("@info", "No hidden filters.")
        color: Kirigami.Theme.disabledTextColor
    }

    TagNameField {
        objectName: "addFilter"
        Layout.fillWidth: true
        editor: settings.editor
        namespaceName: ""  // a whole search term
        placeholderText: KI18n.i18nc("@info:placeholder", "Add a filter, e.g. rating:nsfw…")
    }

    Kirigami.InlineMessage {
        objectName: "errorMessage"
        Layout.fillWidth: true
        visible: settings.editor.errorMessage.length > 0
        type: Kirigami.MessageType.Error
        text: settings.editor.errorMessage
    }

    RowLayout {
        spacing: Kirigami.Units.smallSpacing

        Controls.Button {
            objectName: "saveFilters"
            icon.name: "document-save"
            text: KI18n.i18nc("@action:button", "Save")
            enabled: settings.editor.hasChanges && !settings.editor.busy
            onClicked: settings.editor.save()
        }

        Controls.Button {
            objectName: "cancelFilters"
            icon.name: "dialog-cancel"
            text: KI18n.i18nc("@action:button", "Cancel")
            enabled: settings.editor.hasChanges && !settings.editor.busy
            onClicked: settings.editor.cancel()
        }

        Controls.BusyIndicator {
            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            visible: settings.editor.busy
            running: visible
        }
    }
}
