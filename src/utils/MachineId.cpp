/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "utils/MachineId.hpp"

#include <QCryptographicHash>
#include <QFile>
#include <QMessageAuthenticationCode>

namespace pictorio::utils
{

QString appSpecificMachineId(const QString& machineId)
{
    const QByteArray key = machineId.trimmed().toLatin1();
    return QString::fromLatin1(
        QMessageAuthenticationCode::hash("pictorio-db", key, QCryptographicHash::Sha256).toHex());
}

QString thisComputersAppSpecificId(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return {};
    }

    const QString machineId = QString::fromLatin1(file.readAll()).trimmed();
    return machineId.isEmpty() ? QString() : appSpecificMachineId(machineId);
}

}  // namespace pictorio::utils
