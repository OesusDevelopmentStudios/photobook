/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/ServerErrors.hpp"

#include <KLocalizedString>

namespace photobook::library
{

QString describeError(const server_client::ApiError& error)
{
    if (error.code == server_client::networkErrorCode)
    {
        return i18nc("@info %1 is the reason, e.g. Connection refused", "The server does not answer: %1", error.detail);
    }

    if (error.detail.isEmpty())
    {
        return error.title;
    }

    return i18nc("@info %1 is a title, %2 an explanation", "%1. %2", error.title, error.detail);
}

}  // namespace photobook::library
