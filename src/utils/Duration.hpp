/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QString>

namespace photobook::utils
{

// A video's length as players show it: "0:07", "3:42", "1:02:05". Whole seconds, rounded down; never negative.
QString durationText(qint64 milliseconds);

}  // namespace photobook::utils
