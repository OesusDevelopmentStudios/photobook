/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QString>
#include <QStringList>
#include <QtTypes>

namespace photobook::folder_scanner
{

enum class SortOrder
{
    Name,    // natural order: "img2" before "img10", case ignored
    Date,    // oldest first: when the photo was taken, else the file modification time; ties by name
    Random,  // shuffled; the same seed always gives the same order
};

struct Sorting
{
    SortOrder order = SortOrder::Name;
    quint32 randomSeed = 0;  // used only by SortOrder::Random

    friend bool operator==(const Sorting&, const Sorting&) = default;
};

// Thread-safe, so it can run in the background
class FolderScanner
{
public:
    virtual ~FolderScanner();

    // Absolute paths, not in subfolders. Images are recognized by their file extension; hidden files are
    // skipped. Empty if the folder does not exist.
    virtual QStringList listImages(const QString& folderPath, const Sorting& sorting) const = 0;
};

// The images in a folder to upload: those directly in it, and with includeSubfolders also those in every folder
// below it. Recognized as listImages() does; hidden files and folders are skipped, and links are not followed
// (as the server's folder import). In the natural order of their paths. Empty if the folder does not exist.
// Reads only the file system: thread-safe.
QStringList findImages(const QString& folderPath, bool includeSubfolders);

}  // namespace photobook::folder_scanner
