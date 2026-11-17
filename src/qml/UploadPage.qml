// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

// Lets the delegate below use the page's ids.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.ki18n

// Sending images to the server's library: the files to upload, and how each of them went.
Kirigami.Page {
    id: page

    required property Uploader uploader
    required property list<string> imageNameFilters  // for the file dialog, as the app's other one

    title: KI18n.i18nc("@title", "Upload")

    // The folder's images, and with "Include subfolders" also those below it. Listed in the background.
    function addFolder(folderUrl: url) {
        uploader.addFolder(folderUrl, includeSubfoldersBox.checked);
    }

    function stateText(fileState: int): string {
        switch (fileState) {
        case Uploader.FileState.Uploading:
            return KI18n.i18nc("@info:status of a file", "Uploading…");
        case Uploader.FileState.Uploaded:
            return KI18n.i18nc("@info:status of a file", "Uploaded");
        case Uploader.FileState.AlreadyThere:
            return KI18n.i18nc("@info:status of a file", "Already on the server");
        case Uploader.FileState.Failed:
            return KI18n.i18nc("@info:status of a file", "Failed");
        }
        return KI18n.i18nc("@info:status of a file", "Waiting");
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Controls.Button {
                icon.name: "list-add"
                text: KI18n.i18nc("@action:button", "Add Files…")
                enabled: !page.uploader.running
                onClicked: filesDialog.open()
            }

            Controls.Button {
                icon.name: "folder-add"
                text: KI18n.i18nc("@action:button", "Add Folder…")
                enabled: !page.uploader.running
                onClicked: folderDialog.open()
            }

            Controls.CheckBox {
                id: includeSubfoldersBox

                objectName: "includeSubfolders"
                text: KI18n.i18nc("@option:check", "Include subfolders")
                enabled: !page.uploader.running
            }

            Item {
                Layout.fillWidth: true
            }

            Controls.BusyIndicator {
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                running: page.uploader.findingFiles
                visible: running
            }

            Controls.Label {
                objectName: "fileCount"
                text: KI18n.i18ncp("@info", "%1 image", "%1 images", page.uploader.count)
            }
        }

        // The list in the colours of a view, like the file list in Dolphin
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false
            color: Kirigami.Theme.backgroundColor
            border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor,
                                                                   Kirigami.Theme.textColor, 0.2)

            ListView {
                id: fileList

                objectName: "fileList"
                anchors.fill: parent
                anchors.margins: 1  // inside the border
                model: page.uploader
                clip: true

                Controls.ScrollBar.vertical: Controls.ScrollBar {}

                delegate: Controls.ItemDelegate {
                    id: fileRow

                    required property int index
                    required property string fileName
                    required property string folder
                    required property int fileState
                    required property string message

                    width: ListView.view.width
                    down: false  // a row is not a button
                    hoverEnabled: false

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.largeSpacing

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Controls.Label {
                                Layout.fillWidth: true
                                text: fileRow.fileName
                                elide: Text.ElideMiddle
                            }

                            Controls.Label {
                                Layout.fillWidth: true
                                text: fileRow.folder
                                elide: Text.ElideMiddle
                                font: Kirigami.Theme.smallFont
                                color: Kirigami.Theme.disabledTextColor
                            }

                            // Why it failed, or what else happened
                            Controls.Label {
                                Layout.fillWidth: true
                                visible: text.length > 0
                                text: fileRow.message
                                wrapMode: Text.Wrap
                                font: Kirigami.Theme.smallFont
                                color: fileRow.fileState === Uploader.FileState.Failed
                                       ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.neutralTextColor
                            }
                        }

                        Controls.BusyIndicator {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                            running: fileRow.fileState === Uploader.FileState.Uploading
                            visible: running
                        }

                        Kirigami.Icon {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.small
                            Layout.preferredHeight: Kirigami.Units.iconSizes.small
                            visible: source !== ""
                            source: {
                                switch (fileRow.fileState) {
                                case Uploader.FileState.Uploaded:
                                case Uploader.FileState.AlreadyThere:
                                    return "checkmark";
                                case Uploader.FileState.Failed:
                                    return "dialog-error";
                                }
                                return "";
                            }
                        }

                        Controls.Label {
                            text: page.stateText(fileRow.fileState)
                        }

                        Controls.ToolButton {
                            objectName: "removeFile"
                            icon.name: "list-remove"
                            text: KI18n.i18nc("@action:button", "Remove from List")
                            display: Controls.AbstractButton.IconOnly
                            enabled: !page.uploader.running
                            onClicked: page.uploader.removeFile(fileRow.index)

                            Controls.ToolTip.text: text
                            Controls.ToolTip.visible: hovered
                            Controls.ToolTip.delay: Kirigami.Units.toolTipDelay
                        }
                    }
                }
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: page.uploader.count === 0 && !page.uploader.findingFiles
                icon.name: "cloud-upload"
                text: KI18n.i18nc("@info:placeholder", "No images to upload yet")
                explanation: KI18n.i18nc("@info:placeholder", "Add files, or a folder of images.")
            }
        }
    }

    FileDialog {
        id: filesDialog

        title: KI18n.i18nc("@title:window", "Add Images to Upload")
        fileMode: FileDialog.OpenFiles
        nameFilters: page.imageNameFilters
        onAccepted: page.uploader.addFiles(selectedFiles)
    }

    FolderDialog {
        id: folderDialog

        title: KI18n.i18nc("@title:window", "Add Folder to Upload")
        onAccepted: page.addFolder(selectedFolder)
    }
}
