/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QString>
#include <QStringList>

#include "ApiTypes.hpp"

namespace photobook::tag_editor
{

// The same words whether the app or the server refuses a restricted tag
QString restrictedMessage(const QStringList& tags);

// The server's refusal of a tag change as a sentence for the user
QString describeError(const server_client::ApiError& error);

}  // namespace photobook::tag_editor
