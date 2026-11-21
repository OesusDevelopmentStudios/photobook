/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QString>

namespace photobook::utils
{

inline const QString machineIdFile = QStringLiteral("/etc/machine-id");  // the host's, also in the Flatpak

// What the app tells a photobook-db server to show that both run on the same computer: an HMAC-SHA256 of
// "photobook-db" keyed with the machine ID as written in its file, in hex. The machine ID itself never leaves
// the computer, and the server computes the same from its own.
QString appSpecificMachineId(const QString& machineId);

// The same for this computer, read from the file; empty if it cannot be read, e.g. in a container without one.
QString thisComputersAppSpecificId(const QString& filePath = machineIdFile);

}  // namespace photobook::utils
