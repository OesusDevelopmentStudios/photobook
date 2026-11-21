// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the rows below use the panel's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The same rows as the viewer's edit view, with each namespace's name above its row for the narrow sidebar
ColumnLayout {
    id: panel

    required property BulkTagEditor editor
    required property Library library

    readonly property bool tooMany: editor.imageCount > editor.maximumImageCount

    spacing: Kirigami.Units.smallSpacing

    RowLayout {
        Layout.fillWidth: true

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 4
            text: KI18n.i18ncp("@title:group", "%1 image selected", "%1 images selected", panel.editor.imageCount)
            wrapMode: Text.Wrap
        }

        Controls.ToolButton {
            icon.name: "edit-select-none"
            text: KI18n.i18nc("@action:button", "Clear Selection")
            display: Controls.AbstractButton.IconOnly
            enabled: panel.editor.imageCount > 0
            onClicked: panel.library.clearSelection()

            Controls.ToolTip.text: text
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }

    Controls.Label {
        Layout.fillWidth: true
        visible: panel.editor.imageCount === 0
        text: KI18n.i18nc("@info", "Click images to select them. Shift+click selects a range.")
        wrapMode: Text.Wrap
        color: Kirigami.Theme.disabledTextColor
    }

    Controls.ScrollView {
        id: rowsScroll

        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: availableWidth

        ColumnLayout {
            width: rowsScroll.availableWidth
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: panel.editor.namespaceRows

                delegate: ColumnLayout {
                    id: namespaceRow

                    required property string namespaceName
                    required property bool dropdown
                    required property bool fixedValues
                    required property bool keepable
                    required property var options
                    required property string value
                    required property bool changed
                    required property var tags

                    Layout.fillWidth: true
                    Layout.bottomMargin: Kirigami.Units.smallSpacing
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label {
                        text: namespaceRow.namespaceName + ":"  // as written in a tag, e.g. "author:"
                        color: Kirigami.Theme.disabledTextColor
                    }

                    TagRowContent {
                        Layout.fillWidth: true
                        editor: panel.editor
                        namespaceName: namespaceRow.namespaceName
                        dropdown: namespaceRow.dropdown
                        fixedValues: namespaceRow.fixedValues
                        keepable: namespaceRow.keepable
                        options: namespaceRow.options
                        value: namespaceRow.value
                        changed: namespaceRow.changed
                        tags: namespaceRow.tags
                    }
                }
            }
        }
    }

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: text.length > 0
        type: Kirigami.MessageType.Error
        text: panel.editor.errorMessage
    }

    Controls.Label {
        Layout.fillWidth: true
        visible: panel.tooMany
        text: KI18n.i18nc("@info %1 is the most images", "At most %1 images can be changed at once.",
                          panel.editor.maximumImageCount)
        wrapMode: Text.Wrap
        color: Kirigami.Theme.disabledTextColor
    }

    RowLayout {
        Layout.fillWidth: true

        Controls.Button {
            Layout.fillWidth: true
            icon.name: "dialog-ok-apply"
            text: KI18n.i18ncp("@action:button", "Apply to %1 Image", "Apply to %1 Images", panel.editor.imageCount)
            enabled: panel.editor.hasChanges && !panel.editor.applying && !panel.tooMany
                     && panel.editor.imageCount > 0
            onClicked: panel.editor.apply()
        }

        Controls.BusyIndicator {
            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            visible: running
            running: panel.editor.applying
        }

        Controls.Button {
            icon.name: "edit-clear"
            text: KI18n.i18nc("@action:button Drop the tag changes", "Clear")
            display: Controls.AbstractButton.IconOnly
            enabled: panel.editor.hasChanges && !panel.editor.applying
            onClicked: panel.editor.clear()

            Controls.ToolTip.text: KI18n.i18nc("@info:tooltip", "Drop these changes")
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }
}
