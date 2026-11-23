// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The Albums tab's choices: which albums, in what order, at what size. The order and "Show albums with hidden
// images" are remembered; the filter is not.
Rectangle {
    id: sidebar

    required property Albums albums
    required property LibraryView libraryView

    // The view resizes the tiles, keeping its place in the grid.
    signal tileSizeRequested(int gridUnits)

    function focusFilter() {
        filterField.forceActiveFocus();
        filterField.selectAll();
    }

    // The window's colours, as the library's sidebar
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false
    color: Kirigami.Theme.backgroundColor

    Controls.ButtonGroup {
        id: sortGroup
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing

        Kirigami.SearchField {
            id: filterField

            objectName: "albumFilter"
            Layout.fillWidth: true
            placeholderText: KI18n.i18nc("@info:placeholder", "Filter albums…")
            text: sidebar.albums.filterText
            onTextChanged: sidebar.albums.filterText = text  // at each key: the albums are all here
        }

        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.largeSpacing
            level: 4
            text: KI18n.i18nc("@title:group", "Sort")
        }

        Controls.RadioButton {
            objectName: "sortByChange"
            text: KI18n.i18nc("@option:radio Sort order of albums", "Recently changed")
            checked: sidebar.albums.sort === Albums.Sort.RecentlyChanged
            Controls.ButtonGroup.group: sortGroup
            onToggled: sidebar.albums.sort = Albums.Sort.RecentlyChanged
        }

        Controls.RadioButton {
            objectName: "sortByName"
            text: KI18n.i18nc("@option:radio Sort order of albums", "Name")
            checked: sidebar.albums.sort === Albums.Sort.Name
            Controls.ButtonGroup.group: sortGroup
            onToggled: sidebar.albums.sort = Albums.Sort.Name
        }

        Controls.RadioButton {
            objectName: "sortBySize"
            text: KI18n.i18nc("@option:radio Sort order of albums", "Size")
            checked: sidebar.albums.sort === Albums.Sort.Size
            Controls.ButtonGroup.group: sortGroup
            onToggled: sidebar.albums.sort = Albums.Sort.Size
        }

        Controls.CheckBox {
            objectName: "showHiddenAlbums"
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            text: KI18n.i18nc("@option:check", "Show albums with hidden images")
            checked: sidebar.albums.includeHidden
            onToggled: sidebar.albums.includeHidden = checked

            Controls.ToolTip.text: KI18n.i18nc("@info:tooltip",
                                               "Also list albums with images that your hidden filters match")
            Controls.ToolTip.visible: hovered
            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
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
