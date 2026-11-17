/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/KioTrash.hpp"

#include <KIO/CopyJob>
#include <QUrl>

namespace pictorio::uploader
{

QFuture<std::expected<void, QString>> KioTrash::moveToTrash(const QString& filePath)
{
    KIO::CopyJob* job = KIO::trash(QUrl::fromLocalFile(filePath), KIO::HideProgressInfo);
    // The job deletes itself after its result signal, so the error is read while it still exists.
    return QtFuture::connect(job, &KJob::result).then([](KJob* finishedJob) -> std::expected<void, QString> {
        if (finishedJob->error() != 0)
        {
            return std::unexpected(finishedJob->errorString());
        }

        return {};
    });
}

}  // namespace pictorio::uploader
