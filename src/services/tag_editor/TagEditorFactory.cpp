/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TagEditorFactory.hpp"

namespace photobook::tag_editor
{

std::unique_ptr<TagEditor> createTagEditor(connection::Connection* connection)
{
    return std::make_unique<TagEditor>(connection);
}

std::unique_ptr<BulkTagEditor> createBulkTagEditor(connection::Connection* connection)
{
    return std::make_unique<BulkTagEditor>(connection);
}

std::unique_ptr<UploadTagEditor> createUploadTagEditor(connection::Connection* connection)
{
    return std::make_unique<UploadTagEditor>(connection);
}

std::unique_ptr<HiddenFilterEditor> createHiddenFilterEditor(connection::Connection* connection)
{
    return std::make_unique<HiddenFilterEditor>(connection);
}

std::unique_ptr<SearchSuggestions> createSearchSuggestions(connection::Connection* connection)
{
    return std::make_unique<SearchSuggestions>(connection);
}

std::unique_ptr<TagPaneLayout> createTagPaneLayout(settings::Settings* settings)
{
    return std::make_unique<TagPaneLayout>(settings);
}

}  // namespace photobook::tag_editor
