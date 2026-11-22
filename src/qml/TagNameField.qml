// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the suggestion list below use the field's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// Enter adds the highlighted suggestion, or what was typed. What is typed counts as a change until it is added.
// Without a namespace (the hidden filters) suggestions come from every namespace and are added as whole tags,
// e.g. "rating:private".
Controls.TextField {
    id: field

    required property TagRowsEditor editor
    required property string namespaceName
    readonly property TagSuggestions suggestions: editor.suggestions
    readonly property bool wholeTags: namespaceName === ""

    placeholderText: KI18n.i18nc("@info:placeholder", "Add a tag…")
    enabled: !editor.busy
    onTextChanged: editor.setTypedName(namespaceName, text)
    onAccepted: {
        const row = suggestionList.currentIndex;
        const chosen = row < 0 ? text : wholeTags ? suggestions.tagAt(row) : suggestions.nameAt(row);
        editor.addTag(namespaceName, chosen);  // once added, the editor empties the field
    }

    Keys.onDownPressed: suggestionList.currentIndex = Math.min(suggestionList.currentIndex + 1,
                                                               suggestionList.count - 1)
    Keys.onUpPressed: suggestionList.currentIndex = Math.max(suggestionList.currentIndex - 1, -1)

    // The editor may change the text itself: emptied once added, or when the changes are dropped.
    Connections {
        target: field.editor

        function onTypedNamesChanged() {
            const typed = field.editor.typedName(field.namespaceName);
            if (field.text !== typed) {
                field.text = typed;
            }
        }
    }

    Controls.Popup {
        y: -height  // above: the field may be at the bottom of the window
        width: field.width
        padding: 0
        visible: field.activeFocus && field.suggestions.namespaceName === field.namespaceName
                 && field.suggestions.count > 0
        closePolicy: Controls.Popup.NoAutoClose  // shown while there are suggestions

        contentItem: ListView {
            id: suggestionList

            implicitHeight: contentHeight
            model: field.suggestions
            currentIndex: -1
            onCountChanged: currentIndex = -1  // new suggestions: nothing chosen yet

            delegate: Controls.ItemDelegate {
                id: suggestion

                required property string tag
                required property string name
                readonly property string shown: field.wholeTags ? tag : name
                required property int imageCount

                width: ListView.view.width
                highlighted: ListView.isCurrentItem
                onClicked: field.editor.addTag(field.namespaceName, shown)

                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing

                    Controls.Label {
                        Layout.fillWidth: true
                        text: suggestion.shown
                        elide: Text.ElideRight
                    }

                    Controls.Label {
                        text: suggestion.imageCount  // images with the tag
                        color: Kirigami.Theme.disabledTextColor
                    }
                }
            }
        }
    }
}
