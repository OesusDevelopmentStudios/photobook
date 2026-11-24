// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

// A library photo with tags in author, category, general and meta; the server lists meta before general, and
// applies the tag changes it is sent.
Item {
    id: root

    width: 400
    height: 600

    TestCase {
        id: testCase

        name: "TagPane"
        when: windowShown

        readonly property TagPaneLayout layout: TestServices.tagPaneLayout
        readonly property TagEditor editor: TestServices.paneTagEditor

        Component {
            id: paneComponent

            TagPane {
                width: 300
                height: 600
                tagEditor: TestServices.paneTagEditor
                layout: TestServices.tagPaneLayout
            }
        }

        function initTestCase() {
            tryCompare(TestServices.accountConnection, "state", Connection.State.LoggedIn);
        }

        // The photo as the server has it at first, not being edited
        function init() {
            TestServices.showSamplePhoto();
        }

        // Folding is remembered in the settings the tests share.
        function cleanup() {
            editor.cancel();
            for (const namespaceName of ["category", "author", "general", "meta"]) {
                layout.setFolded(namespaceName, false);
            }
        }

        // Shown, with every section laid out and the counts in. The list gets its height from the pane's layout a
        // moment after it is made and makes its sections again then; a line places its parts once rendered.
        function shownPane(): TagPane {
            const pane = createTemporaryObject(paneComponent, root);
            const sections = findChild(pane, "tagSections");
            tryCompare(sections, "count", 4);
            tryVerify(() => sections.itemAtIndex(sections.count - 1) !== null);
            tryVerify(() => partOf(pane, "count_category:photo")?.visible ?? false);
            waitForRendering(pane);  // the tags' lines are placed then
            return pane;
        }

        // A part of a section, from the sections shown now: sections made before may linger until deleted.
        function partOf(pane: TagPane, name: string): Item {
            const sections = findChild(pane, "tagSections");
            for (let index = 0; index < sections.count; ++index) {
                const section = sections.itemAtIndex(index);
                const part = section ? findChild(section, name) : null;
                if (part) {
                    return part;
                }
            }
            return null;
        }

        function yOf(pane: TagPane, name: string): real {
            return partOf(pane, name).mapToItem(pane, 0, 0).y;
        }

        function test_showsASectionPerNamespaceWithMetaLast() {
            const pane = shownPane();

            compare(partOf(pane, "heading_category").text, "Category");
            verify(yOf(pane, "heading_category") < yOf(pane, "heading_author"));
            verify(yOf(pane, "heading_author") < yOf(pane, "heading_general"));
            verify(yOf(pane, "heading_general") < yOf(pane, "heading_meta"));
        }

        function test_writesNamesWithSpaces() {
            const pane = shownPane();

            compare(partOf(pane, "tag_general:beach_day").text, "beach day");
        }

        function test_showsShortCounts() {
            const pane = shownPane();

            compare(partOf(pane, "count_author:sample_artist").text, "40");
            compare(partOf(pane, "count_general:summer").text, "999");
            compare(partOf(pane, "count_category:photo").text, "1.2k");
            compare(partOf(pane, "count_general:beach_day").text, "46k");
            compare(partOf(pane, "count_meta:jpeg").text, "3.1m");
        }

        function test_shortCount_data() {
            return [
                {tag: "below a thousand", count: 999, shown: "999"},
                {tag: "a thousand", count: 1000, shown: "1.0k"},
                {tag: "rounds up to ten thousand", count: 9999, shown: "10k"},
                {tag: "just below", count: 9949, shown: "9.9k"},
                {tag: "below a million", count: 999499, shown: "999k"},
                {tag: "rounds up to a million", count: 999500, shown: "1.0m"},
                {tag: "millions", count: 12300000, shown: "12m"},
            ];
        }

        function test_shortCount(data) {
            const pane = createTemporaryObject(paneComponent, root);

            compare(pane.shortCount(data.count), data.shown);
        }

        // Built-in namespaces in the theme's colours; the others plain
        function test_coloursTheBuiltInNamespaces() {
            const pane = shownPane();
            const theme = pane.Kirigami.Theme;

            compare(partOf(pane, "tag_author:sample_artist").color, theme.neutralTextColor);
            compare(partOf(pane, "tag_category:photo").color, theme.visitedLinkColor);
            compare(partOf(pane, "tag_meta:jpeg").color, theme.activeTextColor);
            compare(partOf(pane, "tag_general:summer").color, theme.textColor);
        }

        function test_foldsAndUnfoldsASection() {
            const pane = shownPane();

            mouseClick(partOf(pane, "heading_general"));

            verify(layout.isFolded("general"));
            compare(partOf(pane, "heading_general").text, "General (2)");
            tryVerify(() => partOf(pane, "tag_general:summer") === null);
            verify(partOf(pane, "tag_meta:jpeg") !== null, "the other sections stay open");

            mouseClick(partOf(pane, "heading_general"));

            verify(!layout.isFolded("general"));
            tryVerify(() => partOf(pane, "tag_general:summer") !== null);
        }

        // While editing: shown with its controls
        function editingPane(): TagPane {
            const pane = shownPane();
            mouseClick(findChild(pane, "editTags"));
            verify(editor.editing);
            const sections = findChild(pane, "tagSections");
            tryVerify(() => sections.count > 4 && sections.itemAtIndex(sections.count - 1) !== null);
            waitForRendering(pane);
            return pane;
        }

        function type(text: string) {
            for (const character of text) {
                keyClick(character);
            }
        }

        // Every namespace users tag with, also empty ones; meta without controls
        function test_showsEveryNamespaceWhileEditing() {
            const pane = editingPane();

            verify(partOf(pane, "heading_character") !== null, "no tags in it, still a section");
            verify(partOf(pane, "remove_general:summer").visible);
            verify(partOf(pane, "add_general").visible);
            verify(partOf(pane, "tag_meta:jpeg") !== null);
            verify(partOf(pane, "remove_meta:jpeg") === null || !partOf(pane, "remove_meta:jpeg").visible);
            verify(partOf(pane, "add_meta") === null || !partOf(pane, "add_meta").visible);
            verify(!findChild(pane, "editTags").visible);
        }

        function test_hasDropdownsForCategoryAndRating() {
            const pane = editingPane();

            verify(partOf(pane, "valueBox_category").visible);
            verify(partOf(pane, "valueBox_rating").visible);
            verify(partOf(pane, "tag_category:photo") === null, "the dropdown shows the value");
            verify(!partOf(pane, "valueBox_general").visible);
        }

        function test_strikesOutARemovedTagUntilKept() {
            const pane = editingPane();

            mouseClick(partOf(pane, "remove_general:summer"));

            verify(partOf(pane, "tag_general:summer").font.strikeout);
            verify(editor.hasChanges);

            mouseClick(partOf(pane, "remove_general:summer"));  // now "Keep This Tag"

            verify(!partOf(pane, "tag_general:summer").font.strikeout);
            verify(!editor.hasChanges);
        }

        // Enter adds and keeps the field open for the next one.
        function test_addsTagsOneAfterAnother() {
            const pane = editingPane();
            mouseClick(partOf(pane, "add_general"));
            const field = partOf(pane, "addField_general");
            verify(field.visible && field.activeFocus);

            type("sand");
            keyClick(Qt.Key_Return);
            type("sea");
            keyClick(Qt.Key_Return);

            verify(partOf(pane, "tag_general:sand") !== null);
            verify(partOf(pane, "tag_general:sea") !== null);
            verify(field.visible && field.activeFocus && field.text === "");

            keyClick(Qt.Key_Escape);

            verify(!field.visible);
            verify(editor.editing, "Escape closes the field, not the editing");
        }

        // Closed, what is typed is still a change, and saved with the rest.
        function test_keepsTypedTextWhenTheFieldCloses() {
            const pane = editingPane();
            mouseClick(partOf(pane, "add_general"));
            type("dune");

            mouseClick(partOf(pane, "add_author"));  // the focus moves to another section's field

            verify(!partOf(pane, "addField_general").visible);
            verify(partOf(pane, "addField_author").visible);
            verify(editor.hasChanges);

            mouseClick(findChild(pane, "saveTags"));

            tryCompare(editor, "editing", false);
            tryVerify(() => partOf(pane, "tag_general:dune") !== null);
        }

        // The click on a suggestion must not close the field before it counts.
        function test_addsAClickedSuggestion() {
            const pane = editingPane();
            mouseClick(partOf(pane, "add_general"));
            type("s");
            const suggestions = findChild(partOf(pane, "addField_general"), "tagSuggestionList");
            tryVerify(() => suggestions.itemAtIndex(0) !== null);

            mouseClick(suggestions.itemAtIndex(0));

            tryVerify(() => partOf(pane, "tag_general:sample_hero") !== null);
        }

        function test_savesTheChanges() {
            const pane = editingPane();
            mouseClick(partOf(pane, "remove_general:summer"));

            mouseClick(findChild(pane, "saveTags"));

            tryCompare(editor, "editing", false);
            tryVerify(() => partOf(pane, "tag_general:summer") === null);
            verify(findChild(pane, "editTags").visible);
        }

        function test_cancelDropsTheChanges() {
            const pane = editingPane();
            mouseClick(partOf(pane, "remove_general:summer"));

            mouseClick(findChild(pane, "cancelTags"));

            verify(!editor.editing);
            verify(!partOf(pane, "tag_general:summer").font.strikeout);
        }

        // Folding works while editing too; a folded section has no controls showing.
        function test_foldsWhileEditing() {
            const pane = editingPane();

            mouseClick(partOf(pane, "heading_general"));

            verify(!partOf(pane, "add_general").visible);
            tryVerify(() => partOf(pane, "remove_general:summer") === null);
        }

        // As for the next image, or after a restart
        function test_keepsASectionFolded() {
            layout.setFolded("meta", true);

            const pane = shownPane();

            compare(partOf(pane, "heading_meta").text, "Meta (1)");
            verify(partOf(pane, "tag_meta:jpeg") === null);
        }
    }
}
