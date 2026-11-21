/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include "Trash.hpp"

namespace photobook::uploader
{

class KioTrash : public Trash
{
public:
    QFuture<std::expected<void, QString>> moveToTrash(const QString& filePath) override;
};

}  // namespace photobook::uploader
