/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "VideoPlayer.hpp"

#include <algorithm>

#include <KLocalizedString>

#include "ServerClient.hpp"

namespace photobook::video_player
{

namespace
{

// As players offer them, slower ones for looking closely
const QList<double> playbackSpeeds{0.25, 0.5, 1.0, 1.25, 1.5, 2.0};

QString cannotPlay(const QString& reason)
{
    return i18nc("@info %1 is the reason", "Cannot play the video: %1", reason);
}

QString reasonOf(const server_client::ApiError& error)
{
    if (error.code == server_client::networkErrorCode)
    {
        return i18nc(
            "@info %1 is the reason, e.g. Connection refused", "the server does not answer (%1)", error.detail);
    }

    return error.detail.isEmpty() ? error.title : error.detail;
}

}  // namespace

VideoPlayer::VideoPlayer(connection::Connection* connection, settings::Settings* settings, QObject* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_settings(settings)
{
    m_volume = std::clamp(m_settings->videoVolume().value_or(defaultVolume), 0.0, 1.0);
    const double storedSpeed = m_settings->videoSpeed().value_or(defaultSpeed);
    m_speed = playbackSpeeds.contains(storedSpeed) ? storedSpeed : defaultSpeed;
    m_startMuted = m_settings->startVideosMuted().value_or(false);
    m_muted = m_startMuted;
}

VideoPlayer::~VideoPlayer() = default;

void VideoPlayer::openLibraryVideo(int imageId)
{
    ++m_openCount;
    m_libraryImageId = imageId;
    m_askedAgain = false;
    setErrorMessage({});
    setMuted(m_startMuted);
    requestStreamUrl(0);
}

void VideoPlayer::openLocalVideo(const QString& filePath)
{
    ++m_openCount;
    m_libraryImageId = 0;
    m_waitingForSource = false;
    setErrorMessage({});
    setMuted(m_startMuted);
    setSource(QUrl::fromLocalFile(filePath), 0);
}

void VideoPlayer::close()
{
    ++m_openCount;
    m_libraryImageId = 0;
    m_waitingForSource = false;
    setErrorMessage({});
    setSource({}, 0);
}

void VideoPlayer::reportPlaybackError(const QString& message, qint64 positionMs)
{
    if (m_source.isEmpty())
    {
        return;  // an error of a source already left behind
    }

    if (m_libraryImageId != 0 && !m_askedAgain)
    {
        m_askedAgain = true;
        requestStreamUrl(positionMs);
        return;
    }

    setSource({}, 0);
    setErrorMessage(cannotPlay(message));
}

QUrl VideoPlayer::source() const
{
    return m_source;
}

bool VideoPlayer::isWaitingForSource() const
{
    return m_waitingForSource;
}

qint64 VideoPlayer::startPositionMs() const
{
    return m_startPositionMs;
}

QString VideoPlayer::errorMessage() const
{
    return m_errorMessage;
}

double VideoPlayer::volume() const
{
    return m_volume;
}

void VideoPlayer::setVolume(double volume)
{
    const double limited = std::clamp(volume, 0.0, 1.0);
    if (qFuzzyCompare(limited + 1.0, m_volume + 1.0))
    {
        return;
    }

    m_volume = limited;
    m_settings->setVideoVolume(m_volume);
    Q_EMIT volumeChanged();
}

bool VideoPlayer::isMuted() const
{
    return m_muted;
}

void VideoPlayer::setMuted(bool muted)
{
    if (muted == m_muted)
    {
        return;
    }

    m_muted = muted;
    Q_EMIT mutedChanged();
}

double VideoPlayer::speed() const
{
    return m_speed;
}

void VideoPlayer::setSpeed(double speed)
{
    if (!playbackSpeeds.contains(speed) || speed == m_speed)
    {
        return;
    }

    m_speed = speed;
    m_settings->setVideoSpeed(m_speed);
    Q_EMIT speedChanged();
}

QList<double> VideoPlayer::speeds() const
{
    return playbackSpeeds;
}

bool VideoPlayer::startsMuted() const
{
    return m_startMuted;
}

void VideoPlayer::setStartMuted(bool muted)
{
    if (muted == m_startMuted)
    {
        return;
    }

    m_startMuted = muted;
    m_settings->setStartVideosMuted(m_startMuted);
    Q_EMIT startMutedChanged();
}

void VideoPlayer::requestStreamUrl(qint64 startPositionMs)
{
    m_waitingForSource = true;
    setSource({}, startPositionMs);
    server_client::ServerClient* client = m_connection->client();
    if (client == nullptr)
    {
        m_waitingForSource = false;
        setErrorMessage(cannotPlay(i18nc("@info", "not connected to the server")));
        Q_EMIT sourceChanged();
        return;
    }

    const int openCount = m_openCount;
    client->streamUrl(m_libraryImageId)
        .then(this, [this, openCount, startPositionMs](const server_client::Result<server_client::StreamUrl>& answer) {
            if (openCount != m_openCount)
            {
                return;  // the viewer has moved on
            }

            m_waitingForSource = false;
            if (!answer.has_value())
            {
                setErrorMessage(cannotPlay(reasonOf(answer.error())));
                Q_EMIT sourceChanged();
                return;
            }

            setSource(answer->url, startPositionMs);
        });
}

void VideoPlayer::setSource(const QUrl& source, qint64 startPositionMs)
{
    m_source = source;
    m_startPositionMs = startPositionMs;
    Q_EMIT sourceChanged();
}

void VideoPlayer::setErrorMessage(const QString& message)
{
    if (message == m_errorMessage)
    {
        return;
    }

    m_errorMessage = message;
    Q_EMIT errorMessageChanged();
}

}  // namespace photobook::video_player
