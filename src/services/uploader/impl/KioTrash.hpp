/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include "Trash.hpp"

namespace pictorio::uploader
{

class KioTrash : public Trash
{
public:
    QFuture<std::expected<void, QString>> moveToTrash(const QString& filePath) override;
};

}  // namespace pictorio::uploader
