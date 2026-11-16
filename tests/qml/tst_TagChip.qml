// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.pictorio
import pictorio.tests

// A tag while editing: what its ✕ and a click ask the editor to do, and what it shows.
// The TestCase is invisible; what it tests is created in the visible root, where it can be clicked.
Item {
    id: root

    width: 400
    height: 100

    TestCase {
        id: testCase

        name: "TagChip"
        when: windowShown

        Component {
            id: chipComponent

            TagChip {
                editor: FakeTagRowsEditor {}
            }
        }

        function chipFor(change: int, count: int): TagChip {
            const tag = {tag: "general:beach", name: "beach", change: change};
            if (count > 0) {
                tag.count = count;
            }
            return createTemporaryObject(chipComponent, root, {modelData: tag});
        }

        function clickRemove(chip: TagChip) {
            const button = chip.indicator;
            verify(button.visible, "the chip has its ✕");
            mouseClick(button);
        }

        // Once failed: a property named "removed" hid the Chip's removed() signal, and ✕ did nothing.
        function test_removesAKeptTag() {
            const chip = chipFor(TagRowsEditor.Change.Kept, 0);

            clickRemove(chip);

            compare(chip.editor.calls, ["removeTag general:beach"]);
        }

        function test_dropsAnAddedTag() {
            const chip = chipFor(TagRowsEditor.Change.Added, 0);
            verify(chip.isAdded);

            clickRemove(chip);

            compare(chip.editor.calls, ["removeTag general:beach"]);
        }

        function test_keepsARemovedTagWhenClicked() {
            const chip = chipFor(TagRowsEditor.Change.Removed, 0);
            verify(!chip.indicator.visible, "no ✕ on a removed tag");

            mouseClick(chip);

            compare(chip.editor.calls, ["restoreTag general:beach"]);
        }

        // For many images: how many have the tag
        function test_showsTheCount() {
            compare(chipFor(TagRowsEditor.Change.Kept, 7).text, "beach (7)");
            compare(chipFor(TagRowsEditor.Change.Kept, 0).text, "beach");
        }
    }
}
