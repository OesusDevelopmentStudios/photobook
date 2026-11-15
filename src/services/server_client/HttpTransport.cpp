/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "HttpTransport.hpp"

namespace pictorio::server_client
{

QByteArray HttpResponse::header(const QByteArray& name) const
{
    for (const auto& [headerName, value] : headers)
    {
        if (headerName.compare(name, Qt::CaseInsensitive) == 0)
        {
            return value;
        }
    }

    return {};
}

HttpTransport::~HttpTransport() = default;

}  // namespace pictorio::server_client
