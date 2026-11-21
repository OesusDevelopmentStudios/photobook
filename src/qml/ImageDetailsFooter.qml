// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the details below use the footer's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// Over the bottom of the image; the page decides when it shows. Expanded, it shows all details, a library
// image's tags included.
Rectangle {
    id: footer

    required property Viewer viewer
    required property TagEditor tagEditor
    property bool shown: false
    property bool expanded: false
    // The height the footer may take, e.g. the viewer's. The details take up to 40 % of it; while editing,
    // all of it up to the toolbar. More scrolls.
    property real availableHeight: Kirigami.Units.gridUnit * 25
    readonly property real maximumDetailsHeight: tagEditor.editing
        ? availableHeight - headerRow.implicitHeight - Kirigami.Units.smallSpacing * 3
        : availableHeight * 0.4

    // The details after the name, e.g. "14/07/2025 · 4032 × 3024 · 3.1 MB"; what is not known is left out.
    readonly property string summary: {
        const parts = [];
        if (!isNaN(viewer.takenAt.getTime())) {
            parts.push(viewer.takenAt.toLocaleDateString(Qt.locale(), Locale.ShortFormat));
        }
        parts.push(KI18n.i18nc("@info %1 × %2 is a size in pixels", "%1 × %2",
                               viewer.imageDisplaySize.width, viewer.imageDisplaySize.height));
        if (viewer.fileSize > 0) {
            parts.push(Qt.locale().formattedDataSize(viewer.fileSize));
        }
        return parts.join(" · ");
    }

    // 0 hidden, 1 shown; in between while sliding
    property real reveal: shown ? 1 : 0

    // Date and time as the user's locale writes them, e.g. "14 July 2025 18:03"
    function dateAndTime(moment: date): string {
        return KI18n.i18nc("@info %1 is a date, %2 a time", "%1 %2",
                           moment.toLocaleDateString(Qt.locale(), Locale.LongFormat),
                           moment.toLocaleTimeString(Qt.locale(), Locale.ShortFormat));
    }

    function takenAtText(): string {
        switch (viewer.takenAtSource) {
        case Viewer.TakenAtSource.Photo:
            return KI18n.i18nc("@info %1 is a date and time", "%1 (from the photo)", dateAndTime(viewer.takenAt));
        case Viewer.TakenAtSource.FileDate:
            return KI18n.i18nc("@info %1 is a date and time", "%1 (the file's date)", dateAndTime(viewer.takenAt));
        case Viewer.TakenAtSource.ImportTime:
            return KI18n.i18nc("@info %1 is a date and time", "%1 (when it was imported)",
                               dateAndTime(viewer.takenAt));
        }
        return KI18n.i18nc("@info The date a photo was taken", "Unknown");
    }

    implicitHeight: content.implicitHeight + Kirigami.Units.smallSpacing * 2
    visible: reveal > 0
    opacity: reveal
    transform: Translate {
        y: (1 - footer.reveal) * footer.height
    }
    color: Qt.alpha(Kirigami.Theme.backgroundColor, 0.85)

    Behavior on reveal {
        NumberAnimation {
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.OutCubic
        }
    }

    // Never shown: measures a tag chip, so the namespace names line up with their chips.
    Kirigami.Chip {
        id: chipHeight

        visible: false
        text: "x"
    }

    // The image underneath does not react to clicks, drags and the wheel on the footer.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onWheel: wheel => wheel.accepted = true
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.largeSpacing
        anchors.topMargin: Kirigami.Units.smallSpacing
        anchors.bottomMargin: Kirigami.Units.smallSpacing
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            id: headerRow

            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing

            Controls.Label {
                Layout.fillWidth: true
                text: footer.viewer.fileName
                elide: Text.ElideMiddle  // keeps the file's extension in view
                font.bold: true
            }

            Controls.Label {
                text: footer.summary
            }

            Controls.ToolButton {
                visible: footer.expanded && footer.tagEditor.canEdit && !footer.tagEditor.editing
                icon.name: "document-edit"
                text: KI18n.i18nc("@action:button", "Edit Tags")
                onClicked: footer.tagEditor.startEditing()
            }

            Controls.BusyIndicator {
                Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                visible: running
                running: footer.tagEditor.saving
            }

            Controls.Button {
                visible: footer.tagEditor.editing
                icon.name: "dialog-ok-apply"
                text: KI18n.i18nc("@action:button Save the tag changes", "Save")
                enabled: footer.tagEditor.hasChanges && !footer.tagEditor.saving
                onClicked: footer.tagEditor.save()
            }

            Controls.Button {
                visible: footer.tagEditor.editing
                icon.name: "dialog-cancel"
                text: KI18n.i18nc("@action:button Drop the tag changes", "Cancel")
                enabled: !footer.tagEditor.saving
                onClicked: footer.tagEditor.cancel()
            }

            Controls.ToolButton {
                enabled: !footer.tagEditor.editing  // the editing happens in the expanded part
                icon.name: footer.expanded ? "arrow-down" : "arrow-up"
                text: footer.expanded ? KI18n.i18nc("@action:button", "Hide Details")
                                      : KI18n.i18nc("@action:button", "Show Details")
                display: Controls.AbstractButton.IconOnly
                onClicked: footer.expanded = !footer.expanded

                Controls.ToolTip.text: text
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }

        // Made only while expanded
        Loader {
            Layout.fillWidth: true
            active: footer.expanded
            visible: active
            sourceComponent: detailsComponent
        }
    }

    // A line of the details: a name, and its value at the right of it. The names line up.
    component DetailRow: RowLayout {
        id: row

        property string label
        // The height of the value's first line, e.g. of a tag chip, so the name lines up with it
        property real firstLineHeight: 0
        default property alias content: valueArea.data

        spacing: Kirigami.Units.largeSpacing

        Controls.Label {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 7
            Layout.preferredHeight: Math.max(implicitHeight, row.firstLineHeight)
            Layout.alignment: Qt.AlignTop
            horizontalAlignment: Text.AlignRight
            verticalAlignment: Text.AlignVCenter
            text: row.label
            color: Kirigami.Theme.disabledTextColor
            elide: Text.ElideRight
        }

        ColumnLayout {
            id: valueArea

            Layout.fillWidth: true
        }
    }

    Component {
        id: detailsComponent

        Controls.ScrollView {
            id: detailsScroll

            implicitHeight: Math.min(details.implicitHeight, footer.maximumDetailsHeight)
            contentWidth: availableWidth

            ColumnLayout {
                id: details

                width: detailsScroll.availableWidth
                spacing: Kirigami.Units.smallSpacing

                // A library image's tags, one line per namespace
                Repeater {
                    model: footer.tagEditor.editing ? [] : footer.viewer.tagGroups

                    delegate: DetailRow {
                        id: tagGroup

                        required property var modelData

                        Layout.fillWidth: true
                        label: modelData.namespace + ":"  // as written in a tag, e.g. "author:"
                        firstLineHeight: chipHeight.implicitHeight

                        Flow {
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.smallSpacing

                            Repeater {
                                model: tagGroup.modelData.names

                                delegate: Kirigami.Chip {
                                    required property string modelData

                                    text: modelData
                                    closable: false
                                    checkable: false
                                }
                            }
                        }
                    }
                }

                // While editing: a row per namespace, like the server's folder-import form
                Repeater {
                    model: footer.tagEditor.editing ? footer.tagEditor.namespaceRows : null

                    delegate: DetailRow {
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
                        label: namespaceName + ":"
                        firstLineHeight: rowContent.firstLineHeight

                        TagRowContent {
                            id: rowContent

                            Layout.fillWidth: true
                            editor: footer.tagEditor
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

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: footer.tagEditor.editing && footer.tagEditor.errorMessage.length > 0
                    type: Kirigami.MessageType.Error
                    text: footer.tagEditor.errorMessage
                }

                Kirigami.Separator {
                    Layout.fillWidth: true
                    visible: footer.viewer.tagGroups.length > 0 || footer.tagEditor.editing
                }

                DetailRow {
                    Layout.fillWidth: true
                    label: KI18n.i18nc("@label The date a photo was taken", "Taken:")

                    Controls.Label {
                        Layout.fillWidth: true
                        text: footer.takenAtText()
                        wrapMode: Text.Wrap
                    }
                }

                DetailRow {
                    Layout.fillWidth: true
                    visible: footer.viewer.fromLibrary
                    label: KI18n.i18nc("@label The date an image came into the library", "Imported:")

                    Controls.Label {
                        Layout.fillWidth: true
                        text: footer.dateAndTime(footer.viewer.importedAt)
                        wrapMode: Text.Wrap
                    }
                }

                DetailRow {
                    Layout.fillWidth: true
                    label: KI18n.i18nc("@label A file's MIME type", "Type:")

                    Controls.Label {
                        Layout.fillWidth: true
                        text: footer.viewer.mimeType
                    }
                }

                DetailRow {
                    Layout.fillWidth: true
                    label: footer.viewer.fromLibrary
                           ? KI18n.i18nc("@label A path in the server's library", "In the library:")
                           : KI18n.i18nc("@label", "Folder:")

                    // Selectable, so the place can be copied
                    Kirigami.SelectableLabel {
                        Layout.fillWidth: true
                        text: footer.viewer.location
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }
    }
}
