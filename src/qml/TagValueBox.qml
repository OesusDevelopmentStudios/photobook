// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The dropdown of category or rating: "–" for none, then the values; for many images "Keep as is" first, shown
// as the value they all share if they do. The ratings are fixed (an admin sets them on the server); category also takes a new value typed in, confirmed
// with Enter. A value typed but not confirmed counts as a change. A changed value is bold.
Controls.ComboBox {
    id: box

    required property TagRowsEditor editor
    required property string namespaceName
    required property var options
    required property string value
    required property bool changed
    required property bool fixedValues
    required property bool keepable

    readonly property string keepText: KI18n.i18nc("@item:inlistbox Leave the tag of each image as it is",
                                                   "Keep as is")
    readonly property string noValue: "–"
    readonly property var fixedEntries: keepable ? [keepText, noValue] : [noValue]

    // The editor's value, also after it refused a choice
    function showValue() {
        if (keepable && !changed && value.length === 0) {
            currentIndex = 0;
        } else if (value.length === 0) {
            currentIndex = fixedEntries.length - 1;
        } else {
            currentIndex = fixedEntries.length + options.indexOf(value);
        }
    }

    function choose(name: string) {
        if (keepable && name === keepText) {
            editor.keepValue(namespaceName);
        } else {
            editor.setSingleValue(namespaceName, name === noValue ? "" : name);
        }
        showValue();
    }

    model: fixedEntries.concat(options)
    editable: !fixedValues
    enabled: !editor.busy
    font.bold: changed
    onActivated: index => choose(model[index])
    onAccepted: choose(editText.trim())
    onEditTextChanged: {
        if (editable) {
            editor.setTypedName(namespaceName, editText === currentText ? "" : editText);
        }
    }
    onValueChanged: showValue()
    onChangedChanged: showValue()
    onOptionsChanged: Qt.callLater(showValue)  // after the new model is in
    Component.onCompleted: showValue()

    Controls.ToolTip.text: KI18n.i18nc("@info:tooltip", "Pick one, or type a new one and press Enter")
    Controls.ToolTip.visible: editable && hovered
    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
}
