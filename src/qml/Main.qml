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
        ? KI18n.i18nc("@title:window %1 is a file name", "%1 — Photobook", appManager.viewer.fileName)
        : KI18n.i18nc("@title:window", "Photobook")

    width: Kirigami.Units.gridUnit * 50
    height: Kirigami.Units.gridUnit * 35

    // A slideshow shows only the image: no toolbar, fullscreen.
    pageStack.globalToolBar.style: appManager.slideshow.running ? Kirigami.ApplicationHeaderStyle.None
                                                                : Kirigami.ApplicationHeaderStyle.Auto

    // Started with an image or a folder: the viewer. Without: the library while the kept login is restored,
    // otherwise the welcome page. Restoring has begun by now; if nothing was kept, the welcome page follows.
    pageStack.initialPage: {
        if (openedAtStart) {
            return viewerPage;
        }
        const state = appManager.connection.state;
        return state === Connection.State.RestoringLogin || state === Connection.State.LoggedIn ? libraryPage
                                                                                               : welcomePage;
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

    // Unsaved changes, to an image's tags or to the hidden filters, would be lost on leaving their page, and tag
    // changes for many images on leaving the selection: the user saves (applies) or drops them first, or stays.
    // The editor is the tag editor, the hidden filter editor or the bulk tag editor.
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

    // Leaving the library, e.g. for the settings: tag changes not applied to the selected images are settled first.
    // The selection stays, for coming back.
    function leaveLibrary(action) {
        afterChangesSettled(appManager.bulkTagEditor, action);
    }

    // Without a login: the welcome page alone, to choose a server again
    function showWelcome() {
        pageStack.clear();
        pageStack.push(welcomePage);
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
        onTriggered: root.leaveLibrary(root.showUploadPage)  // on another page, nothing to settle
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
        const unsaved = [appManager.tagEditor, appManager.hiddenFilterEditor, appManager.bulkTagEditor].find(
            editor => editor.hasChanges);
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
        readonly property bool bulk: root.editorToSettle === root.appManager.bulkTagEditor
        // With no image selected, bulk changes have nothing to apply to.
        readonly property bool canApply: bulk && root.appManager.bulkTagEditor.imageCount > 0

        title: filters ? KI18n.i18nc("@title:window", "Unsaved Hidden Filters")
                       : bulk ? KI18n.i18nc("@title:window", "Tag Changes Not Applied")
                              : KI18n.i18nc("@title:window", "Unsaved Tags")
        subtitle: {
            if (filters) {
                return KI18n.i18nc("@info", "Save the changes to your hidden filters?");
            }

            if (!bulk) {
                return KI18n.i18nc("@info", "Save the changes to this image's tags?");
            }

            return canApply ? KI18n.i18ncp("@info", "Apply the tag changes to the selected image?",
                                           "Apply the tag changes to the %1 selected images?",
                                           root.appManager.bulkTagEditor.imageCount)
                            : KI18n.i18nc("@info", "No image is selected to apply the tag changes to.");
        }
        standardButtons: {
            const keep = bulk ? (canApply ? Kirigami.Dialog.Apply : Kirigami.Dialog.NoButton) : Kirigami.Dialog.Save;
            return keep | Kirigami.Dialog.Discard | Kirigami.Dialog.Cancel;
        }

        onAccepted: root.editorToSettle.save()  // the action follows once saved
        onApplied: {
            root.editorToSettle.apply();  // the action follows once applied
            close();
        }
        onDiscarded: {
            if (bulk) {
                root.editorToSettle.clear();
            } else {
                root.editorToSettle.cancel();
            }
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
        ignoreUnknownSignals: true  // the bulk tag editor applies; the others save

        function onSaved() {
            root.runActionAfterChanges();
        }

        function onApplied() {
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
            onSaveRotationRequested: root.appManager.saveCurrentRotation()
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
            onLocalServerRequested: root.pageStack.push(localServerPage)
            onLibraryRequested: root.showLibrary()
            Component.onCompleted: root.addUploadStatus(this)
        }
    }

    Component {
        id: localServerPage

        // The connect page checks the local address at once; the user logs in there.
        LocalServerPage {
            onConnectRequested: address => {
                root.appManager.connection.checkServer(address);
                root.pageStack.push(connectPage);
            }
        }
    }

    Component {
        id: libraryPage

        LibraryPage {
            library: root.appManager.library
            albums: root.appManager.albums
            libraryView: root.appManager.libraryView
            bulkTagEditor: root.appManager.bulkTagEditor
            searchSuggestions: root.appManager.searchSuggestions
            navigator: root.appManager.navigator
            connection: root.appManager.connection
            imageActions: root.appManager.imageActions
            onOpenImageRequested: root.leaveLibrary(() => openImageDialog.open())
            onOpenFolderRequested: root.leaveLibrary(() => openFolderDialog.open())
            onServerPageRequested: root.leaveLibrary(root.showServerPage)
            onUploadRequested: root.leaveLibrary(root.showUploadPage)
            onImageOpenRequested: row => {
                root.appManager.openLibraryImage(row);
                root.showViewer();
            }
            onAlbumOpenRequested: row => root.appManager.openAlbum(row)  // the viewer once its first image is there
            onImageCopyRequested: row => root.appManager.copyLibraryImage(row)
            onImageFolderRequested: row => root.appManager.openLibraryImageFolder(row)
            onStopSelectingRequested: root.afterChangesSettled(root.appManager.bulkTagEditor,
                                                               () => root.appManager.library.stopSelecting())
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
            libraryView: root.appManager.libraryView
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

        function onRotationSaved() {
            root.showPassiveNotification(KI18n.i18nc("@info", "Rotation saved"));
        }

        function onFailed(message: string) {
            root.showPassiveNotification(message);
        }
    }

    // An album opens in the viewer at its first image; Back returns to the Albums tab. Not if the user went
    // elsewhere while it loaded.
    Connections {
        target: root.appManager

        function onAlbumOpened() {
            if (root.pageStack.currentItem instanceof LibraryPage) {
                root.showViewer();
            }
        }

        function onAlbumNotOpened(message: string) {
            root.showPassiveNotification(message);
        }
    }

    Connections {
        target: root.appManager.navigator

        function onFolderHasNoImages(folderPath: string) {
            root.showPassiveNotification(KI18n.i18nc("@info %1 is a folder", "No images in %1", folderPath));
        }
    }

    // Logging in leads from the connect page to the library. Logging out, or a kept login the server refuses
    // (or none kept), leads to the welcome page. A server that does not answer keeps the library, to try again.
    Connections {
        target: root.appManager.connection

        function onStateChanged() {
            const currentPage = root.pageStack.currentItem;
            const state = root.appManager.connection.state;
            const withoutLogin = state === Connection.State.ServerFound || state === Connection.State.NoServer;
            if (state === Connection.State.LoggedIn && currentPage instanceof ConnectPage) {
                root.showLibrary();
            } else if (withoutLogin && (currentPage instanceof SettingsPage || currentPage instanceof LibraryPage)) {
                root.showWelcome();
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
