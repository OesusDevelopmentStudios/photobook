/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QFileInfo>
#include <QSet>
#include <QString>
#include <QStringList>

namespace photobook::folder_scanner
{

// Image types Qt can read. Only image/* types: Qt can also read e.g. PDF pages, which are not images.
QSet<QString> readableImageMimeTypes();

// By extension only: reading the content of every file would be slow in large folders.
bool isReadableImage(const QFileInfo& fileInfo, const QSet<QString>& readableMimeTypes);

// Natural order, case ignored: "img2" before "img10"
void sortNaturally(QStringList& filePaths);

}  // namespace photobook::folder_scanner
