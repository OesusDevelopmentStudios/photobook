// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.pictorio

// A tile of the library grid: a click opens the image; with Ctrl and Shift it selects, as in Dolphin.
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

        function clickedTile(modifiers: int): LibraryTile {
            const tile = createTemporaryObject(tileComponent, root);
            for (const spy of [opened, toggled, ranged]) {
                spy.clear();
                spy.target = tile;
            }
            mouseClick(tile, tile.width / 2, tile.height / 2, Qt.LeftButton, modifiers);
            return tile;
        }

        function test_opensOnAClick() {
            clickedTile(Qt.NoModifier);

            compare(opened.count, 1);
            compare(opened.signalArguments[0][0], 4);
            compare(toggled.count + ranged.count, 0);
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
    }
}
