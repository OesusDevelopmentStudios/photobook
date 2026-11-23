// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

// The server has two albums: vacation_2015 (84 images, changed last) and garden (3).
Item {
    id: root

    width: 900
    height: 600

    TestCase {
        id: testCase

        name: "AlbumsView"
        when: windowShown

        readonly property Albums albums: TestServices.albums

        Component {
            id: viewComponent

            AlbumsView {
                anchors.fill: parent
                albums: TestServices.albums
                libraryView: TestServices.libraryView
            }
        }

        Component {
            id: spyComponent

            SignalSpy {
                signalName: "albumOpenRequested"
            }
        }

        function initTestCase() {
            tryCompare(TestServices.accountConnection, "state", Connection.State.LoggedIn);
            tryCompare(albums, "count", 2);
        }

        // The choices are remembered in the settings the tests share.
        function cleanup() {
            albums.filterText = "";
            albums.sort = Albums.Sort.RecentlyChanged;
            albums.includeHidden = false;
            TestServices.libraryView.sidebarVisible = true;
            tryCompare(albums, "loading", false);
        }

        // Shown, with its tiles laid out
        function shownView(): AlbumsView {
            const view = createTemporaryObject(viewComponent, root);
            const grid = findChild(view, "albumGrid");
            tryVerify(() => grid.itemAtIndex(1) !== null);
            return view;
        }

        function tileAt(view: AlbumsView, index: int): AlbumTile {
            return findChild(view, "albumGrid").itemAtIndex(index);
        }

        function test_showsTheAlbumsAsTiles() {
            const view = shownView();

            compare(tileAt(view, 0).name, "vacation 2015");
            compare(tileAt(view, 0).imageCount, 84);
            compare(tileAt(view, 1).name, "garden");
            verify(!findChild(view, "noAlbums").visible);
        }

        function test_sortsByTheChoice() {
            const view = shownView();

            mouseClick(findChild(view, "sortByName"));

            compare(albums.sort, Albums.Sort.Name);
            tryCompare(tileAt(view, 0), "name", "garden");
        }

        function test_filtersByName() {
            const view = shownView();
            const filter = findChild(view, "albumFilter");
            filter.forceActiveFocus();

            keyClick("g");
            keyClick("a");

            compare(albums.filterText, "ga");
            tryCompare(findChild(view, "albumGrid"), "count", 1);
            compare(tileAt(view, 0).name, "garden");
        }

        function test_saysWhenNoAlbumMatches() {
            const view = shownView();

            albums.filterText = "nothing like it";

            const placeholder = findChild(view, "noAlbums");
            tryCompare(placeholder, "visible", true);
            compare(placeholder.text, "No album matches");
        }

        function test_listsAlbumsWithHiddenImagesWhenChecked() {
            const view = shownView();

            mouseClick(findChild(view, "showHiddenAlbums"));

            verify(albums.includeHidden);
        }

        function test_aClickAsksToOpenTheAlbum() {
            const view = shownView();
            const opened = createTemporaryObject(spyComponent, root, {target: view});

            mouseClick(tileAt(view, 1));

            compare(opened.count, 1);
            compare(opened.signalArguments[0][0], 1);
        }

        function test_hidesTheSidebarWithTheLibrarys() {
            const view = shownView();
            verify(findChild(view, "albumFilter").visible);

            TestServices.libraryView.sidebarVisible = false;

            verify(!findChild(view, "albumFilter").visible);
        }
    }
}
