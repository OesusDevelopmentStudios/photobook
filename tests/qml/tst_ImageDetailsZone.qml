// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

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

        // A library photo, so the footer has the tag pane's button
        Component {
            id: libraryZoneComponent

            ImageDetailsZone {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                viewer: TestServices.viewer
                tagEditor: TestServices.paneTagEditor
                tagPaneLayout: TestServices.tagPaneLayout
                availableHeight: root.height
            }
        }

        function initTestCase() {
            tryCompare(TestServices.accountConnection, "state", Connection.State.LoggedIn);
            TestServices.showSamplePhoto();
        }

        function cleanup() {
            TestServices.tagPaneLayout.open = true;
        }

        function shownZone(component = zoneComponent): ImageDetailsZone {
            const zone = createTemporaryObject(component, root);
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

        function test_showsAndHidesTheTagPane() {
            const zone = shownZone(libraryZoneComponent);
            const toggle = findChild(zone, "tagPaneToggle");
            verify(toggle.visible && toggle.checked);

            mouseClick(toggle);

            verify(!TestServices.tagPaneLayout.open);
            verify(!toggle.checked);

            mouseClick(toggle);

            verify(TestServices.tagPaneLayout.open);
        }

        // While the tag pane is open, the details stay without the pointer near.
        function test_staysShownWhilePinned() {
            const zone = createTemporaryObject(libraryZoneComponent, root, {pinned: true});
            mouseMove(root, root.width / 2, 10);  // over the image

            tryCompare(zone.footer, "reveal", 1);
            wait(100);
            verify(zone.footer.shown);

            zone.pinned = false;

            tryCompare(zone.footer, "visible", false);
        }

        // The tags are edited in the pane, so it stays open meanwhile.
        function test_keepsThePaneOpenWhileEditing() {
            const zone = shownZone(libraryZoneComponent);
            TestServices.paneTagEditor.startEditing();

            verify(!findChild(zone, "tagPaneToggle").enabled);

            TestServices.paneTagEditor.cancel();

            verify(findChild(zone, "tagPaneToggle").enabled);
        }

        // A local file has no tags.
        function test_hasNoTagPaneButtonWithoutALibraryImage() {
            const zone = shownZone();

            verify(!findChild(zone, "tagPaneToggle").visible);
        }

        function test_hidesWhenThePointerLeaves() {
            const zone = shownZone();

            mouseMove(root, root.width / 2, 10);

            tryCompare(zone.footer, "visible", false);
        }
    }
}
