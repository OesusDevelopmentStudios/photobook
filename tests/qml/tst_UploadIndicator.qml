// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import io.github.oesusdevelopmentstudios.pictorio

// Each test makes a window of its own, as only an application window gives its pages a toolbar.
Item {
    id: root

    TestCase {
        id: testCase

        name: "UploadIndicator"
        when: windowShown

        Component {
            id: windowComponent

            Kirigami.ApplicationWindow {
                id: window

                property alias statusAction: statusAction
                property int clicks: 0

                visible: true
                width: 800
                height: 400
                pageStack.initialPage: Kirigami.Page {
                    title: "Library"
                    actions: [
                        Kirigami.Action {
                            icon.name: "zoom-in"
                            text: "Larger Tiles"
                        }
                    ]
                    Component.onCompleted: actions.push(statusAction)
                }

                Kirigami.Action {
                    id: statusAction

                    visible: false
                    icon.name: "cloud-upload"
                    text: "Uploading: 1 of 4 done"
                    displayHint: Kirigami.DisplayHint.KeepVisible
                    displayComponent: UploadIndicator {
                        objectName: "uploadIndicator"
                        action: statusAction
                        progress: 0.25
                    }
                    onTriggered: ++window.clicks
                }
            }
        }

        // The indicator the page's toolbar shows, if any
        function shownIndicator(window: Kirigami.ApplicationWindow): Item {
            const indicator = findChild(window, "uploadIndicator");
            return indicator?.visible && indicator.width > 0 ? indicator : null;
        }

        function test_showsWhileTheActionIsVisible() {
            const window = createTemporaryObject(windowComponent, root);
            wait(100);
            compare(shownIndicator(window), null);

            window.statusAction.visible = true;

            tryVerify(() => shownIndicator(window) !== null);
        }

        function test_ringFollowsTheProgress() {
            const window = createTemporaryObject(windowComponent, root);
            window.statusAction.visible = true;
            tryVerify(() => shownIndicator(window) !== null);

            compare(findChild(shownIndicator(window), "doneArc").sweepAngle, 90);
        }

        function test_clickTriggersTheAction() {
            const window = createTemporaryObject(windowComponent, root);
            window.statusAction.visible = true;
            tryVerify(() => shownIndicator(window) !== null);

            mouseClick(shownIndicator(window));

            compare(window.clicks, 1);
        }
    }
}
