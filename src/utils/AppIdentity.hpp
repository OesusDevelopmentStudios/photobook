/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QByteArray>
#include <QString>

namespace pictorio::utils
{

// Reverse-DNS application ID. Used as the Flatpak app ID, the .desktop file name (the window manager
// uses it to find the app icon) and the URI of the user interface's QML module.
inline const QString applicationId = QStringLiteral("io.github.oesusdevelopmentstudios.pictorio");

// Short internal name: translation catalogue (pictorio.mo) and config file (pictoriorc).
inline const QString applicationComponentName = QStringLiteral("pictorio");

// Used for D-Bus names. KAboutData defaults to kde.org, which would be wrong for a project outside KDE.
inline const QByteArray organizationDomain = QByteArrayLiteral("oesusdevelopmentstudios.github.io");

// Call once, right after the QCoreApplication is created and before any i18n() call.
void registerApplicationIdentity();

}  // namespace pictorio::utils
