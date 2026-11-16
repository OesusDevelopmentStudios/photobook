/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "BulkTagEditor.hpp"
#include "TagEditor.hpp"
#include "TagSuggestions.hpp"
#include "UploadTagEditor.hpp"

namespace pictorio::tag_editor
{

// The connection is not owned and must outlive the editor.
std::unique_ptr<TagEditor> createTagEditor(connection::Connection* connection);

// The connection is not owned and must outlive the editor.
std::unique_ptr<BulkTagEditor> createBulkTagEditor(connection::Connection* connection);

// The connection is not owned and must outlive the editor.
std::unique_ptr<UploadTagEditor> createUploadTagEditor(connection::Connection* connection);

// The connection is not owned and must outlive the suggestions.
std::unique_ptr<TagSuggestions> createTagSuggestions(connection::Connection* connection);

}  // namespace pictorio::tag_editor
