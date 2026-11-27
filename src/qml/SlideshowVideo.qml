// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios

import QtQuick

// While a slideshow shows a video, the video plays exactly while the slideshow does: started or resumed, it plays,
// also a video paused before the slideshow began; paused, it pauses. So Space moves both the same way.
QtObject {
    id: link

    required property Slideshow slideshow
    property bool showsVideo: false

    // In a running slideshow: whether a video on screen should play now
    readonly property bool videoMayPlay: !slideshow.running || !slideshow.paused

    signal playRequested
    signal pauseRequested

    function follow(): void {
        if (link.slideshow.running && link.showsVideo) {
            link.slideshow.paused ? link.pauseRequested() : link.playRequested();
        }
    }

    readonly property Connections slideshowState: Connections {
        target: link.slideshow

        function onStateChanged(): void {
            link.follow();
        }
    }
}
