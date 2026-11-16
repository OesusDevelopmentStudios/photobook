/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QObject>

#include "Settings.hpp"

namespace pictorio::library_view
{

// How the library page is laid out, kept between runs: whether its sidebar is shown.
class LibraryView : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool sidebarVisible READ isSidebarVisible WRITE setSidebarVisible NOTIFY sidebarVisibleChanged)

public:
    // The settings are not owned and must outlive the library view.
    explicit LibraryView(settings::Settings* settings, QObject* parent = nullptr);

    bool isSidebarVisible() const;  // shown until the user hides it
    void setSidebarVisible(bool visible);

Q_SIGNALS:
    void sidebarVisibleChanged();

private:
    settings::Settings* m_settings;
    bool m_sidebarVisible;
};

}  // namespace pictorio::library_view
