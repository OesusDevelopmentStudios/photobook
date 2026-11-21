/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QStringList>

#include "Trash.hpp"

namespace pictorio::fakes
{

// The files sent to the trash, and whether the trash takes them. Shared, so a test can look inside after
// handing the trash over.
struct TrashCan
{
    QStringList files;
    bool works = true;
};

class FakeTrash : public uploader::Trash
{
public:
    explicit FakeTrash(std::shared_ptr<TrashCan> can)
        : m_can(std::move(can))
    {}

    QFuture<std::expected<void, QString>> moveToTrash(const QString& filePath) override
    {
        if (!m_can->works)
        {
            return QtFuture::makeReadyValueFuture(
                std::expected<void, QString>(std::unexpected(QStringLiteral("No trash here"))));
        }

        m_can->files.append(filePath);
        return QtFuture::makeReadyValueFuture(std::expected<void, QString>());
    }

private:
    std::shared_ptr<TrashCan> m_can;
};

}  // namespace pictorio::fakes
