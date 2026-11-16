/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/TagMessages.hpp"

#include <KLocalizedString>
#include <QJsonArray>

namespace pictorio::tag_editor
{

QString restrictedMessage(const QStringList& tags)
{
    return i18nc("@info %1 is a list of tags",
                 "An admin has restricted these tags for your account: %1",
                 tags.join(QStringLiteral(", ")));
}

QString describeError(const server_client::ApiError& error)
{
    if (error.code == QLatin1String("tag_restricted"))
    {
        QStringList tags;
        for (const QJsonValue& tag : error.problem.value(QStringLiteral("tags")).toArray())
        {
            tags.append(tag.toString());
        }

        return restrictedMessage(tags);
    }

    if (error.code == server_client::networkErrorCode)
    {
        return i18nc("@info %1 is the reason, e.g. Connection refused", "The server does not answer: %1", error.detail);
    }

    return error.detail.isEmpty()
               ? error.title
               : i18nc("@info %1 is a title, %2 an explanation", "%1. %2", error.title, error.detail);
}

}  // namespace pictorio::tag_editor
