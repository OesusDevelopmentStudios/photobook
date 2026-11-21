/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QStringList>

#include "FileManager.hpp"

namespace pictorio::fakes
{

// The files shown, and whether the file manager opens. Shared, so a test can look inside after handing the
// file manager over.
struct FileManagerWindows
{
    QStringList shownFiles;
    bool opens = true;
};

class FakeFileManager : public image_actions::FileManager
{
public:
    explicit FakeFileManager(std::shared_ptr<FileManagerWindows> windows)
        : m_windows(std::move(windows))
    {}

    QFuture<std::expected<void, QString>> showFile(const QString& filePath) override
    {
        if (!m_windows->opens)
        {
            return QtFuture::makeReadyValueFuture(
                std::expected<void, QString>(std::unexpected(QStringLiteral("No file manager here"))));
        }

        m_windows->shownFiles.append(filePath);
        return QtFuture::makeReadyValueFuture(std::expected<void, QString>());
    }

private:
    std::shared_ptr<FileManagerWindows> m_windows;
};

}  // namespace pictorio::fakes
