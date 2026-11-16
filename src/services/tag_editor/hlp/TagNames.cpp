/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/TagNames.hpp"

#include <optional>

#include <KLocalizedString>
#include <QRegularExpression>

namespace
{

constexpr int maximumNameLength = 100;  // in characters, as the server counts them

// Tag names can become folder names on the server, so no path separators.
const QString pathCharacters = QStringLiteral("/\\");
// Operators of the search language
const QString searchOperators = QStringLiteral(",|()*");

// The first rule the name breaks, if any
std::optional<QString> problemWith(const QString& name)
{
    if (name.isEmpty())
    {
        return i18nc("@info", "A tag name cannot be empty.");
    }

    if (name == QLatin1String(".") || name == QLatin1String(".."))
    {
        return i18nc("@info", "A tag name cannot be '.' or '..'.");
    }

    const QList<uint> characters = name.toUcs4();
    for (const uint character : characters)
    {
        const QString text = QString::fromUcs4(reinterpret_cast<const char32_t*>(&character), 1);
        if (pathCharacters.contains(text))
        {
            return i18nc("@info %1 is a character", "A tag name cannot contain '%1'.", text);
        }

        if (searchOperators.contains(text))
        {
            return i18nc("@info %1 is a character", "A tag name cannot contain '%1': it is a search operator.", text);
        }

        if (QChar::category(static_cast<char32_t>(character)) == QChar::Other_Control)
        {
            return i18nc("@info", "A tag name cannot contain control characters.");
        }
    }

    if (name.startsWith(QLatin1Char('-')))
    {
        return i18nc("@info", "A tag name cannot start with '-': it means NOT in a search.");
    }

    if (characters.size() > maximumNameLength)
    {
        return i18nc("@info", "A tag name can have at most %1 characters.", maximumNameLength);
    }

    return std::nullopt;
}

}  // namespace

namespace pictorio::tag_editor
{

QString TagName::text() const
{
    return namespaceName + QLatin1Char(':') + name;
}

std::expected<TagName, QString> tagFromTypedText(const QString& typedText, const QSet<QString>& namespaceNames)
{
    // Split at the first ':' only if the part before it is a namespace, so "re:zero" stays one general tag.
    const qsizetype colon = typedText.indexOf(QLatin1Char(':'));
    if (colon >= 0)
    {
        const QString prefix = typedText.left(colon).trimmed().toLower();
        if (namespaceNames.contains(prefix))
        {
            return tagFromTypedName(prefix, typedText.mid(colon + 1));
        }
    }

    return tagFromTypedName(generalNamespace, typedText);
}

std::expected<TagName, QString> tagFromTypedName(const QString& namespaceName, const QString& typedName)
{
    static const QRegularExpression spaces(QStringLiteral(" +"));
    const QString name =
        typedName.normalized(QString::NormalizationForm_C).trimmed().toLower().replace(spaces, QStringLiteral("_"));
    if (const std::optional<QString> problem = problemWith(name))
    {
        return std::unexpected(*problem);
    }

    return TagName{namespaceName, name};
}

TagName tagFromServer(const QString& text)
{
    const qsizetype colon = text.indexOf(QLatin1Char(':'));
    return colon < 0 ? TagName{generalNamespace, text} : TagName{text.left(colon), text.mid(colon + 1)};
}

QList<TagName> tagsAfterChange(const QList<TagName>& current,
                               const QList<TagName>& additions,
                               const QList<TagName>& removals,
                               const QSet<QString>& singleValueNamespaces)
{
    QList<TagName> tags;
    for (const TagName& tag : current)
    {
        if (!removals.contains(tag))
        {
            tags.append(tag);
        }
    }

    for (const TagName& added : additions)
    {
        if (singleValueNamespaces.contains(added.namespaceName))
        {
            tags.removeIf([&](const TagName& tag) { return tag.namespaceName == added.namespaceName; });
        }

        if (!tags.contains(added))
        {
            tags.append(added);
        }
    }

    return tags;
}

}  // namespace pictorio::tag_editor
