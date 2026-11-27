// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick
import QtTest
import io.github.oesusdevelopmentstudios.photobook
import photobook.tests

// A video in a slideshow plays exactly while the slideshow does.
Item {
    id: root

    TestCase {
        name: "SlideshowVideo"

        Component {
            id: linkComponent

            SlideshowVideo {
                slideshow: TestServices.slideshow
                showsVideo: true
            }
        }

        SignalSpy {
            id: playSpy

            signalName: "playRequested"
        }

        SignalSpy {
            id: pauseSpy

            signalName: "pauseRequested"
        }

        function linked(showsVideo = true): SlideshowVideo {
            const link = createTemporaryObject(linkComponent, root, {showsVideo: showsVideo});
            for (const spy of [playSpy, pauseSpy]) {
                spy.clear();
                spy.target = link;
            }
            return link;
        }

        function cleanup() {
            TestServices.slideshow.stop();
        }

        // A video paused before the slideshow began once stayed paused while the slideshow ran, and Space then
        // paused the one and played the other.
        function test_startingTheSlideshowPlaysThePausedVideo() {
            const link = linked();

            TestServices.slideshow.start(Slideshow.Order.ByName);

            compare(playSpy.count, 1);
            compare(pauseSpy.count, 0);
            verify(link.videoMayPlay);
        }

        function test_pausesAndResumesWithTheSlideshow() {
            const link = linked();
            TestServices.slideshow.start(Slideshow.Order.ByName);

            TestServices.slideshow.pause();
            compare(pauseSpy.count, 1);
            verify(!link.videoMayPlay);  // a video reached now waits

            TestServices.slideshow.resume();
            compare(playSpy.count, 2);
            verify(link.videoMayPlay);
        }

        function test_leavesPhotosAndTheVideoOutsideASlideshowAlone() {
            const link = linked(false);
            TestServices.slideshow.start(Slideshow.Order.ByName);
            TestServices.slideshow.pause();

            link.showsVideo = true;
            TestServices.slideshow.stop();

            compare(playSpy.count, 0);
            compare(pauseSpy.count, 0);
            verify(link.videoMayPlay);  // without a slideshow, a video plays as it opens
        }
    }
}
