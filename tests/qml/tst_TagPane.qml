// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

// A library photo with tags in author, category, general and meta; the server lists meta before general.
Item {
    id: root

    width: 400
    height: 600

    TestCase {
        id: testCase

        name: "TagPane"
        when: windowShown

        readonly property TagPaneLayout layout: TestServices.tagPaneLayout

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
            TestServices.showSamplePhoto();
        }

        // Folding is remembered in the settings the tests share.
        function cleanup() {
            for (const namespaceName of ["category", "author", "general", "meta"]) {
                layout.setFolded(namespaceName, false);
            }
        }

        // Shown, with every section laid out and the counts in
        function shownPane(): TagPane {
            const pane = createTemporaryObject(paneComponent, root);
            tryCompare(findChild(pane, "tagSections"), "count", 4);
            tryVerify(() => findChild(pane, "count_category:photo")?.visible ?? false);
            return pane;
        }

        function yOf(pane: TagPane, name: string): real {
            return findChild(pane, name).mapToItem(pane, 0, 0).y;
        }

        function test_showsASectionPerNamespaceWithMetaLast() {
            const pane = shownPane();

            compare(findChild(pane, "heading_category").text, "Category");
            verify(yOf(pane, "heading_category") < yOf(pane, "heading_author"));
            verify(yOf(pane, "heading_author") < yOf(pane, "heading_general"));
            verify(yOf(pane, "heading_general") < yOf(pane, "heading_meta"));
        }

        function test_writesNamesWithSpaces() {
            const pane = shownPane();

            compare(findChild(pane, "tag_general:beach_day").text, "beach day");
        }

        function test_showsShortCounts() {
            const pane = shownPane();

            compare(findChild(pane, "count_author:sample_artist").text, "40");
            compare(findChild(pane, "count_general:summer").text, "999");
            compare(findChild(pane, "count_category:photo").text, "1.2k");
            compare(findChild(pane, "count_general:beach_day").text, "46k");
            compare(findChild(pane, "count_meta:jpeg").text, "3.1m");
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

            compare(findChild(pane, "tag_author:sample_artist").color, theme.neutralTextColor);
            compare(findChild(pane, "tag_category:photo").color, theme.visitedLinkColor);
            compare(findChild(pane, "tag_meta:jpeg").color, theme.activeTextColor);
            compare(findChild(pane, "tag_general:summer").color, theme.textColor);
        }

        function test_foldsAndUnfoldsASection() {
            const pane = shownPane();

            mouseClick(findChild(pane, "heading_general"));

            verify(layout.isFolded("general"));
            compare(findChild(pane, "heading_general").text, "General (2)");
            tryVerify(() => findChild(pane, "tag_general:summer") === null);
            verify(findChild(pane, "tag_meta:jpeg") !== null, "the other sections stay open");

            mouseClick(findChild(pane, "heading_general"));

            verify(!layout.isFolded("general"));
            tryVerify(() => findChild(pane, "tag_general:summer") !== null);
        }

        // As for the next image, or after a restart
        function test_keepsASectionFolded() {
            layout.setFolded("meta", true);

            const pane = shownPane();

            compare(findChild(pane, "heading_meta").text, "Meta (1)");
            verify(findChild(pane, "tag_meta:jpeg") === null);
        }
    }
}
