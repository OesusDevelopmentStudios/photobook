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
// remembered for every image.
Rectangle {
    id: pane

    required property TagEditor tagEditor
    required property TagPaneLayout layout

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

    ListView {
        id: sections

        objectName: "tagSections"
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.smallSpacing  // room for the scroll bar
        visible: !pane.tagEditor.editing
        model: pane.tagEditor.editing ? null : pane.tagEditor.namespaceRows
        clip: true
        spacing: Kirigami.Units.smallSpacing

        Controls.ScrollBar.vertical: Controls.ScrollBar {}

        delegate: ColumnLayout {
            id: section

            required property string namespaceName
            required property var tags
            readonly property bool folded: pane.layout.foldedNamespaces.includes(namespaceName)

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

            Repeater {
                model: section.folded ? [] : section.tags

                // The count right after the name, as on boorus; a long name gives way to it.
                delegate: Row {
                    id: tagLine

                    required property var modelData
                    readonly property real indent: Kirigami.Units.iconSizes.small + Kirigami.Units.smallSpacing

                    Layout.leftMargin: indent  // under the heading's text
                    // The name's line; the count's baseline anchor would make the row taller.
                    Layout.preferredHeight: name.implicitHeight
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Label {
                        id: name

                        objectName: "tag_" + tagLine.modelData.tag
                        width: Math.min(implicitWidth,
                                        section.width - tagLine.indent - count.implicitWidth - tagLine.spacing)
                        text: tagLine.modelData.name.replace(/_/g, " ")
                        color: pane.tagColor(section.namespaceName)
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
                }
            }
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 2
        visible: !pane.tagEditor.editing && sections.count === 0
        icon.name: "tag"
        text: KI18n.i18nc("@info:placeholder", "No tags")
    }

    // Until the pane has its own editing, the tags are edited in the details at the bottom.
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 2
        visible: pane.tagEditor.editing
        icon.name: "document-edit"
        text: KI18n.i18nc("@info:placeholder", "Editing the tags below")
    }
}
