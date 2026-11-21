/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/ImageFiles.hpp"

#include <algorithm>

#include <QCollator>
#include <QImageReader>
#include <QMimeDatabase>

namespace photobook::folder_scanner
{

QSet<QString> readableImageMimeTypes()
{
    QSet<QString> mimeTypes;
    for (const QByteArray& mimeType : QImageReader::supportedMimeTypes())
    {
        if (mimeType.startsWith("image/"))
        {
            mimeTypes.insert(QString::fromLatin1(mimeType));
        }
    }

    return mimeTypes;
}

bool isReadableImage(const QFileInfo& fileInfo, const QSet<QString>& readableMimeTypes)
{
    static const QMimeDatabase mimeDatabase;
    const QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo, QMimeDatabase::MatchExtension);
    if (readableMimeTypes.contains(mimeType.name()))
    {
        return true;
    }

    const QStringList aliases = mimeType.aliases();
    return std::ranges::any_of(aliases, [&](const QString& alias) { return readableMimeTypes.contains(alias); });
}

void sortNaturally(QStringList& filePaths)
{
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::ranges::sort(filePaths,
                      [&](const QString& left, const QString& right) { return collator.compare(left, right) < 0; });
}

}  // namespace photobook::folder_scanner
