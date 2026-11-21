/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/KioFileManager.hpp"

#include <KIO/OpenFileManagerWindowJob>
#include <QUrl>

namespace photobook::image_actions
{

QFuture<std::expected<void, QString>> KioFileManager::showFile(const QString& filePath)
{
    KIO::OpenFileManagerWindowJob* job = KIO::highlightInFileManager({QUrl::fromLocalFile(filePath)});
    // The job deletes itself after its result signal, so the error is read while it still exists.
    return QtFuture::connect(job, &KJob::result).then([](KJob* finishedJob) -> std::expected<void, QString> {
        if (finishedJob->error() != 0)
        {
            return std::unexpected(finishedJob->errorString());
        }

        return {};
    });
}

}  // namespace photobook::image_actions
