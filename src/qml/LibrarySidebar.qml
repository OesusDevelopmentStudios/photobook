// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The search and the choices of how its results are shown, at the left of the library page.
Rectangle {
    id: sidebar

    required property Library library
    required property LibraryView libraryView

    // The page resizes the tiles, keeping its place in the grid.
    signal tileSizeRequested(int gridUnits)

    function focusSearch() {
        searchField.forceActiveFocus();
        searchField.selectAll();
    }

    // The window's colours, so the sidebar stands apart from the grid, as Dolphin's panels do.
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false
    color: Kirigami.Theme.backgroundColor

    Controls.ButtonGroup {
        id: orderGroup
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing

        Kirigami.SearchField {
            id: searchField

            Layout.fillWidth: true
            placeholderText: KI18n.i18nc("@info:placeholder", "Search tags…")
            autoAccept: false  // one search per Enter, not one per key
            onAccepted: sidebar.library.search(text)
            onTextChanged: {
                if (text.length === 0 && sidebar.library.searchText.length > 0) {
                    sidebar.library.search("");  // cleared: all images again
                }
            }

            Controls.ToolTip.text: KI18n.i18nc("@info:tooltip", "For example: category:photo summer -rating:nsfw")
            Controls.ToolTip.visible: hovered && text.length === 0
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        // The user's hidden-by-default filters are added to the search on the server.
        Controls.Label {
            Layout.fillWidth: true
            visible: sidebar.library.effectiveQuery.length > 0
                     && sidebar.library.effectiveQuery !== sidebar.library.searchText
            text: KI18n.i18nc("@info %1 is a search query", "Searched for: %1", sidebar.library.effectiveQuery)
            font: Kirigami.Theme.smallFont
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.Wrap
        }

        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.largeSpacing
            level: 4
            text: KI18n.i18nc("@title:group", "Sort")
        }

        Controls.RadioButton {
            text: KI18n.i18nc("@option:radio Sort order", "Newest first")
            checked: sidebar.library.order === Library.Order.NewestFirst
            Controls.ButtonGroup.group: orderGroup
            onToggled: sidebar.library.order = Library.Order.NewestFirst
        }

        Controls.RadioButton {
            text: KI18n.i18nc("@option:radio Sort order", "Oldest first")
            checked: sidebar.library.order === Library.Order.OldestFirst
            Controls.ButtonGroup.group: orderGroup
            onToggled: sidebar.library.order = Library.Order.OldestFirst
        }

        Controls.RadioButton {
            text: KI18n.i18nc("@option:radio Sort order", "Recently added")
            checked: sidebar.library.order === Library.Order.RecentlyAdded
            Controls.ButtonGroup.group: orderGroup
            onToggled: sidebar.library.order = Library.Order.RecentlyAdded
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Controls.RadioButton {
                Layout.fillWidth: true
                text: KI18n.i18nc("@option:radio Sort order", "Random")
                checked: sidebar.library.order === Library.Order.Random
                Controls.ButtonGroup.group: orderGroup
                onToggled: sidebar.library.order = Library.Order.Random
            }

            Controls.ToolButton {
                icon.name: "media-playlist-shuffle"
                text: KI18n.i18nc("@action:button", "Shuffle Again")
                display: Controls.AbstractButton.IconOnly
                enabled: sidebar.library.order === Library.Order.Random
                onClicked: sidebar.library.order = Library.Order.Random  // chosen again: shuffled again

                Controls.ToolTip.text: text
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }

        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.largeSpacing
            level: 4
            text: KI18n.i18nc("@title:group", "Tile Size")
        }

        Controls.Slider {
            Layout.fillWidth: true
            from: sidebar.libraryView.minimumTileSize
            to: sidebar.libraryView.maximumTileSize
            stepSize: 1
            snapMode: Controls.Slider.SnapAlways
            value: sidebar.libraryView.tileSize
            onMoved: sidebar.tileSizeRequested(value)
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
