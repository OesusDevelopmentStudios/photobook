// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// A tag while editing. Kept: ✕ removes it. Added: marked new; ✕ drops it. Removed: struck through; a click keeps
// it after all. With many images, how many of them have it.
Kirigami.Chip {
    id: chip

    required property var modelData  // {"tag", "name", "change", and for many images "count"}
    required property TagRowsEditor editor
    // Not "added" and "removed": a property named removed would hide the Chip's removed() signal, and ✕ would
    // do nothing.
    readonly property bool isAdded: modelData.change === TagRowsEditor.Change.Added
    readonly property bool isRemoved: modelData.change === TagRowsEditor.Change.Removed

    text: (modelData.count ?? 0) > 0
          ? KI18n.i18nc("@item %1 is a tag, %2 how many selected images have it", "%1 (%2)", modelData.name,
                        modelData.count)
          : modelData.name
    icon.name: isAdded ? "list-add" : ""
    font.strikeout: isRemoved
    opacity: isRemoved ? 0.6 : 1
    closable: !isRemoved
    checkable: false
    enabled: !editor.busy
    onRemoved: editor.removeTag(modelData.tag)
    onClicked: {
        if (isRemoved) {
            editor.restoreTag(modelData.tag);
        }
    }

    Controls.ToolTip.text: isRemoved ? KI18n.i18nc("@info:tooltip", "Keep this tag") : ""
    Controls.ToolTip.visible: isRemoved && hovered
    Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
}
