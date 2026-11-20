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
        pageStack.push(loggedIn ? settingsPage : connectPage);
    }

    // Back to the library if it is in the page stack; otherwise it comes in place of the current page.
    function showLibrary() {
        afterChangesSettled(appManager.tagEditor, showLibraryNow);
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

    // Unsaved changes, to an image's tags or to the hidden filters, would be lost on leaving their page: the user
    // saves or drops them first, or stays. The editor is the tag editor or the hidden filter editor.
    property var editorToSettle: null
    property var actionAfterChanges: null

    function afterChangesSettled(editor, action) {
        if (!editor.hasChanges) {
            action();
            return;
        }

        editorToSettle = editor;
        actionAfterChanges = action;
        unsavedChangesDialog.open();
    }

    // On top of the current page, so going back returns to it; also while an upload runs.
    function showUploadPage() {
        pageStack.push(uploadPage);
    }

    // While an upload runs, the other pages show it at the end of their toolbar, and it leads to the upload page.
    Kirigami.Action {
        id: uploadStatusAction

        readonly property Uploader uploader: root.appManager.uploader

        visible: uploader.running
        icon.name: "cloud-upload"
        text: KI18n.i18nc("@info:tooltip %1 and %2 are numbers of files", "Uploading: %1 of %2 done",
                          uploader.filesDoneInRun, uploader.filesInRun)
        displayHint: Kirigami.DisplayHint.KeepVisible
        displayComponent: UploadIndicator {
            action: uploadStatusAction
            progress: uploadStatusAction.uploader.filesInRun > 0
                      ? uploadStatusAction.uploader.filesDoneInRun / uploadStatusAction.uploader.filesInRun : 0
        }
        onTriggered: root.showUploadPage()
    }

    function addUploadStatus(page: Kirigami.Page) {
        page.actions.push(uploadStatusAction);
    }

    // On top of the current page, so going back returns to it.
    function showViewer() {
        if (!(pageStack.currentItem instanceof ViewerPage)) {
            pageStack.push(viewerPage);
        }
    }

    // Unsaved changes first; once they are settled, closing again comes back here for the next, or a running upload.
    onClosing: close => {
        const unsaved = [appManager.tagEditor, appManager.hiddenFilterEditor].find(editor => editor.hasChanges);
        if (unsaved) {
            close.accepted = false;
            afterChangesSettled(unsaved, () => root.close());
        } else if (appManager.uploader.running && !quitDuringUpload) {
            close.accepted = false;
            runningUploadDialog.open();
        }
    }

    property bool quitDuringUpload: false  // the user chose to quit anyway

    // Quitting stops an upload; leaving its page does not.
    Kirigami.PromptDialog {
        id: runningUploadDialog

        title: KI18n.i18nc("@title:window", "Upload Running")
        subtitle: KI18n.i18nc("@info", "Quitting stops the upload. The files not sent yet stay where they are.")
        standardButtons: Kirigami.Dialog.Cancel
        customFooterActions: [
            Kirigami.Action {
                icon.name: "application-exit"
                text: KI18n.i18nc("@action:button", "Quit Anyway")
                onTriggered: {
                    root.quitDuringUpload = true;
                    root.close();
                }
            }
        ]
    }

    Kirigami.PromptDialog {
        id: unsavedChangesDialog

        readonly property bool filters: root.editorToSettle === root.appManager.hiddenFilterEditor

        title: filters ? KI18n.i18nc("@title:window", "Unsaved Hidden Filters")
                       : KI18n.i18nc("@title:window", "Unsaved Tags")
        subtitle: filters ? KI18n.i18nc("@info", "Save the changes to your hidden filters?")
                          : KI18n.i18nc("@info", "Save the changes to this image's tags?")
        standardButtons: Kirigami.Dialog.Save | Kirigami.Dialog.Discard | Kirigami.Dialog.Cancel

        onAccepted: root.editorToSettle.save()  // the action follows once saved
        onDiscarded: {
            root.editorToSettle.cancel();
            close();
            root.runActionAfterChanges();
        }
        onRejected: {
            root.actionAfterChanges = null;
            root.editorToSettle = null;
        }
    }

    function runActionAfterChanges() {
        const action = actionAfterChanges;
        actionAfterChanges = null;
        editorToSettle = null;
        action?.();
    }

    // Saved: on with what the user wanted. Refused: the user stays, with the reason shown by the editor's page.
    Connections {
        target: root.editorToSettle

        function onSaved() {
            root.runActionAfterChanges();
        }

        function onErrorMessageChanged() {
            if (root.editorToSettle.errorMessage.length > 0) {
                root.actionAfterChanges = null;
                root.editorToSettle = null;
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
            uploader: root.appManager.uploader
            imageActions: root.appManager.imageActions
            onServerPageRequested: root.showServerPage()
            onLibraryRequested: root.showLibrary()
            onUploadImageRequested: {
                root.appManager.uploadCurrentImage();
                root.showUploadPage();
            }
            onCopyImageRequested: root.appManager.copyCurrentImage()
            onOpenFolderRequested: root.appManager.openCurrentImageFolder()
            onUploadFolderRequested: {
                root.appManager.uploadCurrentFolder();  // listed in the background, while the page opens
                root.showUploadPage();
            }
            onLeaveRequested: root.afterChangesSettled(root.appManager.tagEditor, () => root.pageStack.pop())
            Component.onCompleted: root.addUploadStatus(this)
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
            Component.onCompleted: root.addUploadStatus(this)
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
            imageActions: root.appManager.imageActions
            onOpenImageRequested: openImageDialog.open()
            onOpenFolderRequested: openFolderDialog.open()
            onServerPageRequested: root.showServerPage()
            onUploadRequested: root.showUploadPage()
            onImageOpenRequested: row => {
                root.appManager.openLibraryImage(row);
                root.showViewer();
            }
            onImageCopyRequested: row => root.appManager.copyLibraryImage(row)
            onImageFolderRequested: row => root.appManager.openLibraryImageFolder(row)
            Component.onCompleted: root.addUploadStatus(this)
        }
    }

    Component {
        id: uploadPage

        UploadPage {
            uploader: root.appManager.uploader
            tagEditor: root.appManager.uploadTagEditor
            imageNameFilters: root.imageNameFilters
        }
    }

    Component {
        id: connectPage

        ConnectPage {
            connection: root.appManager.connection
            Component.onCompleted: root.addUploadStatus(this)
        }
    }

    Component {
        id: settingsPage

        SettingsPage {
            connection: root.appManager.connection
            hiddenFilterEditor: root.appManager.hiddenFilterEditor
            onLeaveRequested: root.afterChangesSettled(root.appManager.hiddenFilterEditor, () => root.pageStack.pop())
            onLogOutRequested: root.afterChangesSettled(root.appManager.hiddenFilterEditor,
                                                        () => root.appManager.connection.logOut())
            Component.onCompleted: root.addUploadStatus(this)
        }
    }

    Connections {
        target: root.appManager.imageActions

        function onCopied() {
            root.showPassiveNotification(KI18n.i18nc("@info", "Image copied"));
        }

        function onFailed(message: string) {
            root.showPassiveNotification(message);
        }
    }

    Connections {
        target: root.appManager.navigator

        function onFolderHasNoImages(folderPath: string) {
            root.showPassiveNotification(KI18n.i18nc("@info %1 is a folder", "No images in %1", folderPath));
        }
    }

    // Logging in leads from the connect page to the library; logging out turns the settings page into the
    // connect page.
    Connections {
        target: root.appManager.connection

        function onStateChanged() {
            const currentPage = root.pageStack.currentItem;
            const loggedIn = root.appManager.connection.state === Connection.State.LoggedIn;
            if (loggedIn && currentPage instanceof ConnectPage) {
                root.showLibrary();
            } else if (!loggedIn && currentPage instanceof SettingsPage) {
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
