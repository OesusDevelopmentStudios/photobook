// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.pictorio
import pictorio.tests

// The upload page's list: the chosen files, taken off it again, and folders with or without their subfolders.
// The TestCase is invisible; what it tests is created in the visible root, where it can be clicked.
Item {
    id: root

    width: 800
    height: 500

    TestCase {
        id: testCase

        name: "UploadPage"
        when: windowShown

        Component {
            id: pageComponent

            UploadPage {
                anchors.fill: parent
                uploader: TestServices.uploader
                imageNameFilters: []
            }
        }

        function init() {
            TestServices.uploader.clear();
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
    }
}
