/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include "TagSuggestions.hpp"

namespace pictorio::tag_editor
{

// The connection is not owned and must outlive the suggestions.
std::unique_ptr<TagSuggestions> createTagSuggestions(connection::Connection* connection);

}  // namespace pictorio::tag_editor
