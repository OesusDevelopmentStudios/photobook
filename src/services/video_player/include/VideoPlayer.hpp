/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QList>
#include <QObject>
#include <QUrl>

#include "Connection.hpp"
#include "Settings.hpp"

namespace photobook::video_player
{

// What the viewer's player plays and how: the address of the video, and the volume, speed and muting the user
// chose. Playing itself is Qt Multimedia's MediaPlayer in QML. A library video is played from a signed URL that
// the server gives for it, as Qt Multimedia cannot send the login token.
class VideoPlayer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl source READ source NOTIFY sourceChanged)
    Q_PROPERTY(bool waitingForSource READ isWaitingForSource NOTIFY sourceChanged)
    Q_PROPERTY(qint64 startPositionMs READ startPositionMs NOTIFY sourceChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QList<double> speeds READ speeds CONSTANT)
    Q_PROPERTY(bool startMuted READ startsMuted WRITE setStartMuted NOTIFY startMutedChanged)

public:
    static constexpr double defaultVolume = 1.0;
    static constexpr double defaultSpeed = 1.0;

    // Neither is owned; both must outlive the player.
    VideoPlayer(connection::Connection* connection, settings::Settings* settings, QObject* parent = nullptr);
    ~VideoPlayer() override;

    // A video in the library: the server is asked for a URL to stream it from.
    void openLibraryVideo(int imageId);
    // A video file on this computer
    void openLocalVideo(const QString& filePath);
    // Nothing to play, e.g. the viewer moved on to a photo
    void close();

    // QML's MediaPlayer could not play the source; positionMs is where it was. A library video's URL may have
    // expired or been refused, so it gets one new URL and goes on from there. Otherwise the message is shown.
    Q_INVOKABLE void reportPlaybackError(const QString& message, qint64 positionMs);

    // A time in the video as the controls show it, e.g. "3:42"
    Q_INVOKABLE QString timeText(qint64 milliseconds) const;

    QUrl source() const;  // empty while waiting for a URL, after an error and when closed
    bool isWaitingForSource() const;
    qint64 startPositionMs() const;  // where to start: 0, or where a refused URL stopped
    QString errorMessage() const;

    double volume() const;
    void setVolume(double volume);  // 0 to 1; remembered
    bool isMuted() const;
    void setMuted(bool muted);  // for this video; the next one starts as startMuted says
    double speed() const;
    void setSpeed(double speed);  // one of speeds(); remembered
    QList<double> speeds() const;
    bool startsMuted() const;
    void setStartMuted(bool muted);  // remembered

Q_SIGNALS:
    void sourceChanged();
    void errorMessageChanged();
    void volumeChanged();
    void mutedChanged();
    void speedChanged();
    void startMutedChanged();

private:
    void requestStreamUrl(qint64 startPositionMs);
    void setSource(const QUrl& source, qint64 startPositionMs);
    void setErrorMessage(const QString& message);

    connection::Connection* m_connection;
    settings::Settings* m_settings;
    QUrl m_source;
    qint64 m_startPositionMs = 0;
    QString m_errorMessage;
    int m_libraryImageId = 0;  // 0: a local video, or none
    bool m_waitingForSource = false;
    bool m_askedAgain = false;  // the open library video got its one new URL already
    int m_openCount = 0;        // which open an answer belongs to; an older one's answer is ignored
    double m_volume = defaultVolume;
    bool m_muted = false;
    double m_speed = defaultSpeed;
    bool m_startMuted = false;
};

}  // namespace photobook::video_player
