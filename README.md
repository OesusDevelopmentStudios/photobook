# Photobook

[![CI](https://github.com/OesusDevelopmentStudios/photobook/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/OesusDevelopmentStudios/photobook/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/tag/OesusDevelopmentStudios/photobook?sort=semver&label=release)](https://github.com/OesusDevelopmentStudios/photobook/releases)
[![License: GPL-3.0](https://img.shields.io/github/license/OesusDevelopmentStudios/photobook)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue?logo=cplusplus)](CMakeLists.txt)
[![Qt](https://img.shields.io/badge/Qt-6.8-41CD52?logo=qt)](CMakeLists.txt)
[![KDE Frameworks](https://img.shields.io/badge/KDE_Frameworks-6.21-1D99F3?logo=kde)](CMakeLists.txt)

Manage your images in an organised way.

Photobook is an image viewer and photo library for KDE Plasma.

- **On its own** it is a simple image viewer: open an image from Dolphin, zoom, rotate, step through
  the folder, run a slideshow.
- **Connected to a [photobook-db](https://github.com/OesusDevelopmentStudios/photobook-db) server**
  it becomes a tag-based photo library: search by tags, browse a thumbnail grid, edit tags, upload
  photos. To set up a server, see
  [photobook-db's install guide](https://github.com/OesusDevelopmentStudios/photobook-db#install).

## Building

### Dependencies

| Dependency | Minimum version |
|---|---|
| CMake | 3.25 |
| C++23 compiler (GCC or Clang) | |
| Qt (Core, Concurrent, DBus, Gui, Network, Qml, Quick, QuickControls2, Widgets; Test and QuickTest for the tests) | 6.8 |
| KDE Frameworks (Extra CMake Modules, Kirigami, KI18n, KCoreAddons, KConfig, KIO) | 6.21 |
| exiv2 | 0.28 |
| QtKeychain (Qt 6 build) | 0.17 |
| Kirigami Addons, needed at run time | 1.7 |
| KDE desktop style for Qt Quick (qqc2-desktop-style), needed at run time | |
| Qt and KDE image format plugins (qtimageformats, kimageformats), needed at run time for WebP, TIFF, AVIF, HEIF and JPEG XL | |

On Fedora:

```sh
sudo dnf install cmake gcc-c++ extra-cmake-modules \
    qt6-qtbase-devel qt6-qtdeclarative-devel \
    kf6-kirigami-devel kf6-ki18n-devel kf6-kcoreaddons-devel kf6-kconfig-devel kf6-kio-devel \
    exiv2-devel qtkeychain-qt6-devel \
    kf6-kirigami-addons kf6-qqc2-desktop-style qt6-qtimageformats kf6-kimageformats
```

HEIF images also need an HEVC decoder for libheif, which Fedora does not ship. The Flatpak build
opens them.

### Build and run

```sh
cmake -B build
cmake --build build
./build/bin/photobook              # or with an image: ./build/bin/photobook beach.jpg
```

Build options:

| Option | Default | Meaning |
|---|---|---|
| `BUILD_TESTING` | `ON` | Build the tests |
| `PHOTOBOOK_WARNINGS_AS_ERRORS` | `ON` | Treat compiler warnings as errors. Turn off for packaging |

Example: `cmake -B build -DBUILD_TESTING=OFF -DPHOTOBOOK_WARNINGS_AS_ERRORS=OFF`

### Flatpak

Needs `flatpak-builder` and the KDE runtime from Flathub
(`flatpak install flathub org.kde.Sdk//6.11 org.kde.Platform//6.11`). From the repository root:

```sh
flatpak-builder --user --install --force-clean build-flatpak \
    packaging/flatpak/io.github.oesusdevelopmentstudios.photobook.yml
flatpak run io.github.oesusdevelopmentstudios.photobook
```

This builds the current source tree, including uncommitted changes.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the code layout, the coding style and how to run the tests.

## License

Photobook is free software, licensed under the
[GNU General Public License, version 3 or later](LICENSE) (`GPL-3.0-or-later`).
