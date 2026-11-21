/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QDateTime>
#include <QList>
#include <QSet>
#include <QStringList>

#include "hlp/TagNames.hpp"

namespace photobook::tag_editor
{

// As the server checks a change: an image must stay visible to the user with its tags after it. Gives the
// image's tags that the restricted terms which apply mention, sorted; none if the image stays visible.
QStringList restrictedTagsAmong(const QStringList& restrictedTerms,
                                const QSet<QString>& namespaceNames,
                                const QList<TagName>& tagsAfter,
                                const QDateTime& takenAt);

}  // namespace photobook::tag_editor
