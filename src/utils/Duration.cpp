/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "Duration.hpp"

#include <algorithm>

namespace photobook::utils
{

QString durationText(qint64 milliseconds)
{
    const qint64 seconds = std::max<qint64>(milliseconds, 0) / 1000;
    const qint64 hours = seconds / 3600;
    const qint64 minutes = seconds / 60 % 60;
    const QString secondsText = QStringLiteral("%1").arg(seconds % 60, 2, 10, QLatin1Char('0'));
    if (hours > 0)
    {
        return QStringLiteral("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QLatin1Char('0')).arg(secondsText);
    }

    return QStringLiteral("%1:%2").arg(minutes).arg(secondsText);
}

}  // namespace photobook::utils
