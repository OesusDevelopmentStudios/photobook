// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the page components below use the window's id.
pragma ComponentBehavior: Bound

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.ki18n

Kirigami.ApplicationWindow {
    id: root

    required property AppManager appManager
    required property bool openedAtStart  // an image or a folder was given on the command line

    title: appManager.viewer.hasImage
        ? KI18n.i18nc("@title:window %1 is a file name", "%1 — Pictorio", appManager.viewer.fileName)
        : KI18n.i18nc("@title:window", "Pictorio")

    // Kirigami grid units scale with the system font size.
    width: Kirigami.Units.gridUnit * 50
    height: Kirigami.Units.gridUnit * 35

    // A slideshow shows only the image: no toolbar, fullscreen.
    pageStack.globalToolBar.style: appManager.slideshow.running ? Kirigami.ApplicationHeaderStyle.None
                                                                : Kirigami.ApplicationHeaderStyle.Auto

    // Started with an image or a folder: the viewer; without: the welcome page.
    pageStack.initialPage: openedAtStart ? viewerPage : welcomePage

    // One page at a time: side by side, the image would get only part of a wide window.
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn

    function showServerPage() {
        const loggedIn = appManager.connection.state === Connection.State.LoggedIn;
        pageStack.push(loggedIn ? accountPage : connectPage);
    }

    Component {
        id: viewerPage

        ViewerPage {
            viewer: root.appManager.viewer
            viewTransform: root.appManager.viewTransform
            navigator: root.appManager.navigator
            slideshow: root.appManager.slideshow
            connection: root.appManager.connection
            onServerPageRequested: root.showServerPage()
        }
    }

    Component {
        id: welcomePage

        WelcomePage {
            connection: root.appManager.connection
            onImageChosen: fileUrl => {
                root.appManager.openImage(fileUrl);
                root.pageStack.replace(viewerPage);
            }
            onFolderChosen: folderUrl => {
                root.appManager.openFolder(folderUrl);  // its first image follows once it is listed
                root.pageStack.replace(viewerPage);
            }
            onServerPageRequested: root.showServerPage()
        }
    }

    Component {
        id: connectPage

        ConnectPage {
            connection: root.appManager.connection
        }
    }

    Component {
        id: accountPage

        AccountPage {
            connection: root.appManager.connection
        }
    }

    Connections {
        target: root.appManager.navigator

        function onFolderHasNoImages(folderPath: string) {
            root.showPassiveNotification(KI18n.i18nc("@info %1 is a folder", "No images in %1", folderPath));
        }
    }

    // Logging in turns the connect page into the account page; logging out turns it back.
    Connections {
        target: root.appManager.connection

        function onStateChanged() {
            const currentPage = root.pageStack.currentItem;
            const loggedIn = root.appManager.connection.state === Connection.State.LoggedIn;
            if (loggedIn && currentPage instanceof ConnectPage) {
                root.pageStack.replace(accountPage);
            } else if (!loggedIn && currentPage instanceof AccountPage) {
                root.pageStack.replace(connectPage);
            }
        }
    }

    // Restored when the slideshow ends, e.g. a maximized window stays maximized.
    property int visibilityBeforeSlideshow: Window.Windowed
    property bool fullScreenForSlideshow: false

    Connections {
        target: root.appManager.slideshow

        function onStateChanged() {
            const running = root.appManager.slideshow.running;
            if (running && root.visibility !== Window.FullScreen) {
                root.visibilityBeforeSlideshow = root.visibility;
                root.fullScreenForSlideshow = true;
                root.showFullScreen();
            } else if (!running && root.fullScreenForSlideshow) {
                root.fullScreenForSlideshow = false;
                root.visibility = root.visibilityBeforeSlideshow;
            }
        }
    }
}
