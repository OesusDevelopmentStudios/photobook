/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "TagEditorFactory.hpp"

namespace pictorio::tag_editor
{

std::unique_ptr<TagEditor> createTagEditor(connection::Connection* connection)
{
    return std::make_unique<TagEditor>(connection);
}

std::unique_ptr<TagSuggestions> createTagSuggestions(connection::Connection* connection)
{
    return std::make_unique<TagSuggestions>(connection);
}

}  // namespace pictorio::tag_editor
