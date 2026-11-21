/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "utils/FileHash.hpp"

#include <QCryptographicHash>
#include <QFile>

namespace photobook::utils
{

std::optional<QString> sha256Of(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return std::nullopt;
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
    {
        return std::nullopt;
    }

    return QString::fromLatin1(hash.result().toHex());
}

}  // namespace photobook::utils
