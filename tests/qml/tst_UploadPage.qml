// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.pictorio
import pictorio.tests

// The connection is not logged in, so every file fails.
Item {
    id: root

    // Room for the whole page, so the buttons at its bottom can be clicked without scrolling
    width: 800
    height: 700

    TestCase {
        id: testCase

        name: "UploadPage"
        when: windowShown

        Component {
            id: pageComponent

            UploadPage {
                anchors.fill: parent
                uploader: TestServices.uploader
                tagEditor: TestServices.uploadTagEditor
                imageNameFilters: []
            }
        }

        function init() {
            TestServices.uploader.clear();
            TestServices.uploader.moveOriginalsToTrash = false;
        }

        function shownPage(): UploadPage {
            return createTemporaryObject(pageComponent, root);
        }

        // The list lays out its rows after the model changes: until then a new row has none, and a removed one
        // still answers for its old place (with index -1).
        function rowAt(list: ListView, index: int): Item {
            tryVerify(() => list.itemAtIndex(index)?.index === index);
            return list.itemAtIndex(index);
        }

        function test_listsTheChosenFiles() {
            const page = shownPage();

            TestServices.uploader.addFiles([TestServices.sampleFile("beach.jpg"),
                                            TestServices.sampleFile("day2/sunset.jpg")]);

            const list = findChild(page, "fileList");
            tryCompare(list, "count", 2);
            compare(rowAt(list, 0).fileName, "beach.jpg");
            compare(findChild(page, "fileCount").text, "2 images");
        }

        function test_takesAFileOffTheList() {
            const page = shownPage();
            TestServices.uploader.addFiles([TestServices.sampleFile("beach.jpg"),
                                            TestServices.sampleFile("day2/sunset.jpg")]);
            const list = findChild(page, "fileList");
            tryCompare(list, "count", 2);

            mouseClick(findChild(rowAt(list, 0), "removeFile"));

            tryCompare(list, "count", 1);
            compare(rowAt(list, 0).fileName, "sunset.jpg");
        }

        function test_addsSubfoldersOnlyWhenTicked() {
            const page = shownPage();

            page.addFolder(TestServices.sampleFolder);
            tryCompare(TestServices.uploader, "findingFiles", false);
            compare(TestServices.uploader.count, 1);

            mouseClick(findChild(page, "includeSubfolders"));
            page.addFolder(TestServices.sampleFolder);
            tryCompare(TestServices.uploader, "count", 2);  // the subfolder's image; beach.jpg was listed already
        }

        function test_startsOnlyWithFilesWaiting() {
            const page = shownPage();
            const start = findChild(page, "startUpload");
            verify(!start.enabled);

            TestServices.uploader.addFiles([TestServices.sampleFile("beach.jpg")]);

            verify(start.enabled);
        }

        function test_tellsHowTheFilesWent() {
            const page = shownPage();
            TestServices.uploader.addFiles([TestServices.sampleFile("beach.jpg"),
                                            TestServices.sampleFile("day2/sunset.jpg")]);

            mouseClick(findChild(page, "startUpload"));

            tryCompare(TestServices.uploader, "failedCount", 2);
            tryCompare(TestServices.uploader, "running", false);
            compare(findChild(page, "summary").text, "2 failed");
            verify(!findChild(page, "startUpload").enabled);  // nothing is waiting any more
            verify(!findChild(page, "cancelUpload").enabled);
        }

        // A window too small for the page: it scrolls, and nothing overlaps.
        function test_scrollsInASmallWindow() {
            const small = createTemporaryQmlObject("import QtQuick; Item { width: 360; height: 260 }", root);
            const page = createTemporaryObject(pageComponent, small);
            const frame = findChild(page, "fileListFrame");
            const placeholder = findChild(page, "emptyPlaceholder");

            tryVerify(() => placeholder.implicitHeight <= frame.height, 1000,
                      "the empty list's placeholder sticks out of its frame");
            tryVerify(() => lastButtonReachable(page), 1000, "the page cannot be scrolled to its last buttons");
        }

        // Scrolled to its end (if it can scroll), the page shows its last button.
        function lastButtonReachable(page: UploadPage): bool {
            const flickable = findChild(page, "pageScroll")?.contentItem;
            if (flickable) {
                flickable.contentX = Math.max(0, flickable.contentWidth - flickable.width);
                flickable.contentY = Math.max(0, flickable.contentHeight - flickable.height);
            }
            const cancel = findChild(page, "cancelUpload");
            const corner = cancel.mapToItem(page, cancel.width, cancel.height);
            return corner.x <= page.width && corner.y <= page.height;
        }

        function test_choosesToMoveTheOriginalsToTheTrash() {
            const page = shownPage();
            const moveToTrash = findChild(page, "moveToTrash");
            verify(!moveToTrash.checked);

            mouseClick(moveToTrash);

            verify(TestServices.uploader.moveOriginalsToTrash);
        }
    }
}
