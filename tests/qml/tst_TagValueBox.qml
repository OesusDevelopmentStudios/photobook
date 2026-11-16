// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.pictorio
import pictorio.tests

// The dropdown of category or rating: what each choice asks the editor to do, and what it shows.
// The TestCase is invisible; what it tests is created in the visible root, where it can be clicked.
Item {
    id: root

    width: 400
    height: 100

    TestCase {
        id: testCase

        name: "TagValueBox"
        when: windowShown

        Component {
            id: boxComponent

            TagValueBox {
                editor: FakeTagRowsEditor {}
                namespaceName: "category"
                options: ["photo", "artwork"]
                value: ""
                changed: false
                fixedValues: false
                keepable: false
            }
        }

        function box(properties: var): TagValueBox {
            return createTemporaryObject(boxComponent, root, properties);
        }

        // For one image: "–", then the values
        function test_showsTheImagesValue() {
            compare(box({value: "artwork"}).currentText, "artwork");
            compare(box({value: ""}).currentText, "–");
        }

        function test_setsTheChosenValue() {
            const valueBox = box({value: "photo"});

            valueBox.activated(2);  // artwork

            compare(valueBox.editor.calls, ["setSingleValue category artwork"]);
        }

        function test_setsNoValue() {
            const valueBox = box({value: "photo"});

            valueBox.activated(0);  // –

            compare(valueBox.editor.calls, ["setSingleValue category"]);
        }

        // For many images: "Keep as is" first, chosen until something else is
        function test_keepsAsIsForManyImages() {
            const valueBox = box({keepable: true, value: "", changed: false});
            compare(valueBox.currentIndex, 0);

            valueBox.activated(1);  // –
            valueBox.activated(0);  // keep as is

            compare(valueBox.editor.calls, ["setSingleValue category", "keepValue category"]);
        }

        // The ratings: only the server's values, nothing typed
        function test_ratingsAreFixed() {
            verify(!box({fixedValues: true}).editable);
            verify(box({fixedValues: false}).editable);
        }
    }
}
