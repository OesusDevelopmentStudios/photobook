// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the page components below use the window's id.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs
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

    // Started with an image or a folder: the viewer. Without: the library if a server login is remembered,
    // otherwise the welcome page.
    pageStack.initialPage: {
        if (openedAtStart) {
            return viewerPage;
        }
        return appManager.connection.state === Connection.State.NoServer ? welcomePage : libraryPage;
    }

    // Back only: going forward again is a click on the same image or page.
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton

    // One page at a time: side by side, the image would get only part of a wide window.
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn

    function showServerPage() {
        const loggedIn = appManager.connection.state === Connection.State.LoggedIn;
        pageStack.push(loggedIn ? accountPage : connectPage);
    }

    // Back to the library if it is in the page stack; otherwise it comes in place of the current page.
    function showLibrary() {
        afterTagChangesSettled(showLibraryNow);
    }

    function showLibraryNow() {
        const pages = pageStack.items;
        for (let index = pages.length - 1; index >= 0; --index) {
            if (pages[index] instanceof LibraryPage) {
                pageStack.pop(pages[index]);
                return;
            }
        }
        pageStack.replace(libraryPage);
    }

    // Unsaved tag changes would be lost on leaving the viewer: the user saves or drops them first, or stays.
    property var actionAfterTagChanges: null

    function afterTagChangesSettled(action) {
        if (!appManager.tagEditor.hasChanges) {
            action();
            return;
        }

        actionAfterTagChanges = action;
        unsavedTagsDialog.open();
    }

    // On top of the current page, so going back returns to it.
    function showViewer() {
        if (!(pageStack.currentItem instanceof ViewerPage)) {
            pageStack.push(viewerPage);
        }
    }

    onClosing: close => {
        if (appManager.tagEditor.hasChanges) {
            close.accepted = false;
            afterTagChangesSettled(() => root.close());
        }
    }

    Kirigami.PromptDialog {
        id: unsavedTagsDialog

        title: KI18n.i18nc("@title:window", "Unsaved Tags")
        subtitle: KI18n.i18nc("@info", "Save the changes to this image's tags?")
        standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Discard | Kirigami.Dialog.Cancel

        onAccepted: root.appManager.tagEditor.save()  // the action follows once saved
        onDiscarded: {
            root.appManager.tagEditor.cancel();
            close();
            root.runActionAfterTagChanges();
        }
        onRejected: root.actionAfterTagChanges = null
    }

    function runActionAfterTagChanges() {
        const action = actionAfterTagChanges;
        actionAfterTagChanges = null;
        action?.();
    }

    // Saved: on with what the user wanted. Refused: the user stays, with the reason in the details footer.
    Connections {
        target: root.appManager.tagEditor

        function onSaved() {
            root.runActionAfterTagChanges();
        }

        function onErrorMessageChanged() {
            if (root.appManager.tagEditor.errorMessage.length > 0) {
                root.actionAfterTagChanges = null;
            }
        }
    }

    // The file dialogs' choice of types: images, or anything
    readonly property list<string> imageNameFilters: [
        KI18n.i18nc("@item:inlistbox File type filter",
                    "Images (*.jpg *.jpeg *.png *.webp *.gif *.avif *.heic *.heif *.jxl *.tif *.tiff *.bmp)"),
        KI18n.i18nc("@item:inlistbox File type filter", "All files (*)")
    ]

    FileDialog {
        id: openImageDialog

        title: KI18n.i18nc("@title:window", "Open Image")
        nameFilters: root.imageNameFilters
        onAccepted: {
            root.appManager.openImage(selectedFile);
            root.showViewer();
        }
    }

    FolderDialog {
        id: openFolderDialog

        title: KI18n.i18nc("@title:window", "Open Folder")
        onAccepted: {
            root.appManager.openFolder(selectedFolder);  // its first image follows once it is listed
            root.showViewer();
        }
    }

    Component {
        id: viewerPage

        ViewerPage {
            viewer: root.appManager.viewer
            viewTransform: root.appManager.viewTransform
            navigator: root.appManager.navigator
            slideshow: root.appManager.slideshow
            connection: root.appManager.connection
            tagEditor: root.appManager.tagEditor
            onServerPageRequested: root.showServerPage()
            onLibraryRequested: root.showLibrary()
            onLeaveRequested: root.afterTagChangesSettled(() => root.pageStack.pop())
        }
    }

    Component {
        id: welcomePage

        WelcomePage {
            connection: root.appManager.connection
            onOpenImageRequested: openImageDialog.open()
            onOpenFolderRequested: openFolderDialog.open()
            onServerPageRequested: root.showServerPage()
            onLibraryRequested: root.showLibrary()
        }
    }

    Component {
        id: libraryPage

        LibraryPage {
            library: root.appManager.library
            libraryView: root.appManager.libraryView
            bulkTagEditor: root.appManager.bulkTagEditor
            navigator: root.appManager.navigator
            connection: root.appManager.connection
            onOpenImageRequested: openImageDialog.open()
            onOpenFolderRequested: openFolderDialog.open()
            onServerPageRequested: root.showServerPage()
            onUploadRequested: root.pageStack.push(uploadPage)  // going back returns to the library
            onImageOpenRequested: row => {
                root.appManager.openLibraryImage(row);
                root.showViewer();
            }
        }
    }

    Component {
        id: uploadPage

        UploadPage {
            uploader: root.appManager.uploader
            imageNameFilters: root.imageNameFilters
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

    // Logging in leads from the connect page to the library; logging out turns the account page into the
    // connect page.
    Connections {
        target: root.appManager.connection

        function onStateChanged() {
            const currentPage = root.pageStack.currentItem;
            const loggedIn = root.appManager.connection.state === Connection.State.LoggedIn;
            if (loggedIn && currentPage instanceof ConnectPage) {
                root.showLibrary();
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
