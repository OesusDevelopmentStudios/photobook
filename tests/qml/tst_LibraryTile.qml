// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.pictorio

// A tile of the library grid: a click opens the image, or selects it while selecting; with Ctrl and Shift it
// selects, as in Dolphin; a right-click asks for its menu.
// The TestCase is invisible; what it tests is created in the visible root, where it can be clicked.
Item {
    id: root

    width: 300
    height: 300

    TestCase {
        id: testCase

        name: "LibraryTile"
        when: windowShown

        Component {
            id: tileComponent

            LibraryTile {
                width: 200
                height: 200
                index: 4
                thumbnailUrl: ""
                largeThumbnailUrl: ""
                fileName: "beach.jpg"
                takenAt: new Date(2025, 6, 14)
                imageWidth: 400
                imageHeight: 300
                selected: false
            }
        }

        SignalSpy {
            id: opened

            signalName: "openRequested"
        }

        SignalSpy {
            id: toggled

            signalName: "selectionToggled"
        }

        SignalSpy {
            id: ranged

            signalName: "rangeSelected"
        }

        SignalSpy {
            id: menuAsked

            signalName: "menuRequested"
        }

        function clickedTile(modifiers: int, button = Qt.LeftButton, selecting = false): LibraryTile {
            const tile = createTemporaryObject(tileComponent, root, {selecting: selecting});
            for (const spy of [opened, toggled, ranged, menuAsked]) {
                spy.clear();
                spy.target = tile;
            }
            mouseClick(tile, tile.width / 2, tile.height / 2, button, modifiers);
            return tile;
        }

        function test_opensOnAClick() {
            clickedTile(Qt.NoModifier);

            compare(opened.count, 1);
            compare(opened.signalArguments[0][0], 4);
            compare(toggled.count + ranged.count, 0);
        }

        // Choosing images for their tags: no Ctrl needed
        function test_togglesOnAClickWhileSelecting() {
            clickedTile(Qt.NoModifier, Qt.LeftButton, true);

            compare(toggled.count, 1);
            compare(toggled.signalArguments[0][0], 4);
            compare(opened.count + ranged.count, 0);
        }

        function test_togglesWithCtrl() {
            clickedTile(Qt.ControlModifier);

            compare(toggled.count, 1);
            compare(opened.count + ranged.count, 0);
        }

        function test_selectsARangeWithShift() {
            clickedTile(Qt.ShiftModifier);

            compare(ranged.count, 1);
            compare(ranged.signalArguments[0][0], 4);
            compare(ranged.signalArguments[0][1], false);
            compare(opened.count + toggled.count, 0);
        }

        function test_addsARangeWithCtrlShift() {
            clickedTile(Qt.ControlModifier | Qt.ShiftModifier);

            compare(ranged.count, 1);
            compare(ranged.signalArguments[0][0], 4);
            compare(ranged.signalArguments[0][1], true);
        }

        function test_asksForItsMenuOnARightClick() {
            clickedTile(Qt.NoModifier, Qt.RightButton);

            compare(menuAsked.count, 1);
            compare(menuAsked.signalArguments[0][0], 4);
            compare(opened.count + toggled.count + ranged.count, 0);
        }
    }
}
