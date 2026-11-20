// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import org.kde.kirigami as Kirigami

// How a page's toolbar actions show when the toolbar is too narrow for all of them with their labels: first
// every label collapses to an icon; only then do whole groups move to the overflow menu, the least important
// first. Kirigami alone would move the rightmost actions first, whatever they are, and keep the labels.
// Each action takes its display hint from displayHint(action).
QtObject {
    id: priority

    // Most important first. Each group is a list of {action, labelled}; its actions come and go together.
    // labelled: shown with its text while every label fits.
    required property var groups
    required property real availableWidth  // the toolbar's, with room for its overflow menu button
    required property real iconButtonWidth
    required property real labelOverhead  // what a button with a label needs besides the text
    required property font font
    property real spacing: Kirigami.Units.smallSpacing
    // All of the toolbar's actions: those in no group, e.g. a running upload's, count as icons
    property list<QtObject> allActions

    // 0: the labels show. 1: icons only. 2 and up: one more group in the overflow menu each, the least
    // important first; groups.length + 1: all of them.
    readonly property int level: {
        const lastLevel = groups.length + 1;
        for (let candidate = 0; candidate < lastLevel; ++candidate) {
            if (neededWidth(candidate) <= availableWidth) {
                return candidate;
            }
        }
        return lastLevel;
    }

    readonly property FontMetrics metrics: FontMetrics {
        font: priority.font
    }

    function isHidden(group: int, atLevel: int): bool {
        return atLevel >= groups.length + 1 - group;
    }

    function inGroup(action: QtObject): bool {
        return groups.some(members => members.some(member => member.action === action));
    }

    // The overflow menu's button is always there: some actions are only ever in the menu.
    function neededWidth(atLevel: int): real {
        let width = iconButtonWidth;
        for (const action of allActions) {
            const kirigamiAction = action as Kirigami.Action;
            if (kirigamiAction?.visible && !inGroup(action)
                    && !(kirigamiAction.displayHint & Kirigami.DisplayHint.AlwaysHide)) {
                width += spacing + iconButtonWidth;
            }
        }
        for (let group = 0; group < groups.length; ++group) {
            if (isHidden(group, atLevel)) {
                continue;
            }
            for (const member of groups[group]) {
                if (member.action.visible) {
                    const withLabel = atLevel === 0 && member.labelled;
                    width += spacing + (withLabel ? labelOverhead + metrics.advanceWidth(member.action.text)
                                                  : iconButtonWidth);
                }
            }
        }
        return width;
    }

    function displayHint(action: Kirigami.Action): int {
        for (let group = 0; group < groups.length; ++group) {
            const member = groups[group].find(candidate => candidate.action === action);
            if (member === undefined) {
                continue;
            }
            if (isHidden(group, level)) {
                return Kirigami.DisplayHint.AlwaysHide;
            }
            return level === 0 && member.labelled ? Kirigami.DisplayHint.NoPreference : Kirigami.DisplayHint.IconOnly;
        }
        return Kirigami.DisplayHint.NoPreference;
    }
}
