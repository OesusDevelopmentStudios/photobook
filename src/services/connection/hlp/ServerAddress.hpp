/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QList>
#include <QString>
#include <QUrl>

namespace pictorio::connection
{

// The URLs a typed server address may mean, in the order to try them. With http:// or https:// that one
// URL; without, https first and plain http second, like a browser. Empty if the text is no address.
QList<QUrl> serverUrlCandidates(const QString& typedAddress);

// The device name the server lists for this login, e.g. "laptop (Pictorio 0.1.0)".
QString deviceName();

QString describeServerError(const QString& code, const QString& title, const QString& detail);

}  // namespace pictorio::connection
