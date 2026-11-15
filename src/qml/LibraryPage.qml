// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the delegate below use the page's ids.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// The images on the server: a search bar and a grid of square thumbnails, newest first. More images load
// as the grid nears its end.
Kirigami.Page {
    id: page

    required property Library library
    required property Connection connection

    signal openImageRequested()
    signal openFolderRequested()
    signal serverPageRequested()

    readonly property int connectionState: connection.state
    readonly property bool loggedIn: connectionState === Connection.State.LoggedIn

    title: KI18n.i18nc("@title", "Library")
    padding: 0

    header: Controls.ToolBar {
        visible: page.loggedIn

        Kirigami.SearchField {
            id: searchField

            anchors.left: parent.left
            anchors.right: parent.right
            placeholderText: KI18n.i18nc("@info:placeholder", "Search, e.g. category:photo summer -rating:nsfw")
            autoAccept: false  // one search per Enter, not one per key
            onAccepted: page.library.search(text)
            onTextChanged: {
                if (text.length === 0 && page.library.searchText.length > 0) {
                    page.library.search("");  // cleared: all images again
                }
            }
        }
    }

    actions: [
        Kirigami.Action {
            icon.name: "view-sort"
            text: KI18n.i18nc("@action:intoolbar", "Sort")
            visible: page.loggedIn

            Kirigami.Action {
                text: KI18n.i18nc("@action:inmenu Sort order", "Newest First")
                checkable: true
                checked: page.library.order === Library.Order.NewestFirst
                Controls.ActionGroup.group: orderGroup
                onTriggered: page.library.order = Library.Order.NewestFirst
            }
            Kirigami.Action {
                text: KI18n.i18nc("@action:inmenu Sort order", "Oldest First")
                checkable: true
                checked: page.library.order === Library.Order.OldestFirst
                Controls.ActionGroup.group: orderGroup
                onTriggered: page.library.order = Library.Order.OldestFirst
            }
            Kirigami.Action {
                text: KI18n.i18nc("@action:inmenu Sort order", "Recently Added")
                checkable: true
                checked: page.library.order === Library.Order.RecentlyAdded
                Controls.ActionGroup.group: orderGroup
                onTriggered: page.library.order = Library.Order.RecentlyAdded
            }
            Kirigami.Action {
                text: KI18n.i18nc("@action:inmenu Sort order", "Random")
                checkable: true
                checked: page.library.order === Library.Order.Random
                Controls.ActionGroup.group: orderGroup
                onTriggered: page.library.order = Library.Order.Random  // again: shuffled again
            }
        },
        Kirigami.Action {
            icon.name: "document-open"
            text: KI18n.i18nc("@action:inmenu", "Open Image…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.openImageRequested()
        },
        Kirigami.Action {
            icon.name: "folder-open"
            text: KI18n.i18nc("@action:inmenu", "Open Folder…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.openFolderRequested()
        },
        Kirigami.Action {
            icon.name: page.loggedIn ? "user-identity" : "network-server"
            text: page.loggedIn ? KI18n.i18nc("@action:inmenu", "Account…")
                                : KI18n.i18nc("@action:inmenu", "Connect to Server…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.serverPageRequested()
        }
    ]

    Controls.ActionGroup {
        id: orderGroup
    }

    GridView {
        id: grid

        // As many columns of about 8 grid units as fit; at least two.
        readonly property int columns: Math.max(2, Math.floor(width / (Kirigami.Units.gridUnit * 8)))

        anchors.fill: parent
        visible: page.loggedIn
        model: page.loggedIn ? page.library : null
        clip: true
        cellWidth: width / columns
        cellHeight: cellWidth
        cacheBuffer: cellHeight * 2  // rows above and below the window are made in advance
        reuseItems: true  // tiles scrolled away are reused, not made anew

        Controls.ScrollBar.vertical: Controls.ScrollBar {}

        // The next page loads while two screens of images are still ahead.
        function loadMoreIfNearTheEnd() {
            if (contentY + height * 3 > contentHeight) {
                page.library.loadMore();
            }
        }

        onContentYChanged: loadMoreIfNearTheEnd()
        onCountChanged: loadMoreIfNearTheEnd()  // a short page may not fill the window
        onHeightChanged: loadMoreIfNearTheEnd()

        // Kept light, as many are made while scrolling: no clip (the crop stays inside the tile by itself)
        // and no tooltip.
        delegate: Rectangle {
            id: tile

            required property url thumbnailUrl

            width: grid.cellWidth
            height: grid.cellHeight
            color: Kirigami.Theme.alternateBackgroundColor
            border.color: Kirigami.Theme.backgroundColor
            border.width: 1

            Image {
                anchors.fill: parent
                anchors.margins: 1
                source: tile.thumbnailUrl
                fillMode: Image.PreserveAspectCrop  // square tiles, the image cropped to fill them
                asynchronous: true
                smooth: true
            }
        }

        footer: Item {
            width: grid.width
            height: page.library.loading ? Kirigami.Units.gridUnit * 3 : 0

            Controls.BusyIndicator {
                anchors.centerIn: parent
                running: page.library.loading
                visible: running
            }
        }
    }

    Kirigami.InlineMessage {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Kirigami.Units.smallSpacing
        visible: page.loggedIn && page.library.errorMessage.length > 0
        type: Kirigami.MessageType.Error
        text: page.library.errorMessage
        actions: [
            Kirigami.Action {
                icon.name: "view-refresh"
                text: KI18n.i18nc("@action:button", "Try Again")
                onTriggered: page.library.reload()
            }
        ]
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: page.loggedIn && page.library.count === 0 && !page.library.loading
                 && page.library.errorMessage.length === 0
        icon.name: "image-x-generic"
        text: page.library.searchText.length > 0 ? KI18n.i18nc("@info:placeholder", "Nothing matches")
                                                 : KI18n.i18nc("@info:placeholder", "No images in the library yet")
        explanation: page.library.searchText.length > 0 ? page.library.effectiveQuery : ""
    }

    Kirigami.LoadingPlaceholder {
        anchors.centerIn: parent
        visible: page.connectionState === Connection.State.RestoringLogin
        text: KI18n.i18nc("@info:placeholder", "Connecting to %1…", page.connection.serverAddress)
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: page.connectionState === Connection.State.Offline
        icon.name: "network-disconnect"
        text: KI18n.i18nc("@info:placeholder %1 is a server address", "%1 does not answer",
                          page.connection.serverAddress)
        explanation: page.connection.errorMessage
        helpfulAction: Kirigami.Action {
            icon.name: "view-refresh"
            text: KI18n.i18nc("@action:button", "Try Again")
            onTriggered: page.connection.restoreLogin()
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !page.loggedIn && page.connectionState !== Connection.State.RestoringLogin
                 && page.connectionState !== Connection.State.Offline
        icon.name: "network-server"
        text: KI18n.i18nc("@info:placeholder", "Not logged in")
        explanation: page.connection.errorMessage
        helpfulAction: Kirigami.Action {
            icon.name: "go-next"
            text: KI18n.i18nc("@action:button", "Log In…")
            onTriggered: page.serverPageRequested()
        }
    }
}
