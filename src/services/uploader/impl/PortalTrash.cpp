/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "impl/PortalTrash.hpp"

#include <cerrno>

#include <fcntl.h>

#include <KLocalizedString>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusUnixFileDescriptor>
#include <QFile>

namespace photobook::uploader
{

QFuture<std::expected<void, QString>> PortalTrash::moveToTrash(const QString& filePath)
{
    using Outcome = std::expected<void, QString>;

    // The portal is handed the file itself, not its path. It trashes only a file opened for writing: the
    // sandbox must allow the app to change it. A link is not followed, so its target never goes instead.
    const int fd = ::open(QFile::encodeName(filePath).constData(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    if (fd == -1)
    {
        return QtFuture::makeReadyValueFuture(Outcome(std::unexpected(qt_error_string(errno))));
    }

    QDBusUnixFileDescriptor file;
    file.giveFileDescriptor(fd);
    QDBusMessage call = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                       QStringLiteral("/org/freedesktop/portal/desktop"),
                                                       QStringLiteral("org.freedesktop.portal.Trash"),
                                                       QStringLiteral("TrashFile"));
    call << QVariant::fromValue(file);

    auto* watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call));
    return QtFuture::connect(watcher, &QDBusPendingCallWatcher::finished)
        .then([](QDBusPendingCallWatcher* finished) -> Outcome {
            finished->deleteLater();
            const QDBusPendingReply<uint> reply = *finished;
            if (reply.isError())
            {
                return std::unexpected(reply.error().message());
            }

            // The portal gives no reason, only 1 for done.
            if (reply.value() != 1)
            {
                return std::unexpected(i18nc("@info", "The desktop did not move it to the trash."));
            }

            return {};
        });
}

}  // namespace photobook::uploader
