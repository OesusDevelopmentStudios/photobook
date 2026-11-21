/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/QtFolderScanner.hpp"

#include <algorithm>
#include <numeric>

#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSet>
#include <QtConcurrentMap>

#include "hlp/ImageFiles.hpp"

namespace
{

// Fisher-Yates with Qt's generator: std::shuffle may give a different order for the same seed
// depending on the standard library.
void shuffle(QStringList& filePaths, quint32 seed)
{
    QRandomGenerator generator(seed);
    for (qsizetype lastIndex = filePaths.size() - 1; lastIndex > 0; --lastIndex)
    {
        const auto swapIndex = static_cast<qsizetype>(generator.bounded(static_cast<quint64>(lastIndex) + 1));
        filePaths.swapItemsAt(lastIndex, swapIndex);
    }
}

}  // namespace

namespace photobook::folder_scanner
{

QtFolderScanner::QtFolderScanner(std::unique_ptr<const image_loader::ImageLoader> imageLoader)
    : m_imageLoader(std::move(imageLoader))
{}

QStringList QtFolderScanner::listImages(const QString& folderPath, const Sorting& sorting) const
{
    const QDir folder(folderPath);
    const QFileInfoList entries = folder.entryInfoList(QDir::Files | QDir::Readable);
    const QSet<QString> readableMimeTypes = readableImageMimeTypes();

    QStringList imagePaths;
    for (const QFileInfo& entry : entries)
    {
        if (isReadableImage(entry, readableMimeTypes))
        {
            imagePaths.append(entry.absoluteFilePath());
        }
    }

    // Name order first: date ties keep it, and the shuffle starts from the same order every time. The paths
    // share their folder, so their natural order is that of the file names.
    sortNaturally(imagePaths);
    switch (sorting.order)
    {
    case SortOrder::Name:
        break;
    case SortOrder::Date:
        sortByDate(imagePaths);
        break;
    case SortOrder::Random:
        shuffle(imagePaths, sorting.randomSeed);
        break;
    }

    return imagePaths;
}

void QtFolderScanner::sortByDate(QStringList& imagePaths) const
{
    // Reading the dates is the slow part (it opens every file), so it runs in parallel.
    const QList<QDateTime> dates = QtConcurrent::blockingMapped(imagePaths, [this](const QString& imagePath) {
        const QDateTime captureDate = m_imageLoader->readCaptureDate(imagePath);
        return captureDate.isValid() ? captureDate : QFileInfo(imagePath).lastModified();
    });

    QList<qsizetype> order(imagePaths.size());
    std::iota(order.begin(), order.end(), 0);
    std::ranges::stable_sort(order, [&](qsizetype left, qsizetype right) { return dates.at(left) < dates.at(right); });

    QStringList sortedPaths;
    sortedPaths.reserve(imagePaths.size());
    for (const qsizetype index : order)
    {
        sortedPaths.append(imagePaths.at(index));
    }

    imagePaths = std::move(sortedPaths);
}

}  // namespace photobook::folder_scanner
