// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import io.github.oesusdevelopmentstudios.photobook

// A toolbar growing narrower, as the viewer's: first the labels collapse one at a time, then whole groups go to
// the overflow menu, the least important first, the image navigation last.
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
        toolbar.availableWidth = toolbar.neededWidth(0, 0);

        compare(toolbar.collapsedLabels, 0);
        compare(toolbar.hiddenGroups, 0);
        compare(toolbar.displayHint(save), Kirigami.DisplayHint.NoPreference);
        compare(toolbar.displayHint(previous), Kirigami.DisplayHint.IconOnly);  // never with a label
    }

    // Fit's first, then the slideshow's, then Save Rotation's: the least important first. Nothing hidden yet.
    function test_collapsesOneLabelAtATime_data() {
        const label = Kirigami.DisplayHint.NoPreference;
        const icon = Kirigami.DisplayHint.IconOnly;
        return [
            {tag: "fit", collapsed: 1, hints: [icon, icon, icon, icon, label, label, icon]},
            {tag: "slideshow", collapsed: 2, hints: [icon, icon, icon, icon, label, icon, icon]},
            {tag: "save", collapsed: 3, hints: [icon, icon, icon, icon, icon, icon, icon]}
        ];
    }

    function test_collapsesOneLabelAtATime(data) {
        const toolbar = priority();
        toolbar.availableWidth = toolbar.neededWidth(data.collapsed - 1, 0) - 1;

        compare(toolbar.collapsedLabels, data.collapsed);
        compare(toolbar.hiddenGroups, 0);
        compare(hints(toolbar), data.hints);
    }

    function test_hidesTheLeastImportantFirst_data() {
        const icon = Kirigami.DisplayHint.IconOnly;
        const hidden = Kirigami.DisplayHint.AlwaysHide;
        return [
            {tag: "the rest", hidden: 1, hints: [icon, icon, icon, icon, icon, icon, hidden]},
            {tag: "slideshow", hidden: 2, hints: [icon, icon, icon, icon, icon, hidden, hidden]},
            {tag: "save", hidden: 3, hints: [icon, icon, icon, icon, hidden, hidden, hidden]},
            {tag: "rotation, together", hidden: 4, hints: [icon, icon, hidden, hidden, hidden, hidden, hidden]},
            {tag: "navigation, together", hidden: 5, hints: [hidden, hidden, hidden, hidden, hidden, hidden, hidden]}
        ];
    }

    function test_hidesTheLeastImportantFirst(data) {
        const toolbar = priority();
        const allCollapsed = toolbar.collapseOrder.length;
        toolbar.availableWidth = toolbar.neededWidth(allCollapsed, data.hidden - 1) - 1;

        compare(toolbar.collapsedLabels, allCollapsed);
        compare(toolbar.hiddenGroups, data.hidden);
        compare(hints(toolbar), data.hints);
    }

    function test_needsNoRoomForWhatIsNotShown() {
        const toolbar = priority();
        const withSave = toolbar.neededWidth(3, 0);

        save.visible = false;

        compare(toolbar.neededWidth(3, 0), withSave - toolbar.spacing - toolbar.iconButtonWidth);
    }

    // The settings are only ever in the overflow menu; the upload's indicator takes an icon's room.
    function test_countsActionsInNoGroup() {
        const toolbar = priority();
        const withUpload = toolbar.neededWidth(3, 0);
        toolbar.allActions = [previous, next, slideshow, fit, save, rotateLeft, rotateRight, settings, upload];

        compare(toolbar.neededWidth(3, 0), withUpload + toolbar.spacing + toolbar.iconButtonWidth);
    }
}
