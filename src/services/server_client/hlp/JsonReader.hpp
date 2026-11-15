/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace pictorio::server_client
{

// Reads the fields of a JSON object that the API promises. The first field that is missing or has the
// wrong type is remembered, and the getters then return empty values: check isValid() after reading.
class JsonReader
{
public:
    explicit JsonReader(const QJsonObject& object, const QString& context = {});

    QString string(const QString& key);
    int integer(const QString& key);
    bool boolean(const QString& key);
    QDateTime dateTime(const QString& key);
    std::optional<QDateTime> optionalDateTime(const QString& key);  // null: nothing
    QStringList stringList(const QString& key);
    QJsonObject object(const QString& key);

    void reject(const QString& key, const QString& reason);  // for checks the getters cannot make
    bool isValid() const;
    QString error() const;  // e.g. "user.role: missing"

private:
    QJsonValue field(const QString& key, QJsonValue::Type type);

    QJsonObject m_object;
    QString m_context;
    QString m_error;
};

}  // namespace pictorio::server_client
