# Contributing to Photobook

Both AI and human made submissions are welcome. It is not required to specifically mark AI submissions but all need to followe the rules described below.

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

Fedora:
```sh
sudo dnf install cmake gcc-c++ extra-cmake-modules \
    qt6-qtbase-devel qt6-qtdeclarative-devel \
    kf6-kirigami-devel kf6-ki18n-devel kf6-kcoreaddons-devel kf6-kconfig-devel kf6-kio-devel \
    exiv2-devel qtkeychain-qt6-devel \
    kf6-kirigami-addons kf6-qqc2-desktop-style qt6-qtimageformats kf6-kimageformats
```

Ubuntu:
```sh
sudo apt install cmake g++ extra-cmake-modules \
    qt6-base-dev qt6-declarative-dev \
    libkirigami-dev libkf6i18n-dev libkf6coreaddons-dev libkf6config-dev libkf6kio-dev \
    libexiv2-dev qt6keychain-dev \
    qml6-module-org-kde-kirigami qml6-module-org-kde-kirigamiaddons-components \
    qml6-module-org-kde-desktop qt6-image-formats-plugins kimageformat6-plugins \
    qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-dialogs \
    qml6-module-qtquick-layouts qml6-module-qtquick-shapes qml6-module-qtqml-workerscript \
    qml6-module-qttest
```

Arch:
```sh
sudo pacman -S --needed cmake base-devel extra-cmake-modules \
    qt6-base qt6-declarative \
    kirigami ki18n kcoreaddons kconfig kio \
    exiv2 qtkeychain-qt6 \
    kirigami-addons qqc2-desktop-style qt6-imageformats kimageformats
```

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

## Workflow

- **Small commits.** Each commit does one thing, builds, passes the tests and can be reviewed on its
  own.
- **Tests come with the code.** New logic comes with a unit test in `tests/`.
- Before committing:
  ```sh
  clang-format -i <changed C++ files>
  cmake --build build && ctest --test-dir build --output-on-failure
  cmake --build build --target photobook_ui_qmllint
  ```

## Code layout

```
src/
├── main.cpp            starts the application; connects the pieces, contains no logic
├── app_manager/        the primary manager: creates the services and makes them available to QML
├── services/           one folder per service, each with a single responsibility
├── utils/              small helpers shared by several services
└── qml/                user interface (presentation only)
packaging/              the desktop file, AppStream metainfo, icon and Flatpak manifest
tests/                  mirrors src/: tests/utils/TestAppIdentity.cpp tests src/utils/AppIdentity.cpp
├── fakes/              stand-ins for the server, keyring, trash and file manager, shared by the tests
├── packaging/          checks of the files in packaging/
├── qml/                QML tests, tst_<component>.qml, with C++ fakes such as FakeTagRowsEditor
└── e2e/                end-to-end tests against a real photobook-db server, TestEndToEnd<flow>.cpp
```

**QML contains presentation only.** Decisions (sorting, paging, what to upload, when to delete) live in
C++ services, where they can be unit-tested. What the QML itself does (a click reaching the right call,
a part showing the right state) is tested in `tests/qml/` with Qt Quick Test. The tests load the
application's QML module and run offscreen as part of `ctest`. Put the item under test in the file's
visible root item, not in the `TestCase`, which is invisible: an invisible item gets no clicks.

**End-to-end tests** (`tests/e2e/`) run the app's services against a real photobook-db server. A plain
`ctest` skips them. `tests/e2e/run_end_to_end_tests.py` installs the server from a photobook-db checkout
next to this repository into a temporary virtual environment, starts it on a temporary folder, creates
the test accounts, runs the tests and removes everything again:

```sh
tests/e2e/run_end_to_end_tests.py                    # needs ../photobook-db and a build in build/
tests/e2e/run_end_to_end_tests.py --server ../photobook-db/.venv/bin/photobook-db   # faster: no install
```

**CI** (`.github/workflows/ci.yml`) builds and runs all tests, the end-to-end ones too, inside the KDE SDK
the Flatpak is built with, against photobook-db at a fixed commit. The same runs locally, into `build-sdk/`:

