/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "BulkTagEditor.hpp"
#include "HiddenFilterEditor.hpp"
#include "SearchSuggestions.hpp"
#include "TagEditor.hpp"
#include "TagPaneLayout.hpp"
#include "UploadTagEditor.hpp"

namespace photobook::tag_editor
{

// The connection is not owned and must outlive the editor.
std::unique_ptr<TagEditor> createTagEditor(connection::Connection* connection);

// The connection is not owned and must outlive the editor.
std::unique_ptr<BulkTagEditor> createBulkTagEditor(connection::Connection* connection);

// The connection is not owned and must outlive the editor.
std::unique_ptr<UploadTagEditor> createUploadTagEditor(connection::Connection* connection);

// The connection is not owned and must outlive the editor.
std::unique_ptr<HiddenFilterEditor> createHiddenFilterEditor(connection::Connection* connection);

// The connection is not owned and must outlive the suggestions.
std::unique_ptr<SearchSuggestions> createSearchSuggestions(connection::Connection* connection);

// The settings are not owned and must outlive the layout.
std::unique_ptr<TagPaneLayout> createTagPaneLayout(settings::Settings* settings);

}  // namespace photobook::tag_editor
