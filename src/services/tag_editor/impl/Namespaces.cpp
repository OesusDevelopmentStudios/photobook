/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/Namespaces.hpp"

#include <algorithm>
#include <iterator>

#include <KLocalizedString>

#include "hlp/TagNames.hpp"

namespace
{

// The tags offered in a dropdown without a fixed list: the most used ones
constexpr int knownValueCount = 50;

}  // namespace

namespace photobook::tag_editor
{

bool Namespaces::Info::hasDropdown() const
{
    return singleValue && builtin;
}

Namespaces::Namespaces(connection::Connection* connection, QObject* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_infos{
          {QStringLiteral("category"), true, true, std::nullopt},
          {QStringLiteral("author"), false, true, std::nullopt},
          {QStringLiteral("character"), false, true, std::nullopt},
          {QStringLiteral("rating"), true, true, std::nullopt},
          {QStringLiteral("album"), false, true, std::nullopt},
          {QStringLiteral("general"), false, true, std::nullopt},
      }
{}

void Namespaces::load()
{
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        return;
    }

    loadKnownValues();
    client->namespaces().then(this, [this](const server_client::Result<QList<server_client::Namespace>>& answer) {
        if (!answer.has_value())
        {
            return;  // the built-in ones are still known
        }

        m_infos.clear();
        for (const server_client::Namespace& info : *answer)
        {
            m_infos.append({info.name, info.singleValue, info.isBuiltin, info.allowedValues, info.userEditable});
        }

        loadKnownValues();  // for dropdowns only the server knew of
        Q_EMIT changed();
    });
}

const QList<Namespaces::Info>& Namespaces::all() const
{
    return m_infos;
}

QList<Namespaces::Info> Namespaces::editable() const
{
    QList<Info> editable;
    std::ranges::copy_if(m_infos, std::back_inserter(editable), &Info::userEditable);
    return editable;
}

const Namespaces::Info* Namespaces::find(const QString& name) const
{
    const auto found = std::ranges::find(m_infos, name, &Info::name);
    return found == m_infos.end() ? nullptr : &*found;
}

QSet<QString> Namespaces::names() const
{
    QSet<QString> names;
    for (const Info& info : m_infos)
    {
        names.insert(info.name);
    }

    return names;
}

QSet<QString> Namespaces::singleValueNames() const
{
    QSet<QString> names;
    for (const Info& info : m_infos)
    {
        if (info.singleValue)
        {
            names.insert(info.name);
        }
    }

    return names;
}

QStringList Namespaces::knownValues(const QString& name) const
{
    return m_knownValues.value(name);
}

std::optional<QString> Namespaces::notAllowedProblem(const QString& namespaceName, const QString& name) const
{
    const Info* info = find(namespaceName);
    if (info == nullptr || !info->allowedValues.has_value() || info->allowedValues->contains(name))
    {
        return std::nullopt;
    }

    return i18nc("@info %1 is a namespace, %2 a list of names",
                 "A %1 tag can only be one of: %2",
                 namespaceName,
                 info->allowedValues->join(QStringLiteral(", ")));
}

void Namespaces::loadKnownValues()
{
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        return;
    }

    for (const Info& info : m_infos)
    {
        if (!info.hasDropdown() || info.allowedValues.has_value() || m_knownValues.contains(info.name))
        {
            continue;
        }

        m_knownValues.insert(info.name, {});  // asked once
        client->tags({}, info.name, knownValueCount)
            .then(this, [this, name = info.name](const server_client::Result<QList<server_client::TagCount>>& answer) {
                QStringList names;
                for (const server_client::TagCount& known : answer.value_or(QList<server_client::TagCount>()))
                {
                    names.append(tagFromServer(known.tag).name);
                }

                m_knownValues.insert(name, names);
                Q_EMIT changed();
            });
    }
}

}  // namespace photobook::tag_editor
