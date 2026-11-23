// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

// A page 900 wide whose header leaves the tabs the room from 60 to 760, off the page's middle.
Item {
    id: root

    width: 900
    height: 200

    TestCase {
        id: testCase

        name: "LibraryTabs"
        when: windowShown

        Component {
            id: tabsComponent

            LibraryTabs {
                x: 60
                width: 700
                height: 40
                libraryView: TestServices.libraryView
                centreOf: root
            }
        }

        function cleanup() {
            TestServices.libraryView.tab = LibraryView.Tab.Library;
        }

        function test_switchesTheTab() {
            const tabs = createTemporaryObject(tabsComponent, root);
            const library = findChild(tabs, "libraryTab");
            const albums = findChild(tabs, "albumsTab");
            verify(library.current && !albums.current);

            mouseClick(albums);

            compare(TestServices.libraryView.tab, LibraryView.Tab.Albums);
            verify(albums.current && !library.current);
        }

        // In the middle of the page, as the web admin's, not of the room the header leaves
        function test_sitsInTheMiddleOfThePage() {
            const tabs = createTemporaryObject(tabsComponent, root);
            const library = findChild(tabs, "libraryTab");
            const albums = findChild(tabs, "albumsTab");

            const left = library.mapToItem(root, 0, 0).x;
            const right = albums.mapToItem(root, albums.width, 0).x;
            fuzzyCompare((left + right) / 2, root.width / 2, 1);
        }

        // Without room for that, they stay inside the header's room.
        function test_staysInItsRoom() {
            const tabs = createTemporaryObject(tabsComponent, root, {x: 600, width: 250});
            const albums = findChild(tabs, "albumsTab");

            tryVerify(() => albums.mapToItem(root, albums.width, 0).x <= 850 + 0.5);
            verify(findChild(tabs, "libraryTab").mapToItem(root, 0, 0).x >= 600 - 0.5);
        }
    }
}
