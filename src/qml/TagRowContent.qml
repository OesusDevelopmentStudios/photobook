// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the chips below use the content's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Without the namespace's name. The same for one image and for many.
ColumnLayout {
    id: content

    required property TagRowsEditor editor
    required property string namespaceName
    required property bool dropdown
    required property bool fixedValues
    required property bool keepable
    required property var options
    required property string value
    required property bool changed
    required property var tags

    // So the row's name can line up with the first line
    readonly property real firstLineHeight: dropdown ? valueBox.implicitHeight
                                                     : chips.count > 0 ? chips.itemAt(0).implicitHeight
                                                                       : addField.implicitHeight

    spacing: Kirigami.Units.smallSpacing

    TagValueBox {
        id: valueBox

        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 12
        visible: content.dropdown
        editor: content.editor
        namespaceName: content.namespaceName
        options: content.options
        value: content.value
        changed: content.changed
        fixedValues: content.fixedValues
        keepable: content.keepable
    }

    // What the changes will do to each tag
    Flow {
        Layout.fillWidth: true
        visible: chips.count > 0
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            id: chips

            model: content.dropdown ? [] : content.tags

            delegate: TagChip {
                editor: content.editor
            }
        }
    }

    TagNameField {
        id: addField

        Layout.fillWidth: true
        visible: !content.dropdown
        editor: content.editor
        namespaceName: content.namespaceName
    }
}
