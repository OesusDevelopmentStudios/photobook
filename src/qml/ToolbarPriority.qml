// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import org.kde.kirigami as Kirigami

// How a page's toolbar actions show when the toolbar is too narrow for all of them with their labels: first the
// labels collapse to icons, one at a time, the least important first; only then do whole groups move to the
// overflow menu, the least important first. Kirigami alone would move the rightmost actions first, whatever they
// are, and keep the labels. Each action takes its display hint from displayHint(action).
QtObject {
    id: priority

    // Most important first. Each group is a list of {action, labelled}; its actions come and go together.
    // labelled: shown with its text while there is room for it.
    required property var groups
    required property real availableWidth  // the toolbar's, with room for its overflow menu button
    required property real iconButtonWidth
    required property real labelOverhead  // what a button with a label needs besides the text
    required property font font
    property real spacing: Kirigami.Units.smallSpacing
    // All of the toolbar's actions: those in no group, e.g. a running upload's, count as icons
    property list<QtObject> allActions

    // The labelled actions in the order their labels collapse: the least important group first, and in a group
    // the rightmost first
    readonly property var collapseOrder: {
        const order = [];
        for (let group = groups.length - 1; group >= 0; --group) {
            for (let index = groups[group].length - 1; index >= 0; --index) {
                if (groups[group][index].labelled) {
                    order.push(groups[group][index].action);
                }
            }
        }
        return order;
    }

    // The first arrangement that fits: one label more collapsed at a time, then one group more hidden
    readonly property var arrangement: {
        for (let collapsed = 0; collapsed <= collapseOrder.length; ++collapsed) {
            if (neededWidth(collapsed, 0) <= availableWidth) {
                return {collapsedLabels: collapsed, hiddenGroups: 0};
            }
        }
        for (let hidden = 1; hidden < groups.length; ++hidden) {
            if (neededWidth(collapseOrder.length, hidden) <= availableWidth) {
                return {collapsedLabels: collapseOrder.length, hiddenGroups: hidden};
            }
        }
        return {collapsedLabels: collapseOrder.length, hiddenGroups: groups.length};
    }
    readonly property int collapsedLabels: arrangement.collapsedLabels
    readonly property int hiddenGroups: arrangement.hiddenGroups  // counted from the least important

    readonly property FontMetrics metrics: FontMetrics {
        font: priority.font
    }

    function isHidden(group: int, hidden: int): bool {
        return group >= groups.length - hidden;
    }

    function showsLabel(action: QtObject, collapsed: int): bool {
        const position = collapseOrder.indexOf(action);
        return position >= collapsed;  // -1: never labelled
    }

    function inGroup(action: QtObject): bool {
        return groups.some(members => members.some(member => member.action === action));
    }

    // The overflow menu's button is always there: some actions are only ever in the menu.
    function neededWidth(collapsed: int, hidden: int): real {
        let width = iconButtonWidth;
        for (const action of allActions) {
            const kirigamiAction = action as Kirigami.Action;
            if (kirigamiAction?.visible && !inGroup(action)
                    && !(kirigamiAction.displayHint & Kirigami.DisplayHint.AlwaysHide)) {
                width += spacing + iconButtonWidth;
            }
        }
        for (let group = 0; group < groups.length; ++group) {
            if (isHidden(group, hidden)) {
                continue;
            }
            for (const member of groups[group]) {
                if (member.action.visible) {
                    width += spacing + (showsLabel(member.action, collapsed)
                                        ? labelOverhead + metrics.advanceWidth(member.action.text)
                                        : iconButtonWidth);
                }
            }
        }
        return width;
    }

    function displayHint(action: Kirigami.Action): int {
        const group = groups.findIndex(members => members.some(member => member.action === action));
        if (group < 0) {
            return Kirigami.DisplayHint.NoPreference;
        }
        if (isHidden(group, hiddenGroups)) {
            return Kirigami.DisplayHint.AlwaysHide;
        }
        return showsLabel(action, collapsedLabels) ? Kirigami.DisplayHint.NoPreference
                                                   : Kirigami.DisplayHint.IconOnly;
    }
}
