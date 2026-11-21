// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtQuick.Controls as Controls
import QtTest
import io.github.oesusdevelopmentstudios.pictorio

Item {
    id: root

    width: 800
    height: 700

    // Where the test pastes what was copied
    Controls.TextField {
        id: pasted
    }

    TestCase {
        id: testCase

        name: "LocalServerPage"
        when: windowShown

        Component {
            id: pageComponent

            LocalServerPage {
                width: root.width
                height: root.height
            }
        }

        SignalSpy {
            id: connectRequests

            signalName: "connectRequested"
        }

        function test_copiesTheInstallCommand() {
            const page = createTemporaryObject(pageComponent, root);
            pasted.clear();

            mouseClick(findChild(page, "copyCommand"));
            pasted.paste();

            compare(pasted.text, page.installCommand);
            verify(page.installCommand.includes("/releases/latest/download/install.sh | sh"));
            verify(findChild(page, "copiedMessage").visible);
        }

        function test_connectsToTheScriptsDefaultAddress() {
            const page = createTemporaryObject(pageComponent, root);
            connectRequests.clear();
            connectRequests.target = page;

            mouseClick(findChild(page, "connect"));

            compare(connectRequests.count, 1);
            compare(connectRequests.signalArguments[0][0], "http://localhost:8737");
        }

        function test_usesThePortTyped() {
            const page = createTemporaryObject(pageComponent, root);
            connectRequests.clear();
            connectRequests.target = page;
            const port = findChild(page, "port");
            port.selectAll();
            port.forceActiveFocus();
            keyClick(Qt.Key_9);
            keyClick(Qt.Key_0);
            keyClick(Qt.Key_0);
            keyClick(Qt.Key_0);

            compare(page.webAdminUrl, "http://localhost:9000/admin");
            mouseClick(findChild(page, "connect"));
            compare(connectRequests.signalArguments[0][0], "http://localhost:9000");
        }

        function test_needsAPort() {
            const page = createTemporaryObject(pageComponent, root);
            const port = findChild(page, "port");
            port.selectAll();
            port.forceActiveFocus();
            keyClick(Qt.Key_Backspace);

            verify(!page.portValid);
            verify(!findChild(page, "connect").enabled);
        }
    }
}
