// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the sections below use the pane's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The viewer's tags, as on a booru: a heading per namespace, under it the tags, each with how many images have
// it. A click on a heading folds its section to the heading and the number of its tags; the folded namespaces are
// remembered for every image. "Edit Tags" at the bottom edits them here: ✕ removes a tag (struck through until
// saved), + at the end of a section opens a field for new ones, category and rating have a dropdown. Every
// namespace users tag with gets a section then; meta stays without controls.
Rectangle {
    id: pane

    required property TagEditor tagEditor
    required property TagPaneLayout layout

    readonly property bool editing: tagEditor.editing

    // A count as boorus write it: 412, 1.2k, 33k, 4.5m
    function shortCount(count: int): string {
        if (count < 1000) {
            return String(count);
        }
        const millions = count >= 999500;  // 999.5k would round to 1000k
        const value = millions ? count / 1000000 : count / 1000;
        const oneDecimal = value < 9.95;  // 9,999 is 10k, not 10.0k
        const shown = Qt.locale().toString(oneDecimal ? value : Math.round(value), "f", oneDecimal ? 1 : 0);
        return millions ? KI18n.i18nc("@info A number of images in millions, e.g. 4.5m", "%1m", shown)
                        : KI18n.i18nc("@info A number of images in thousands, e.g. 1.2k", "%1k", shown);
    }

    // "general" → "General", "art_style" → "Art style"
    function heading(namespaceName: string): string {
        const words = namespaceName.replace(/_/g, " ");
        return words.charAt(0).toUpperCase() + words.slice(1);
    }

    // The built-in namespaces in the theme's named colours, so they suit light and dark; the others plain
    function tagColor(namespaceName: string): color {
        switch (namespaceName) {
        case "author":
            return Kirigami.Theme.neutralTextColor;
        case "character":
            return Kirigami.Theme.positiveTextColor;
        case "album":
            return Kirigami.Theme.linkColor;
        case "category":
            return Kirigami.Theme.visitedLinkColor;
        case "rating":
            return Kirigami.Theme.negativeTextColor;
        case "meta":
            return Kirigami.Theme.activeTextColor;
        }
        return Kirigami.Theme.textColor;
    }

    // The window's colours, as the library's sidebar
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false
    color: Kirigami.Theme.backgroundColor

    // A small flat button with an icon only, on a tag's line or at a section's end
    component LineButton: Controls.ToolButton {
        implicitWidth: Kirigami.Units.iconSizes.small + Kirigami.Units.smallSpacing * 2
        implicitHeight: implicitWidth
        padding: Kirigami.Units.smallSpacing / 2
        icon.width: Kirigami.Units.iconSizes.small
        icon.height: Kirigami.Units.iconSizes.small
        display: Controls.AbstractButton.IconOnly

        Controls.ToolTip.text: text
        Controls.ToolTip.visible: hovered
        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.smallSpacing  // room for the scroll bar
        spacing: Kirigami.Units.smallSpacing

        ListView {
            id: sections

            objectName: "tagSections"
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: pane.tagEditor.namespaceRows
            clip: true
            spacing: Kirigami.Units.smallSpacing

            Controls.ScrollBar.vertical: Controls.ScrollBar {}

            delegate: ColumnLayout {
                id: section

                required property string namespaceName
                required property var tags
                required property bool editable
                required property bool dropdown
                required property bool fixedValues
                required property bool keepable
                required property var options
                required property string value
                required property bool changed

                readonly property bool folded: pane.layout.foldedNamespaces.includes(namespaceName)
                readonly property bool withControls: pane.editing && editable
                readonly property real indent: Kirigami.Units.iconSizes.small + Kirigami.Units.smallSpacing
                property bool adding: false  // the field for new tags is open

                width: ListView.view.width - Kirigami.Units.largeSpacing  // clear of the scroll bar
                spacing: 0

                Controls.AbstractButton {
                    id: headingButton

                    objectName: "heading_" + section.namespaceName
                    Layout.fillWidth: true
                    text: section.folded
                          ? KI18n.i18nc("@title %1 is a namespace, %2 how many tags the image has in it", "%1 (%2)",
                                        pane.heading(section.namespaceName), section.tags.length)
                          : pane.heading(section.namespaceName)
                    onClicked: pane.layout.setFolded(section.namespaceName, !section.folded)

                    Accessible.role: Accessible.Button
                    Accessible.name: text

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing

                        Kirigami.Icon {
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                            source: section.folded ? "arrow-right" : "arrow-down"
                        }

                        Controls.Label {
                            Layout.fillWidth: true
                            text: headingButton.text
                            font.bold: true
                            elide: Text.ElideRight
                        }
                    }

                    background: Rectangle {
                        radius: Kirigami.Units.cornerRadius
                        color: headingButton.hovered ? Qt.alpha(Kirigami.Theme.highlightColor, 0.22) : "transparent"
                        border.width: headingButton.visualFocus ? 1 : 0
                        border.color: Kirigami.Theme.highlightColor
                    }
                }

                // Category and rating while editing: one value, chosen
                TagValueBox {
                    objectName: "valueBox_" + section.namespaceName
                    Layout.fillWidth: true
                    Layout.leftMargin: section.indent
                    visible: section.withControls && section.dropdown && !section.folded
                    editor: pane.tagEditor
                    namespaceName: section.namespaceName
                    options: section.options
                    value: section.value
                    changed: section.changed
                    fixedValues: section.fixedValues
                    keepable: section.keepable
                }

                Repeater {
                    model: section.folded || (section.withControls && section.dropdown) ? [] : section.tags

                    // The count right after the name, as on boorus; a long name gives way to it and the button.
                    delegate: Row {
                        id: tagLine

                        required property var modelData
                        readonly property bool isAdded: modelData.change === TagRowsEditor.Change.Added
                        readonly property bool isRemoved: modelData.change === TagRowsEditor.Change.Removed

                        Layout.leftMargin: section.indent  // under the heading's text
                        // As tall as the line's content; the count's baseline anchor would make the row taller.
                        Layout.preferredHeight: removeButton.visible ? removeButton.implicitHeight : name.implicitHeight
                        spacing: Kirigami.Units.smallSpacing

                        // New, not saved yet
                        Kirigami.Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: Kirigami.Units.iconSizes.small
                            visible: tagLine.isAdded
                            source: "list-add"
                        }

                        Controls.Label {
                            id: name

                            objectName: "tag_" + tagLine.modelData.tag
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.min(implicitWidth,
                                            section.width - section.indent - count.implicitWidth
                                            - (removeButton.visible ? removeButton.implicitWidth : 0)
                                            - (tagLine.isAdded ? Kirigami.Units.iconSizes.small : 0)
                                            - tagLine.spacing * 3)
                            text: tagLine.modelData.name.replace(/_/g, " ")
                            color: pane.tagColor(section.namespaceName)
                            font.strikeout: tagLine.isRemoved
                            opacity: tagLine.isRemoved ? 0.6 : 1
                            elide: Text.ElideRight
                        }

                        Controls.Label {
                            id: count

                            objectName: "count_" + tagLine.modelData.tag
                            anchors.baseline: name.baseline  // the smaller font on the name's line
                            visible: tagLine.modelData.count >= 0  // until the counts arrive, the names alone
                            text: pane.shortCount(tagLine.modelData.count)
                            color: Kirigami.Theme.disabledTextColor
                            font: Kirigami.Theme.smallFont
                        }

                        // ✕ removes; a removed tag is kept after all
                        LineButton {
                            id: removeButton

                            objectName: "remove_" + tagLine.modelData.tag
                            anchors.verticalCenter: parent.verticalCenter
                            visible: section.withControls
                            enabled: !pane.tagEditor.busy
                            icon.name: tagLine.isRemoved ? "edit-undo" : "edit-delete-remove"
                            text: tagLine.isRemoved ? KI18n.i18nc("@action:button", "Keep This Tag")
                                                    : KI18n.i18nc("@action:button", "Remove This Tag")
                            onClicked: tagLine.isRemoved ? pane.tagEditor.restoreTag(tagLine.modelData.tag)
                                                         : pane.tagEditor.removeTag(tagLine.modelData.tag)
                        }
                    }
                }

                // Open while tags are added one after another: Enter adds and keeps it open; Escape or the focus
                // moving elsewhere closes it. What is typed and not added stays a change.
                TagNameField {
                    id: addField

                    objectName: "addField_" + section.namespaceName
                    Layout.fillWidth: true
                    Layout.leftMargin: section.indent
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    visible: section.withControls && !section.dropdown && !section.folded && section.adding
                    editor: pane.tagEditor
                    namespaceName: section.namespaceName

                    Keys.onEscapePressed: event => {
                        section.adding = false;
                        event.accepted = true;  // not the viewer's Escape
                    }
                    onActiveFocusChanged: {
                        if (!activeFocus) {
                            section.adding = false;
                        }
                    }
                }

                LineButton {
                    objectName: "add_" + section.namespaceName
                    Layout.leftMargin: Kirigami.Units.iconSizes.small
                    visible: section.withControls && !section.dropdown && !section.folded && !section.adding
                    enabled: !pane.tagEditor.busy
                    icon.name: "list-add"
                    text: KI18n.i18nc("@action:button %1 is a namespace", "Add a Tag to %1",
                                      pane.heading(section.namespaceName))
                    onClicked: {
                        section.adding = true;
                        addField.forceActiveFocus();
                    }
                }
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: pane.editing && pane.tagEditor.errorMessage.length > 0
            type: Kirigami.MessageType.Error
            text: pane.tagEditor.errorMessage
        }

        // At the end of the pane, also when its sections scroll
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Controls.Button {
                objectName: "editTags"
                Layout.fillWidth: true
                visible: !pane.editing
                enabled: pane.tagEditor.canEdit
                icon.name: "document-edit"
                text: KI18n.i18nc("@action:button", "Edit Tags")
                onClicked: pane.tagEditor.startEditing()
            }

            Controls.BusyIndicator {
                Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                visible: running
                running: pane.tagEditor.saving
            }

            Controls.Button {
                objectName: "saveTags"
                Layout.fillWidth: true
                visible: pane.editing
                enabled: pane.tagEditor.hasChanges && !pane.tagEditor.saving
                icon.name: "dialog-ok-apply"
                text: KI18n.i18nc("@action:button Save the tag changes", "Save")
                onClicked: pane.tagEditor.save()
            }

            Controls.Button {
                objectName: "cancelTags"
                Layout.fillWidth: true
                visible: pane.editing
                enabled: !pane.tagEditor.saving
                icon.name: "dialog-cancel"
                text: KI18n.i18nc("@action:button Drop the tag changes", "Cancel")
                onClicked: pane.tagEditor.cancel()
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 2
        visible: !pane.editing && sections.count === 0
        icon.name: "tag"
        text: KI18n.i18nc("@info:placeholder", "No tags")
    }
}
