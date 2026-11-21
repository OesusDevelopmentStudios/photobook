// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

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

        function chipFor(change: int): TagChip {
            const tag = {tag: "general:beach", name: "beach", change: change};
            return createTemporaryObject(chipComponent, root, {modelData: tag});
        }

        function clickRemove(chip: TagChip) {
            const button = chip.indicator;
            verify(button.visible, "the chip has its ✕");
            mouseClick(button);
        }

        // Once failed: a property named "removed" hid the Chip's removed() signal, and ✕ did nothing.
        function test_removesAKeptTag() {
            const chip = chipFor(TagRowsEditor.Change.Kept);

            clickRemove(chip);

            compare(chip.editor.calls, ["removeTag general:beach"]);
        }

        function test_dropsAnAddedTag() {
            const chip = chipFor(TagRowsEditor.Change.Added);
            verify(chip.isAdded);

            clickRemove(chip);

            compare(chip.editor.calls, ["removeTag general:beach"]);
        }

        function test_keepsARemovedTagWhenClicked() {
            const chip = chipFor(TagRowsEditor.Change.Removed);
            verify(!chip.indicator.visible, "no ✕ on a removed tag");

            mouseClick(chip);

            compare(chip.editor.calls, ["restoreTag general:beach"]);
        }
    }
}