```sh
tests/run_in_flatpak_sdk.sh    # needs flatpak-builder, the KDE SDK and ../photobook-db
```

### Service layout

```
services/<service_name>/                    folder: snake_case, e.g. image_loader/
├── include/
│   ├── <ServiceName>.hpp                   public interface: the only header other code may include
│   └── <ServiceName>Factory.hpp            creates the service
├── impl/                                   implementation files (.cpp/.hpp), e.g. QtImageLoader.hpp
├── hlp/                                    helpers used only by this service (.cpp/.hpp)
├── <ServiceName>.cpp
└── <ServiceName>Factory.cpp
```

- Code lives in `namespace photobook::<service_name>`.
- Services never create each other. The app manager creates them through their factories and passes
  in their dependencies. Tests can then pass in fakes.
- Each service is built as a static library, so the application and the tests link the same code.

## C++ style

Formatting is enforced by [.clang-format](.clang-format): Allman braces, 4 spaces, at most 120
columns, one argument per line when a call does not fit on one line.

### Naming

| What | Style | Example |
|---|---|---|
| Classes, structs, enums | `PascalCase` | `FolderScanner` |
| Functions, variables | `camelCase` | `registerApplicationIdentity()` |
| Private member variables | `m_` + `camelCase` | `m_currentIndex` |
| Public fields of plain structs | `camelCase` | `ImageInfo::displaySize` |
| C++ files | `PascalCase`, usually named after the main class (exception: `main.cpp`) | `QtImageLoader.hpp`, `AppIdentity.cpp` |
| Folders | `snake_case` | `services/image_loader/` |
| Namespaces | `photobook::snake_case` | `photobook::folder_scanner` |
| Test files | `Test<file under test>.cpp` | `TestAppIdentity.cpp` |

- **Names are verbose.** No single-letter names, not even for loop counters: `imageIndex`, not `i`.
- **Each function has a single responsibility.**

### Empty line after a closing brace

A closing brace `}` is always followed by an empty line, unless the next line is another closing brace,
`else` or `catch`:

```cpp
if (!fileInfo.exists())
{
    return ImageLoadError::FileNotFound;
}

if (!fileInfo.isFile())
{
    return ImageLoadError::NotAFile;
}

return std::nullopt;
```

clang-format enforces this only between function and class definitions; inside functions it is checked
in review.

An empty function body stays on one line: `{}`.

```cpp
Viewer::Viewer(std::unique_ptr<image_loader::ImageLoader> imageLoader, QObject* parent)
    : QObject(parent)
    , m_imageLoader(std::move(imageLoader))
{}
```

### `.cpp` file layout

```cpp
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "OwnHeader.hpp"           // 1. this file's own header

#include <vector>                  // 2. standard library

#include <QString>                 // 3. external libraries (Qt, KDE Frameworks, ...)

#include "utils/AppIdentity.hpp"   // 4. common helpers

#include "impl/LocalFile.hpp"      // 5. local files of this service

namespace
{
// constants and helper functions used only in this file
}  // namespace

namespace photobook::service_name
{
// service functions
}  // namespace photobook::service_name
```

Groups are separated by a blank line; clang-format sorts includes only within a group.

### `.hpp` file layout

```cpp
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

namespace photobook::service_name
{

class Example
{
    // structs / enums used only by this class, then type aliases

public:
    Example();
    ~Example();

    // public functions

private:
    // private functions

    // member variables
};

}  // namespace photobook::service_name
```

## QML style

QML uses the usual QML brace style (`Item {` on the same line) and 4 spaces. User-visible strings go
through `KI18n.i18nc("@context", "Text")` (`import org.kde.ki18n`), so translators get context and
qmllint can check the calls.

## License headers

Every source file starts with an SPDX header:

| File type | Header |
|---|---|
| C++ | `/* SPDX-License-Identifier: GPL-3.0-or-later ... */` block, as above |
| QML | `// SPDX-License-Identifier: GPL-3.0-or-later` + `// SPDX-FileCopyrightText: ...` |
| CMake, YAML, `.clang-format` | `# SPDX-License-Identifier: GPL-3.0-or-later` + `# SPDX-FileCopyrightText: ...` |
