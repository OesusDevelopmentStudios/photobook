// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.pictorio
import pictorio.tests

// The settings page's hidden filters: the saved ones as chips, removing and adding filters, Save and Cancel.
// The server has "rating:nsfw" and "category:refs" saved and keeps what is saved; the page loads them on opening.
// The TestCase is invisible; what it tests is created in the visible root, where it can be clicked.
Item {
    id: root

    width: 800
    height: 700

    TestCase {
        id: testCase

        name: "SettingsPage"
        when: windowShown

        readonly property HiddenFilterEditor editor: TestServices.hiddenFilterEditor

        Component {
            id: pageComponent

            SettingsPage {
                anchors.fill: parent
                connection: TestServices.accountConnection
                hiddenFilterEditor: TestServices.hiddenFilterEditor
            }
        }

        function initTestCase() {
            tryCompare(TestServices.accountConnection, "state", Connection.State.LoggedIn);
        }

        // Opened, with the saved filters shown. The chips are placed side by side only after they appear; until
        // then a click on the first would reach the second, on top of it.
        function shownPage(): SettingsPage {
            const page = createTemporaryObject(pageComponent, root);
            tryCompare(editor, "loading", false);
            tryCompare(findChild(page, "filterChips"), "count", 2);
            tryVerify(() => chipAt(page, 1).x > chipAt(page, 0).x);
            return page;
        }

        function chipAt(page: SettingsPage, index: int): TagChip {
            return findChild(page, "filterChips").itemAt(index);
        }

        function shownFilters(page: SettingsPage): list<string> {
            const chips = findChild(page, "filterChips");
            const shown = [];
            for (let index = 0; index < chips.count; ++index) {
                const chip = chips.itemAt(index);
                shown.push((chip.isAdded ? "+" : chip.isRemoved ? "-" : "") + chip.text);
            }
            return shown;
        }

        function typeFilter(page: SettingsPage, text: string) {
            const field = findChild(page, "addFilter");
            field.forceActiveFocus();
            field.text = text;
            keyClick(Qt.Key_Return);
        }

        function test_showsTheSavedFilters() {
            const page = shownPage();

            compare(shownFilters(page), ["rating:nsfw", "category:refs"]);
            verify(!findChild(page, "saveFilters").enabled, "nothing to save");
            verify(!findChild(page, "noFilters").visible);
        }

        function test_marksARemovedFilterUntilCancel() {
            const page = shownPage();

            mouseClick(chipAt(page, 0).indicator);

            compare(shownFilters(page), ["-rating:nsfw", "category:refs"]);
            verify(findChild(page, "saveFilters").enabled);

            mouseClick(findChild(page, "cancelFilters"));

            compare(shownFilters(page), ["rating:nsfw", "category:refs"]);
            verify(!findChild(page, "saveFilters").enabled);
        }

        function test_addsATypedFilter() {
            const page = shownPage();

            typeFilter(page, "character:*");

            compare(shownFilters(page), ["rating:nsfw", "category:refs", "+character:*"]);
            compare(findChild(page, "addFilter").text, "", "the field empties once added");
        }

        // Unlike a namespace's row, the field adds a suggestion with its namespace.
        function test_addsASuggestionAsAWholeTag() {
            const page = shownPage();
            const field = findChild(page, "addFilter");
            field.forceActiveFocus();
            field.text = "sample";
            tryCompare(editor.suggestions, "count", 1);

            keyClick(Qt.Key_Down);
            keyClick(Qt.Key_Return);

            compare(shownFilters(page), ["rating:nsfw", "category:refs", "+character:sample_hero"]);
        }

        function test_saysWhyAFilterIsRefused() {
            const page = shownPage();

            typeFilter(page, "-rating:nsfw");

            verify(findChild(page, "errorMessage").visible);
            compare(shownFilters(page), ["rating:nsfw", "category:refs"]);
            compare(findChild(page, "addFilter").text, "-rating:nsfw", "kept, to correct");
        }

        function test_savesTheChanges() {
            const page = shownPage();
            mouseClick(chipAt(page, 1).indicator);
            typeFilter(page, "character:*");

            mouseClick(findChild(page, "saveFilters"));

            tryCompare(editor, "saving", false);
            compare(shownFilters(page), ["rating:nsfw", "character:*"]);
            verify(!findChild(page, "saveFilters").enabled, "nothing left to save");
        }

        function test_showsWhenNoFiltersAreHidden() {
            const page = shownPage();
            mouseClick(chipAt(page, 0).indicator);
            mouseClick(chipAt(page, 1).indicator);
            mouseClick(findChild(page, "saveFilters"));

            tryCompare(findChild(page, "noFilters"), "visible", true);
        }
    }
}
