/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "FolderScanner.hpp"

#include <QDir>
#include <QFileInfo>

#include "hlp/ImageFiles.hpp"

namespace
{

using pictorio::folder_scanner::isReadableImage;

// Without QDir::Hidden, hidden files and folders are left out; QDir::NoSymLinks leaves out links.
void collectImages(const QString& folderPath,
                   bool includeSubfolders,
                   const QSet<QString>& readableMimeTypes,
                   QStringList& imagePaths)
{
    const QDir folder(folderPath);
    for (const QFileInfo& entry : folder.entryInfoList(QDir::Files | QDir::Readable | QDir::NoSymLinks))
    {
        if (isReadableImage(entry, readableMimeTypes))
        {
            imagePaths.append(entry.absoluteFilePath());
        }
    }

    if (includeSubfolders)
    {
        for (const QFileInfo& subfolder : folder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks))
        {
            collectImages(subfolder.absoluteFilePath(), true, readableMimeTypes, imagePaths);
        }
    }
}

}  // namespace

namespace pictorio::folder_scanner
{

FolderScanner::~FolderScanner() = default;

QStringList findImages(const QString& folderPath, bool includeSubfolders)
{
    QStringList imagePaths;
    collectImages(folderPath, includeSubfolders, readableImageMimeTypes(), imagePaths);
    sortNaturally(imagePaths);
    return imagePaths;
}

}  // namespace pictorio::folder_scanner
