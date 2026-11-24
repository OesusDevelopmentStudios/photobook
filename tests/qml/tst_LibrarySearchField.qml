// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

// The server suggests "character:sample_hero" (3 images) for whatever is typed.
Item {
    id: root

    width: 400
    height: 400

    TestCase {
        id: testCase

        name: "LibrarySearchField"
        when: windowShown

        Component {
            id: fieldComponent

            LibrarySearchField {
                width: 300
                searchSuggestions: TestServices.searchSuggestions
                searchedText: ""
            }
        }

        Component {
            id: spyComponent

            SignalSpy {
                signalName: "searchRequested"
            }
        }

        function initTestCase() {
            tryCompare(TestServices.accountConnection, "state", Connection.State.LoggedIn);
        }

        // A field with focus, and a spy on the searches it asks for
        function focusedField(): list<QtObject> {
            const field = createTemporaryObject(fieldComponent, root);
            const searches = createTemporaryObject(spyComponent, root, {target: field});
            field.forceActiveFocus();
            return [field, searches];
        }

        function type(text: string) {
            for (const character of text) {
                keyClick(character);
            }
        }

        // Typed, and the suggestion shown
        function typeWithSuggestion(field: LibrarySearchField, text: string) {
            type(text);
            compare(field.text, text);
            tryVerify(() => field.suggesting && field.suggestions.tagAt(0) === "character:sample_hero");
        }

        function test_suggestsForTheTagBeingTyped() {
            const [field] = focusedField();

            typeWithSuggestion(field, "category:photo her");
        }

        function test_enterSearchesWhatIsTyped() {
            const [field, searches] = focusedField();
            typeWithSuggestion(field, "her");

            keyClick(Qt.Key_Return);

            compare(searches.count, 1);
            compare(searches.signalArguments[0][0], "her");
            verify(!field.suggesting);
        }

        function test_tabTakesTheFirstSuggestion() {
            const [field, searches] = focusedField();
            typeWithSuggestion(field, "category:photo her");

            keyClick(Qt.Key_Tab);

            compare(field.text, "category:photo character:sample_hero ");
            compare(field.cursorPosition, field.text.length);
            verify(field.activeFocus, "the focus stays for the next term");
            verify(!field.suggesting);
            compare(searches.count, 0, "taken, not searched yet");
        }

        function test_enterTakesTheHighlightedOneThenSearches() {
            const [field, searches] = focusedField();
            typeWithSuggestion(field, "her");

            keyClick(Qt.Key_Down);
            keyClick(Qt.Key_Return);

            compare(field.text, "character:sample_hero ");
            compare(searches.count, 0);

            keyClick(Qt.Key_Return);

            compare(searches.count, 1);
            compare(searches.signalArguments[0][0], "character:sample_hero ");
        }

        function test_aClickTakesASuggestion() {
            const [field] = focusedField();
            typeWithSuggestion(field, "her");
            const list = findChild(field, "searchSuggestionList");
            tryVerify(() => list.itemAtIndex(0) !== null);

            mouseClick(list.itemAtIndex(0));

            compare(field.text, "character:sample_hero ");
        }

        function test_escapeClosesTheListUntilTypingGoesOn() {
            const [field] = focusedField();
            typeWithSuggestion(field, "her");

            keyClick(Qt.Key_Escape);

            verify(!field.suggesting);
            compare(field.text, "her");

            type("o");

            tryCompare(field, "suggesting", true);
        }

        // e.g. a tag clicked in the viewer's tag pane
        function test_showsASearchStartedElsewhere() {
            const [field, searches] = focusedField();

            field.searchedText = "author:sample_artist";

            compare(field.text, "author:sample_artist");
            verify(!field.suggesting);
            compare(searches.count, 0, "already searched");
        }

        function test_movingTheCursorClosesTheList() {
            const [field] = focusedField();
            typeWithSuggestion(field, "her");

            keyClick(Qt.Key_Left);

            tryCompare(field, "suggesting", false);
        }
    }
}
