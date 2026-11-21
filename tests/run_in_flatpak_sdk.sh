#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
#
# Builds photobook and runs all its tests, the end-to-end tests too, inside the KDE SDK that the Flatpak is
# built with: the same compiler, Qt and KDE Frameworks as the Flatpak. CI runs it; it works locally too.
#
#   tests/run_in_flatpak_sdk.sh [work folder]    # default: build-sdk/ in this repository
#
# Needs flatpak-builder, the SDK the manifest names, and a photobook-db checkout next to this repository, or
# where PHOTOBOOK_DB_SOURCE says.

set -eu

repository=$(cd "$(dirname "$0")/.." && pwd)
manifest="$repository/packaging/flatpak/io.github.oesusdevelopmentstudios.photobook.yml"
work="${1:-$repository/build-sdk}"
mkdir -p "$work"
work=$(cd "$work" && pwd)
photobook_db=$(cd "${PHOTOBOOK_DB_SOURCE:-$repository/../photobook-db}" && pwd)

# The libraries the SDK lacks (exiv2, QtKeychain), built from the Flatpak manifest; photobook itself is built
# below, with its tests, which the manifest leaves out. Without rofiles-fuse, which needs FUSE, as containers
# such as CI's lack it; it only guards flatpak-builder's cache.
flatpak-builder --force-clean --disable-rofiles-fuse --stop-at=photobook --state-dir="$work/state" \
    "$work/app" "$manifest"

# A command in the SDK's sandbox, with those libraries in /app. It sees only the folders named here; the
# network is for installing and reaching the test server. flatpak build, not flatpak-builder --run, which
# would need rofiles-fuse again.
in_sdk() {
    flatpak build --die-with-parent \
        --filesystem="$repository" --filesystem="$photobook_db" --filesystem="$work" --share=network \
        "$work/app" "$@"
}

in_sdk cmake -S "$repository" -B "$work/build" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/app
in_sdk cmake --build "$work/build"
in_sdk ctest --test-dir "$work/build" --output-on-failure
in_sdk cmake --build "$work/build" --target photobook_ui_qmllint

# The server runs in the same sandbox, so the tests reach it and get its address without anything passed in.
in_sdk python3 "$repository/tests/e2e/run_end_to_end_tests.py" \
    --photobook-db "$photobook_db" --build-dir "$work/build" --temp-parent "$work"
