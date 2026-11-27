// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

// The controls without a video: they show what they are given and ask; the player keeps speed and sound.
Item {
    id: root

    width: 800
    height: 200

    TestCase {
        name: "VideoControls"
        when: windowShown

        Component {
            id: controlsComponent

            VideoControls {
                width: root.width
                videoPlayer: TestServices.videoPlayer
                position: 42000
                duration: 222400
                seekable: true
            }
        }

        SignalSpy {
            id: playPauseSpy

            signalName: "playPauseRequested"
        }

        SignalSpy {
            id: seekSpy

            signalName: "seekRequested"
        }

        function shownControls(): VideoControls {
            const controls = createTemporaryObject(controlsComponent, root);
            for (const spy of [playPauseSpy, seekSpy]) {
                spy.clear();
                spy.target = controls;
            }
            waitForRendering(controls);
            return controls;
        }

        function cleanup() {
            TestServices.videoPlayer.speed = 1;
            TestServices.videoPlayer.volume = 1;
            TestServices.videoPlayer.muted = false;
        }

        function test_showsWhereItIsAndHowLong() {
            const controls = shownControls();

            compare(findChild(controls, "positionLabel").text, "0:42");
            compare(findChild(controls, "durationLabel").text, "3:42");
            compare(findChild(controls, "speedButton").text, "1×");
        }

        function test_asksToPlayAndPause() {
            const controls = shownControls();
            const button = findChild(controls, "playPauseButton");
            compare(button.icon.name, "media-playback-start");

            mouseClick(button);
            compare(playPauseSpy.count, 1);

            controls.playing = true;
            compare(button.icon.name, "media-playback-pause");
        }

        function test_scrubbingAsksToSeek() {
            const controls = shownControls();
            const scrubBar = findChild(controls, "scrubBar");

            mouseClick(scrubBar, scrubBar.width / 2, scrubBar.height / 2);

            compare(seekSpy.count, 1);
            fuzzyCompare(seekSpy.signalArguments[0][0], 222400 / 2, 222400 * 0.05);
        }

        function test_choosesTheSpeed() {
            const controls = shownControls();
            mouseClick(findChild(controls, "speedButton"));
            const menu = findChild(controls, "speedMenu");
            tryCompare(menu, "opened", true);

            const item = menu.itemAt(4);  // 0.25, 0.5, 1, 1.25, 1.5, 2
            compare(item.text, "1.5×");
            mouseClick(item);

            compare(TestServices.videoPlayer.speed, 1.5);
            compare(findChild(controls, "speedButton").text, "1.5×");
        }

        function test_mutesAndUnmutes() {
            const controls = shownControls();
            const button = findChild(controls, "muteButton");

            mouseClick(button);
            verify(TestServices.videoPlayer.muted);
            compare(button.icon.name, "audio-volume-muted");

            mouseClick(button);
            verify(!TestServices.videoPlayer.muted);
        }

        function test_showsTheFrameUnderThePointer() {
            tryCompare(TestServices.accountConnection, "state", Connection.State.LoggedIn);
            TestServices.openSampleVideo();
            tryVerify(() => TestServices.previewPlayer.hasPreviews);
            mouseMove(root, root.width / 2, root.height - 1);  // not where the bar will be
            const controls = createTemporaryObject(controlsComponent, root, {videoPlayer: TestServices.previewPlayer});
            waitForRendering(controls);
            const scrubBar = findChild(controls, "scrubBar");
            const preview = findChild(controls, "scrubPreview");
            verify(!preview.visible);

            // The middle: 111.2 s, frame 50 (row 5, column 0)
            mouseMove(scrubBar, scrubBar.leftPadding + scrubBar.availableWidth / 2, scrubBar.height / 2);

            tryVerify(() => preview.visible);
            const sprite = findChild(controls, "previewSprite");
            compare(sprite.x, 0);
            compare(sprite.y, -450);
            compare(findChild(controls, "previewTime").text, "1:51");
            verify(preview.y + preview.height <= 0, "above the bar");
            const middle = scrubBar.mapToItem(controls, scrubBar.leftPadding + scrubBar.availableWidth / 2, 0).x;
            fuzzyCompare(preview.x + preview.width / 2, middle, 1);

            mouseMove(root, root.width / 2, root.height - 1);  // away
            tryVerify(() => !preview.visible);
        }

        // Without a storyboard, e.g. one the server could not make, the bar works without previews.
        function test_showsNoPreviewWithoutAStoryboard() {
            const controls = shownControls();
            const scrubBar = findChild(controls, "scrubBar");

            mouseMove(scrubBar, scrubBar.width / 2, scrubBar.height / 2);
            wait(50);

            verify(!findChild(controls, "scrubPreview").visible);
        }

        // Turning the volume up means to hear it.
        function test_turningTheVolumeUpUnmutes() {
            const controls = shownControls();
            TestServices.videoPlayer.muted = true;
            const slider = findChild(controls, "volumeSlider");

            mouseClick(slider, slider.width * 0.6, slider.height / 2);

            verify(!TestServices.videoPlayer.muted);
            fuzzyCompare(TestServices.videoPlayer.volume, 0.6, 0.15);
        }
    }
}
