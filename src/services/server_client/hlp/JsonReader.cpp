/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "hlp/JsonReader.hpp"

#include <limits>

#include <QJsonValue>

namespace
{

QString typeName(QJsonValue::Type type)
{
    switch (type)
    {
    case QJsonValue::String:
        return QStringLiteral("a string");
    case QJsonValue::Double:
        return QStringLiteral("a number");
    case QJsonValue::Bool:
        return QStringLiteral("true or false");
    case QJsonValue::Array:
        return QStringLiteral("a list");
    case QJsonValue::Object:
        return QStringLiteral("an object");
    default:
        return QStringLiteral("another type");
    }
}

}  // namespace

namespace pictorio::server_client
{

JsonReader::JsonReader(const QJsonObject& object, const QString& context)
    : m_object(object)
    , m_context(context)
{}

QString JsonReader::string(const QString& key)
{
    return field(key, QJsonValue::String).toString();
}

int JsonReader::integer(const QString& key)
{
    const qint64 number = integer64(key);
    if (number < std::numeric_limits<int>::min() || number > std::numeric_limits<int>::max())
    {
        reject(key, QStringLiteral("too large"));
        return 0;
    }

    return static_cast<int>(number);
}

qint64 JsonReader::integer64(const QString& key)
{
    const QJsonValue value = field(key, QJsonValue::Double);
    const qint64 number = value.toInteger();
    if (value.isDouble() && QJsonValue(number) != value)
    {
        reject(key, QStringLiteral("not a whole number"));
        return 0;
    }

    return number;
}

bool JsonReader::boolean(const QString& key)
{
    return field(key, QJsonValue::Bool).toBool();
}

QDateTime JsonReader::dateTime(const QString& key)
{
    const QString text = field(key, QJsonValue::String).toString();
    if (!isValid())
    {
        return {};
    }

    // The server sends UTC with microseconds, e.g. 2026-11-15T19:22:03.724229Z
    const QDateTime moment = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!moment.isValid())
    {
        reject(key, QStringLiteral("not a date and time: %1").arg(text));
        return {};
    }

    return moment.toUTC();
}

QDateTime JsonReader::wallClockTime(const QString& key)
{
    const QString text = field(key, QJsonValue::String).toString();
    if (!isValid())
    {
        return {};
    }

    // e.g. 2025-07-14T18:03:12: the time on the clock where the photo was taken. Qt reads it as local time
    // and it is shown unchanged, so it is never converted to another zone.
    const QDateTime moment = QDateTime::fromString(text, Qt::ISODateWithMs);
    if (!moment.isValid())
    {
        reject(key, QStringLiteral("not a date and time: %1").arg(text));
        return {};
    }

    return moment;
}

std::optional<QDateTime> JsonReader::optionalDateTime(const QString& key)
{
    if (isNull(key))
    {
        return std::nullopt;
    }

    return dateTime(key);
}

QString JsonReader::optionalString(const QString& key)
{
    return isNull(key) ? QString() : string(key);
}

QStringList JsonReader::stringList(const QString& key)
{
    QStringList strings;
    for (const QJsonValue& item : field(key, QJsonValue::Array).toArray())
    {
        if (!item.isString())
        {
            reject(key, QStringLiteral("a list with something other than strings"));
            return {};
        }

        strings.append(item.toString());
    }

    return strings;
}

std::optional<QStringList> JsonReader::optionalStringList(const QString& key)
{
    if (isNull(key))
    {
        return std::nullopt;
    }

    return stringList(key);
}

QJsonObject JsonReader::object(const QString& key)
{
    return field(key, QJsonValue::Object).toObject();
}

void JsonReader::reject(const QString& key, const QString& reason)
{
    if (m_error.isEmpty())  // the first problem is the one worth reporting
    {
        const QString path = m_context.isEmpty() ? key : m_context + QLatin1Char('.') + key;
        m_error = QStringLiteral("%1: %2").arg(path, reason);
    }
}

bool JsonReader::isValid() const
{
    return m_error.isEmpty();
}

QString JsonReader::error() const
{
    return m_error;
}

bool JsonReader::isNull(const QString& key) const
{
    return m_object.contains(key) && m_object.value(key).isNull();
}

QJsonValue JsonReader::field(const QString& key, QJsonValue::Type type)
{
    if (!m_object.contains(key))
    {
        reject(key, QStringLiteral("missing"));
        return {};
    }

    const QJsonValue value = m_object.value(key);
    if (value.type() != type)
    {
        reject(key, QStringLiteral("not %1").arg(typeName(type)));
        return {};
    }

    return value;
}

}  // namespace pictorio::server_client
