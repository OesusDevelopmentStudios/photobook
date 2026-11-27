/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QObject>
#include <QTimer>

#include "Navigator.hpp"
#include "Settings.hpp"

namespace photobook::slideshow
{

// Starts with the image on screen and starts over after the last one. The chosen order applies to folders;
// library images keep the library's order. Moving by hand also restarts the interval. A video plays to its end
// instead of for the interval.
class Slideshow : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)
    Q_PROPERTY(bool paused READ isPaused NOTIFY stateChanged)
    Q_PROPERTY(int intervalSeconds READ intervalSeconds WRITE setIntervalSeconds NOTIFY intervalSecondsChanged)
    Q_PROPERTY(Order order READ order NOTIFY orderChanged)
    // A video is on screen and can play: its end moves on (videoEnded()), not the interval.
    Q_PROPERTY(bool waitingForVideo READ isWaitingForVideo WRITE setWaitingForVideo NOTIFY waitingForVideoChanged)
    Q_PROPERTY(int minimumIntervalSeconds READ minimumIntervalSecondsForQml CONSTANT)
    Q_PROPERTY(int maximumIntervalSeconds READ maximumIntervalSecondsForQml CONSTANT)

public:
    enum class Order
    {
        ByName,
        ByDate,
        Random,
    };

    Q_ENUM(Order)

    static constexpr int minimumIntervalSeconds = 1;
    static constexpr int maximumIntervalSeconds = 3600;
    static constexpr int defaultIntervalSeconds = 5;

    Slideshow(navigator::Navigator* navigator, settings::Settings* settings, QObject* parent = nullptr);
    ~Slideshow() override;

    Q_INVOKABLE void start(Order order);
    Q_INVOKABLE void stop();  // the folder is in name order again
    Q_INVOKABLE void pause();
    Q_INVOKABLE void resume();
    // The video on screen has played to its end.
    Q_INVOKABLE void videoEnded();

    bool isRunning() const;  // also while paused
    bool isPaused() const;
    int intervalSeconds() const;
    void setIntervalSeconds(int intervalSeconds);  // limited to minimum..maximum
    Order order() const;
    bool isWaitingForVideo() const;
    void setWaitingForVideo(bool waiting);

Q_SIGNALS:
    void stateChanged();
    void intervalSecondsChanged();
    void orderChanged();
    void waitingForVideoChanged();

private:
    // The limits for QML, which cannot read static members
    int minimumIntervalSecondsForQml() const;
    int maximumIntervalSecondsForQml() const;

    void applyIntervalSeconds(int intervalSeconds);
    void showNextImage();
    void restartIntervalIfShowing();
    void startOrStopInterval();

    navigator::Navigator* m_navigator;
    settings::Settings* m_settings;
    QTimer m_timer;
    Order m_order = Order::ByName;
    bool m_running = false;
    bool m_paused = false;
    bool m_waitingForVideo = false;
};

}  // namespace photobook::slideshow
