// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.pictorio
import pictorio.tests

Item {
    id: root

    width: 800
    height: 400

    TestCase {
        id: testCase

        name: "ImageDetailsZone"
        when: windowShown

        Component {
            id: zoneComponent

            ImageDetailsZone {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                viewer: TestServices.viewer
                tagEditor: TestServices.tagEditor
                availableHeight: root.height
            }
        }

        function shownZone(): ImageDetailsZone {
            const zone = createTemporaryObject(zoneComponent, root);
            mouseMove(root, root.width / 2, 10);  // over the image
            verify(!zone.footer.shown);
            mouseMove(root, root.width / 2, zone.y + 5);  // in the zone, above the footer
            tryCompare(zone.footer, "reveal", 1);
            return zone;
        }

        // The footer once flickered here, when it lay on top of the zone as a sibling. With today's footer that
        // arrangement no longer shows it (the footer's buttons change how Qt hands out the hover), so this
        // checks the behaviour, not that one cause.
        function test_staysWhileThePointerIsOnTheFooter() {
            const zone = shownZone();

            mouseMove(root, root.width / 2, root.height - 5);  // on the footer
            let hidden = 0;
            for (let sample = 0; sample < 30; ++sample) {
                wait(20);
                if (!zone.footer.shown) {
                    ++hidden;
                }
            }

            compare(hidden, 0, "the footer hid while the pointer was on it");
        }

        function test_hidesWhenThePointerLeaves() {
            const zone = shownZone();

            mouseMove(root, root.width / 2, 10);

            tryCompare(zone.footer, "visible", false);
        }
    }
}
