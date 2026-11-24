// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the image components below use the page's ids.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.ki18n

// During a slideshow: a black background, and a control bar that appears when the mouse moves
Kirigami.Page {
    id: page

    required property Viewer viewer
    required property ViewTransform viewTransform
    required property Navigator navigator
    required property Slideshow slideshow
    required property Connection connection
    required property TagEditor tagEditor
    required property TagPaneLayout tagPaneLayout
    required property Uploader uploader
    required property ImageActions imageActions

    signal serverPageRequested()
    signal libraryRequested()
    signal uploadImageRequested()
    signal uploadFolderRequested()  // without subfolders
    signal copyImageRequested()
    signal openFolderRequested()
    signal saveRotationRequested()
    signal leaveRequested()  // back, while there are unsaved tag changes; the window asks first

    onBackRequested: event => {
        if (tagEditor.hasChanges) {
            event.accepted = true;  // the page stays until the changes are saved or dropped
            leaveRequested();
        }
    }

    readonly property bool presenting: slideshow.running
    // Unsaved tag changes are saved or dropped first; stepping on would lose them.
    readonly property bool canStep: !tagEditor.hasChanges
    readonly property bool canStartSlideshow: canStep && viewer.hasImage
                                              && (navigator.canGoToNext || navigator.canGoToPrevious)

    // Only the position, which keeps its width; a long file name would push the controls around. The name is
    // in the details footer and the window title.
    title: navigator.currentIndex < 0 || navigator.imageCount < 1
           ? ""
           : KI18n.i18nc("@title %1 is the image's position among the images, %2 their number", "%1 of %2",
                         navigator.currentIndex + 1, navigator.imageCount)
    padding: 0

    background: Rectangle {
        color: page.presenting ? "black" : Kirigami.Theme.backgroundColor
    }

    actions: [
        Kirigami.Action {
            id: previousAction

            icon.name: "go-previous"
            text: KI18n.i18nc("@action:intoolbar Previous image in the folder or the library", "Previous")
            displayHint: toolbarPriority.displayHint(previousAction)
            shortcut: "Left"
            enabled: page.canStep && page.navigator.canGoToPrevious
            onTriggered: page.navigator.goToPrevious()
        },
        Kirigami.Action {
            id: nextAction

            icon.name: "go-next"
            text: KI18n.i18nc("@action:intoolbar Next image in the folder or the library", "Next")
            displayHint: toolbarPriority.displayHint(nextAction)
            shortcut: "Right"
            enabled: page.canStep && page.navigator.canGoToNext
            onTriggered: page.navigator.goToNext()
        },
        // Library images play in the library's order, so there is no order to choose.
        Kirigami.Action {
            id: librarySlideshowAction

            icon.name: "media-playback-start"
            text: KI18n.i18nc("@action:intoolbar", "Slideshow")
            displayHint: toolbarPriority.displayHint(librarySlideshowAction)
            visible: page.navigator.browsingLibrary
            enabled: page.canStartSlideshow
            onTriggered: page.slideshow.start(page.slideshow.order)
        },
        Kirigami.Action {
            id: folderSlideshowAction

            icon.name: "media-playback-start"
            text: KI18n.i18nc("@action:intoolbar", "Slideshow")
            displayHint: toolbarPriority.displayHint(folderSlideshowAction)
            visible: !page.navigator.browsingLibrary
            enabled: page.canStartSlideshow

            // The order of the last slideshow is checked; F5 starts a slideshow in that order.
            Kirigami.Action {
                text: KI18n.i18nc("@action:inmenu Slideshow order", "By Name")
                checkable: true
                checked: page.slideshow.order === Slideshow.Order.ByName
                Controls.ActionGroup.group: slideshowOrderGroup
                onTriggered: page.slideshow.start(Slideshow.Order.ByName)
            }
            Kirigami.Action {
                text: KI18n.i18nc("@action:inmenu Slideshow order", "By Date Taken")
                checkable: true
                checked: page.slideshow.order === Slideshow.Order.ByDate
                Controls.ActionGroup.group: slideshowOrderGroup
                onTriggered: page.slideshow.start(Slideshow.Order.ByDate)
            }
            Kirigami.Action {
                text: KI18n.i18nc("@action:inmenu Slideshow order", "In Random Order")
                checkable: true
                checked: page.slideshow.order === Slideshow.Order.Random
                Controls.ActionGroup.group: slideshowOrderGroup
                onTriggered: page.slideshow.start(Slideshow.Order.Random)
            }
        },
        Kirigami.Action {
            id: zoomOutAction

            icon.name: "zoom-out"
            text: KI18n.i18nc("@action:intoolbar", "Zoom Out")
            displayHint: toolbarPriority.displayHint(zoomOutAction)
            shortcut: StandardKey.ZoomOut
            enabled: page.viewTransform.canZoomOut
            onTriggered: page.zoomAroundCenter(() => page.viewTransform.zoomOut())
        },
        Kirigami.Action {
            id: zoomInAction

            icon.name: "zoom-in"
            text: KI18n.i18nc("@action:intoolbar", "Zoom In")
            displayHint: toolbarPriority.displayHint(zoomInAction)
            shortcut: StandardKey.ZoomIn
            enabled: page.viewTransform.canZoomIn
            onTriggered: page.zoomAroundCenter(() => page.viewTransform.zoomIn())
        },
        Kirigami.Action {
            id: fitAction

            icon.name: "zoom-fit-best"
            text: KI18n.i18nc("@action:intoolbar Zoom so that the whole image is visible", "Fit")
            displayHint: toolbarPriority.displayHint(fitAction)
            shortcut: "F"
            enabled: page.viewer.hasImage && !page.viewTransform.fitToWindow
            onTriggered: page.viewTransform.fitToWindow = true
        },
        Kirigami.Action {
            id: actualSizeAction

            icon.name: "zoom-original"
            text: KI18n.i18nc("@action:intoolbar Zoom to 100 %", "Actual Size")
            displayHint: toolbarPriority.displayHint(actualSizeAction)
            shortcut: "Ctrl+0"
            enabled: page.viewer.hasImage && page.viewTransform.scale !== 1
            onTriggered: page.zoomAroundCenter(() => page.viewTransform.showActualSize())
        },
        // While the image is turned, if its type can be saved turned without losing quality; the server
        // follows the same rule for library images. Left of the rotate buttons, so that they stay where they are
        // when it appears.
        Kirigami.Action {
            id: saveRotationAction

            icon.name: "document-save"
            text: KI18n.i18nc("@action:intoolbar", "Save Rotation")
            displayHint: toolbarPriority.displayHint(saveRotationAction)
            shortcut: StandardKey.Save
            visible: page.viewer.hasImage && page.viewTransform.rotationAngle !== 0
                     && page.imageActions.canSaveRotation(page.viewer.mimeType)
            enabled: !page.imageActions.busy
            onTriggered: page.saveRotationRequested()
        },
        Kirigami.Action {
            id: rotateLeftAction

            icon.name: "object-rotate-left"
            text: KI18n.i18nc("@action:intoolbar", "Rotate Left")
            displayHint: toolbarPriority.displayHint(rotateLeftAction)
            shortcut: "Ctrl+L"
            enabled: page.viewer.hasImage
            onTriggered: page.viewTransform.rotateCounterclockwise()
        },
        Kirigami.Action {
            id: rotateRightAction

            icon.name: "object-rotate-right"
            text: KI18n.i18nc("@action:intoolbar", "Rotate Right")
            displayHint: toolbarPriority.displayHint(rotateRightAction)
            shortcut: "Ctrl+R"
            enabled: page.viewer.hasImage
            onTriggered: page.viewTransform.rotateClockwise()
        },
        // In the menu; Ctrl+C reaches it unless a text field, e.g. while editing tags, takes it first.
        Kirigami.Action {
            id: copyImageAction

            icon.name: "edit-copy"
            text: KI18n.i18nc("@action:inmenu", "Copy Image")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            shortcut: StandardKey.Copy
            enabled: page.viewer.hasImage && !page.imageActions.busy
            onTriggered: page.copyImageRequested()
        },
        Kirigami.Action {
            id: openFolderAction

            icon.name: "document-open-folder"
            text: KI18n.i18nc("@action:inmenu", "Open Containing Folder")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            // A library image's file is only reachable when the server runs on this computer.
            visible: page.viewer.hasImage && (!page.viewer.fromLibrary || page.connection.localLibraryDir.length > 0)
            onTriggered: page.openFolderRequested()
        },
        // Not while an upload runs: its list is fixed then.
        Kirigami.Action {
            id: importAction

            icon.name: "cloud-upload"
            text: KI18n.i18nc("@action:intoolbar", "Import to Server")
            displayHint: toolbarPriority.displayHint(importAction)
            visible: page.connection.state === Connection.State.LoggedIn && page.viewer.hasImage
                     && !page.viewer.fromLibrary
            enabled: !page.uploader.running

            Kirigami.Action {
                icon.name: "image-x-generic"
                text: KI18n.i18nc("@action:inmenu Import to the server", "This Image")
                onTriggered: page.uploadImageRequested()
            }
            Kirigami.Action {
                icon.name: "folder-pictures"
                text: KI18n.i18nc("@action:inmenu Import to the server", "This Folder")
                onTriggered: page.uploadFolderRequested()
            }
        },
        Kirigami.Action {
            icon.name: "view-preview"
            text: KI18n.i18nc("@action:inmenu", "Library")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            visible: page.connection.state === Connection.State.LoggedIn
            onTriggered: page.libraryRequested()
        },
        Kirigami.Action {
            readonly property bool loggedIn: page.connection.state === Connection.State.LoggedIn

            icon.name: loggedIn ? "configure" : "network-server"
            text: loggedIn ? KI18n.i18nc("@action:inmenu", "Settings…")
                           : KI18n.i18nc("@action:inmenu", "Connect to Server…")
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: page.serverPageRequested()
        }
    ]

    // A narrow window first collapses the toolbar's labels, then moves whole groups into the overflow menu, the
    // least important first: the image navigation stays longest.
    ToolbarPriority {
        id: toolbarPriority

        groups: [
            [{action: previousAction, labelled: false}, {action: nextAction, labelled: false}],
            [{action: rotateLeftAction, labelled: false}, {action: rotateRightAction, labelled: false}],
            [{action: saveRotationAction, labelled: true}],
            [{action: librarySlideshowAction, labelled: true}, {action: folderSlideshowAction, labelled: true}],
            // The rest, each on its own except the zoom pair, from the left of the toolbar
            [{action: zoomOutAction, labelled: false}, {action: zoomInAction, labelled: false}],
            [{action: fitAction, labelled: true}],
            [{action: actualSizeAction, labelled: true}],
            [{action: importAction, labelled: true}]
        ]
        allActions: page.actions
        // A little to spare: Kirigami's buttons may differ from the probes by a few pixels, e.g. a menu arrow.
        availableWidth: page.toolbarWidth - Kirigami.Units.gridUnit
        iconButtonWidth: iconButtonProbe.implicitWidth
        labelOverhead: labelledButtonProbe.implicitWidth - probeMetrics.advanceWidth(labelledButtonProbe.text)
        font: labelledButtonProbe.font
    }

    // The width Kirigami's page header gives its toolbar: all that the title leaves. The header is Kirigami's
    // own, so its toolbar is found among its children.
    readonly property real toolbarWidth: {
        const toolBar = findToolBar(globalToolBarItem);
        return toolBar ? toolBar.width : width;
    }

    function findToolBar(item: Item): Kirigami.ActionToolBar {
        if (!item) {
            return null;
        }
        if (item instanceof Kirigami.ActionToolBar) {
            return item as Kirigami.ActionToolBar;
        }
        for (const child of item.children) {
            const found = findToolBar(child);
            if (found) {
                return found;
            }
        }
        return null;
    }

    // Buttons as Kirigami's toolbar makes them, to measure: flat tool buttons, with an icon only or with a label
    Controls.ToolButton {
        id: iconButtonProbe

        visible: false
        display: Controls.AbstractButton.IconOnly
        icon.name: "zoom-in"
    }

    Controls.ToolButton {
        id: labelledButtonProbe

        visible: false
        display: Controls.AbstractButton.TextBesideIcon
        icon.name: "zoom-fit-best"
        text: "Fit"
    }

    FontMetrics {
        id: probeMetrics

        font: labelledButtonProbe.font
    }

    // Runs zoomAction while keeping the image point under viewportPoint where it is on screen.
    function zoomAround(viewportPoint: point, zoomAction: var) {
        const relativeX = (flickable.contentX + viewportPoint.x - imageFrame.x) / imageFrame.width;
        const relativeY = (flickable.contentY + viewportPoint.y - imageFrame.y) / imageFrame.height;

        zoomAction();  // updates the bindings below, including imageFrame's position and size

        const wantedX = imageFrame.x + relativeX * imageFrame.width - viewportPoint.x;
        const wantedY = imageFrame.y + relativeY * imageFrame.height - viewportPoint.y;
        const maximumX = flickable.contentWidth - flickable.width;
        const maximumY = flickable.contentHeight - flickable.height;
        flickable.contentX = Math.max(0, Math.min(wantedX, maximumX));
        flickable.contentY = Math.max(0, Math.min(wantedY, maximumY));
    }

    function zoomAroundCenter(zoomAction: var) {
        zoomAround(Qt.point(flickable.width / 2, flickable.height / 2), zoomAction);
    }

    // Exclusive, so choosing the checked order again keeps it checked.
    Controls.ActionGroup {
        id: slideshowOrderGroup
    }

    Shortcut {
        sequence: "F5"
        enabled: page.canStartSlideshow && !page.presenting
        onActivated: page.slideshow.start(page.slideshow.order)
    }

    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: page.presenting
        onActivated: page.slideshow.stop()
    }

    Shortcut {
        sequence: "Space"
        enabled: page.presenting
        onActivated: page.slideshow.paused ? page.slideshow.resume() : page.slideshow.pause()
    }

    Binding {
        target: page.viewTransform
        property: "viewportSize"
        value: Qt.size(flickable.width, flickable.height)
    }

    // The image's tags at the left, full height; the image fits beside it, never under it. Library images only,
    // and not in a slideshow.
    TagPane {
        id: tagPane

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: Kirigami.Units.gridUnit * 14
        visible: page.tagPaneLayout.open && page.tagEditor.canEdit && !page.presenting
        tagEditor: page.tagEditor
        layout: page.tagPaneLayout
    }

    Kirigami.Separator {
        id: tagPaneSeparator

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: tagPane.right
        visible: tagPane.visible
    }

    // Drag to pan. The content is at least as large as the viewport, so a small image stays centered.
    Flickable {
        id: flickable

        anchors.fill: parent
        anchors.leftMargin: tagPane.visible ? tagPane.width + tagPaneSeparator.width : 0
        visible: page.viewer.hasImage
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        contentWidth: Math.max(width, page.viewTransform.displayedSize.width)
        contentHeight: Math.max(height, page.viewTransform.displayedSize.height)

        // The rotated, scaled image as it appears on screen.
        Item {
            id: imageFrame

            x: (flickable.contentWidth - width) / 2
            y: (flickable.contentHeight - height) / 2
            width: page.viewTransform.displayedSize.width
            height: page.viewTransform.displayedSize.height

            Loader {
                id: imageLoader

                anchors.centerIn: parent
                width: page.viewTransform.imageSize.width * page.viewTransform.scale
                height: page.viewTransform.imageSize.height * page.viewTransform.scale
                rotation: page.viewTransform.rotationAngle

                // Image shows only the first frame of an animation.
                sourceComponent: page.viewer.imageAnimated ? animatedImage : stillImage
            }
        }

        // Handlers live in the content item, so event positions are in content coordinates.

        // Mouse wheel zooms around the cursor. WheelHandler accepts only mice by default, but on
        // Wayland a mouse wheel can arrive from a device reported as a touchpad.
        WheelHandler {
            target: null
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: event => page.zoomAround(
                Qt.point(event.x - flickable.contentX, event.y - flickable.contentY),
                () => page.viewTransform.zoomBySteps(event.angleDelta.y / 120))  // 120 = one wheel notch
        }

        PinchHandler {
            id: pinchHandler

            target: null
            onScaleChanged: delta => page.zoomAround(
                Qt.point(pinchHandler.centroid.position.x - flickable.contentX,
                         pinchHandler.centroid.position.y - flickable.contentY),
                () => page.viewTransform.zoomBy(delta))
        }

        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: imageMenu.popup()
        }

        // Double-click switches between fit and 100 %.
        TapHandler {
            onDoubleTapped: eventPoint => page.zoomAround(
                Qt.point(eventPoint.position.x - flickable.contentX, eventPoint.position.y - flickable.contentY),
                () => page.viewTransform.fitToWindow ? page.viewTransform.showActualSize()
                                                     : page.viewTransform.fitToWindow = true)
        }
    }

    // The right-click menu: the page's own actions, so it shows and enables them as the toolbar does.
    Components.ConvergentContextMenu {
        id: imageMenu

        actions: [
            copyImageAction,
            openFolderAction,
            separatorAction,
            rotateLeftAction,
            rotateRightAction,
            importSeparatorAction,
            importAction
        ]
    }

    // The menu's separators; declared inside it, they would replace its actions.
    Kirigami.Action {
        id: separatorAction

        separator: true
    }

    Kirigami.Action {
        id: importSeparatorAction

        separator: true
        visible: importAction.visible
    }

    // The file changed under the same address: a new image item reads it again.
    Connections {
        target: page.viewer

        function onImageFileChanged() {
            imageLoader.active = false;
            imageLoader.active = true;
        }
    }

    Component {
        id: stillImage

        Image {
            source: page.viewer.imageSource
            cache: false  // a file can change under the same address, e.g. a rotation saved into it
            autoTransform: true
            asynchronous: true
            smooth: true
            mipmap: true
        }
    }

    Component {
        id: animatedImage

        AnimatedImage {
            source: page.viewer.imageSource
            smooth: true
        }
    }

    // Images from the server can take a moment to arrive.
    Controls.BusyIndicator {
        anchors.centerIn: parent
        running: (imageLoader.item as Image)?.status === Image.Loading
        visible: running
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: !page.viewer.hasImage && page.viewer.errorMessage.length === 0

        icon.name: "image-x-generic"
        text: KI18n.i18nc("@info:placeholder", "No image open")
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.gridUnit * 4
        visible: page.viewer.errorMessage.length > 0 || (imageLoader.item as Image)?.status === Image.Error

        icon.name: "data-error"
        text: KI18n.i18nc("@info:placeholder %1 is a file name", "Cannot open %1", page.viewer.fileName)
        explanation: page.viewer.errorMessage
    }

    ImageDetailsZone {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: page.viewer.hasImage && !page.presenting
        viewer: page.viewer
        tagEditor: page.tagEditor
        tagPaneLayout: page.tagPaneLayout
        availableHeight: page.height
    }

    SlideshowControls {
        anchors.fill: parent
        visible: page.presenting
        slideshow: page.slideshow
        navigator: page.navigator
    }
}
