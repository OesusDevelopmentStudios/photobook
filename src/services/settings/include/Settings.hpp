/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QString>
#include <QUrl>

namespace pictorio::settings
{

// Settings that are kept between runs of the application. Getters return nothing for values that were
// never stored, so each user of a setting decides its default. Changes are saved at once.
class Settings
{
public:
    virtual ~Settings();

    virtual std::optional<int> slideshowIntervalSeconds() const = 0;
    virtual void setSlideshowIntervalSeconds(int intervalSeconds) = 0;

    // A word such as "date", so the config file stays readable
    virtual std::optional<QString> slideshowOrder() const = 0;
    virtual void setSlideshowOrder(const QString& order) = 0;

    // The pictorio-db server last connected to, and who logged in there. The login token itself is kept in
    // the system keyring, never here.
    virtual std::optional<QUrl> serverUrl() const = 0;
    virtual void setServerUrl(const QUrl& serverUrl) = 0;
    virtual std::optional<QString> serverUsername() const = 0;
    virtual void setServerUsername(const QString& username) = 0;
};

}  // namespace pictorio::settings
