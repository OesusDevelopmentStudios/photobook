// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import io.github.oesusdevelopmentstudios.pictorio

// A toolbar growing narrower, as the viewer's: first the labels collapse, then whole groups go to the overflow
// menu, the least important first, the image navigation last.
TestCase {
    id: testCase

    name: "ToolbarPriority"

    Kirigami.Action {
        id: previous

        text: "Previous"
    }

    Kirigami.Action {
        id: next

        text: "Next"
    }

    Kirigami.Action {
        id: rotateLeft

        text: "Rotate Left"
    }

    Kirigami.Action {
        id: rotateRight

        text: "Rotate Right"
    }

    Kirigami.Action {
        id: save

        text: "Save Rotation"
    }

    Kirigami.Action {
        id: slideshow

        text: "Slideshow"
    }

    Kirigami.Action {
        id: fit

        text: "Fit"
    }

    Kirigami.Action {
        id: upload  // as a running upload's, in no group

        text: "Uploading"
    }

    Kirigami.Action {
        id: settings  // only ever in the overflow menu

        text: "Settings"
        displayHint: Kirigami.DisplayHint.AlwaysHide
    }

    Component {
        id: priorityComponent

        ToolbarPriority {
            groups: [
                [{action: previous, labelled: false}, {action: next, labelled: false}],
                [{action: rotateLeft, labelled: false}, {action: rotateRight, labelled: false}],
                [{action: save, labelled: true}],
                [{action: slideshow, labelled: true}],
                [{action: fit, labelled: true}]
            ]
            allActions: [previous, next, slideshow, fit, save, rotateLeft, rotateRight, settings]
            availableWidth: 10000
            iconButtonWidth: 30
            labelOverhead: 40
            spacing: 4
            font.pixelSize: 12
        }
    }

    function priority(): ToolbarPriority {
        save.visible = true;
        upload.visible = true;
        return createTemporaryObject(priorityComponent, testCase);
    }

    function hints(toolbar: ToolbarPriority): var {
        const actions = [previous, next, rotateLeft, rotateRight, save, slideshow, fit];
        return actions.map(action => toolbar.displayHint(action));
    }

    function test_showsTheLabelsWhileThereIsRoom() {
        const toolbar = priority();
        toolbar.availableWidth = toolbar.neededWidth(0);

        compare(toolbar.level, 0);
        compare(toolbar.displayHint(save), Kirigami.DisplayHint.NoPreference);
        compare(toolbar.displayHint(previous), Kirigami.DisplayHint.IconOnly);  // never with a label
    }

    function test_collapsesTheLabelsBeforeHidingAnything() {
        const toolbar = priority();
        toolbar.availableWidth = toolbar.neededWidth(0) - 1;

        compare(toolbar.level, 1);
        verify(hints(toolbar).every(hint => hint === Kirigami.DisplayHint.IconOnly));
    }

    function test_hidesTheLeastImportantFirst_data() {
        const iconOnly = Kirigami.DisplayHint.IconOnly;
        const hidden = Kirigami.DisplayHint.AlwaysHide;
        return [
            {tag: "the rest", level: 2, hints: [iconOnly, iconOnly, iconOnly, iconOnly, iconOnly, iconOnly, hidden]},
            {tag: "slideshow", level: 3, hints: [iconOnly, iconOnly, iconOnly, iconOnly, iconOnly, hidden, hidden]},
            {tag: "save", level: 4, hints: [iconOnly, iconOnly, iconOnly, iconOnly, hidden, hidden, hidden]},
            {tag: "rotation, together", level: 5, hints: [iconOnly, iconOnly, hidden, hidden, hidden, hidden, hidden]},
            {tag: "navigation, together", level: 6, hints: [hidden, hidden, hidden, hidden, hidden, hidden, hidden]}
        ];
    }

    function test_hidesTheLeastImportantFirst(data) {
        const toolbar = priority();
        toolbar.availableWidth = toolbar.neededWidth(data.level - 1) - 1;

        compare(toolbar.level, data.level);
        compare(hints(toolbar), data.hints);
    }

    function test_needsNoRoomForWhatIsNotShown() {
        const toolbar = priority();
        const withSave = toolbar.neededWidth(1);

        save.visible = false;

        compare(toolbar.neededWidth(1), withSave - toolbar.spacing - toolbar.iconButtonWidth);
    }

    // The settings are only ever in the overflow menu; the upload's indicator takes an icon's room.
    function test_countsActionsInNoGroup() {
        const toolbar = priority();
        const withUpload = toolbar.neededWidth(1);
        toolbar.allActions = [previous, next, slideshow, fit, save, rotateLeft, rotateRight, settings, upload];

        compare(toolbar.neededWidth(1), withUpload + toolbar.spacing + toolbar.iconButtonWidth);
    }
}
