// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the suggestion list below use the field's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The library's search, with suggestions for the tag being typed. Enter searches, unless a suggestion is
// highlighted with the arrow keys: Enter or Tab then takes it; Tab takes the first when none is. A suggestion
// taken only changes the text, the search starts at the next Enter. The list shows while typing: moving the
// cursor, Enter and Escape close it.
Kirigami.SearchField {
    id: field

    required property SearchSuggestions searchSuggestions
    required property string searchedText  // what the library shows now
    readonly property TagSuggestions suggestions: searchSuggestions.suggestions
    // The popup stays visible while it closes, so this says whether it is meant to show.
    readonly property bool suggesting: activeFocus && typing && suggestions.count > 0
    property bool typing: false
    property int typedCursorPosition: -1

    signal searchRequested(string text)

    function followTyping() {
        typing = true;
        typedCursorPosition = cursorPosition;
        searchSuggestions.setTypedSearch(text, cursorPosition);
    }

    function closeIfCursorMoved() {
        if (cursorPosition !== typedCursorPosition) {
            typing = false;
        }
    }

    placeholderText: KI18n.i18nc("@info:placeholder", "Search tags…")
    autoAccept: false  // one search per Enter, not one per key
    onTextEdited: followTyping()
    onCursorPositionChanged: closeIfCursorMoved()  // typing moves it too, and then follows
    onAccepted: {
        typing = false;
        searchRequested(text);
    }
    onTextChanged: {
        if (text.length === 0 && searchedText.length > 0) {
            searchRequested("");  // cleared: all images again
        }
    }

    Keys.onPressed: event => {
        if (!suggesting) {
            return;
        }

        switch (event.key) {
        case Qt.Key_Down:
            suggestionList.currentIndex = Math.min(suggestionList.currentIndex + 1, suggestionList.count - 1);
            break;
        case Qt.Key_Up:
            suggestionList.currentIndex = Math.max(suggestionList.currentIndex - 1, -1);
            break;
        case Qt.Key_Tab:
            searchSuggestions.choose(Math.max(suggestionList.currentIndex, 0));
            break;
        case Qt.Key_Return:
        case Qt.Key_Enter:
            if (suggestionList.currentIndex < 0) {
                return;  // searches
            }
            searchSuggestions.choose(suggestionList.currentIndex);
            break;
        case Qt.Key_Escape:
            typing = false;
            break;
        default:
            return;
        }
        event.accepted = true;
    }

    Controls.ToolTip.text: KI18n.i18nc("@info:tooltip", "For example: category:photo summer -rating:private")
    Controls.ToolTip.visible: hovered && text.length === 0
    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay

    Connections {
        target: field.searchSuggestions

        function onCompleted(text: string, cursorPosition: int) {
            field.typing = false;
            field.text = text;
            field.cursorPosition = cursorPosition;
            field.typedCursorPosition = cursorPosition;
        }
    }

    Controls.Popup {
        id: suggestionPopup

        y: field.height
        width: field.width
        padding: 0
        visible: field.suggesting
        closePolicy: Controls.Popup.NoAutoClose  // shown while there are suggestions
        onVisibleChanged: suggestionList.currentIndex = -1

        contentItem: ListView {
            id: suggestionList

            objectName: "searchSuggestionList"
            implicitHeight: contentHeight
            model: field.suggestions
            currentIndex: -1
            onCountChanged: currentIndex = -1  // new suggestions: nothing chosen yet

            delegate: Controls.ItemDelegate {
                id: suggestion

                required property int index
                required property string tag  // with its namespace: the search may need it
                required property int imageCount

                width: ListView.view.width
                highlighted: ListView.isCurrentItem
                onClicked: field.searchSuggestions.choose(index)

                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing

                    Controls.Label {
                        Layout.fillWidth: true
                        text: suggestion.tag
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
