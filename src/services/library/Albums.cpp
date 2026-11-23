/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "Albums.hpp"

#include <algorithm>

#include "hlp/ServerErrors.hpp"

namespace
{

using photobook::library::Albums;
using photobook::server_client::Album;

// As stored in the settings: words, so the config file stays readable
const QString recentlyChangedWord = QStringLiteral("recently_changed");
const QString nameWord = QStringLiteral("name");
const QString sizeWord = QStringLiteral("size");

Albums::Sort sortFromWord(const std::optional<QString>& word)
{
    if (word == nameWord)
    {
        return Albums::Sort::Name;
    }

    if (word == sizeWord)
    {
        return Albums::Sort::Size;
    }

    return Albums::Sort::RecentlyChanged;
}

QString wordOf(Albums::Sort sort)
{
    switch (sort)
    {
    case Albums::Sort::Name:
        return nameWord;
    case Albums::Sort::Size:
        return sizeWord;
    case Albums::Sort::RecentlyChanged:
        break;
    }

    return recentlyChangedWord;
}

// "album:vacation_2015" is shown as "vacation 2015", as tag chips show names.
QString shownName(const Album& album)
{
    return album.tag.mid(album.tag.indexOf(QLatin1Char(':')) + 1).replace(QLatin1Char('_'), QLatin1Char(' '));
}

// For the filter: a typed "_" or " " finds either
QString comparable(const QString& text)
{
    return QString(text).replace(QLatin1Char('_'), QLatin1Char(' ')).toLower();
}

bool comesBefore(const Album& first, const Album& second, Albums::Sort sort)
{
    switch (sort)
    {
    case Albums::Sort::RecentlyChanged:
        if (first.lastChangedAt != second.lastChangedAt)
        {
            return first.lastChangedAt > second.lastChangedAt;
        }
        break;
    case Albums::Sort::Size:
        if (first.imageCount != second.imageCount)
        {
            return first.imageCount > second.imageCount;
        }
        break;
    case Albums::Sort::Name:
        break;
    }

    return QString::localeAwareCompare(shownName(first), shownName(second)) < 0;
}

}  // namespace

namespace photobook::library
{

Albums::Albums(connection::Connection* connection, settings::Settings* settings, QObject* parent)
    : QAbstractListModel(parent)
    , m_connection(connection)
    , m_settings(settings)
    , m_sort(sortFromWord(settings->albumSort()))
    , m_includeHidden(settings->showAlbumsWithHiddenImages().value_or(false))
{
    connect(m_connection, &connection::Connection::stateChanged, this, &Albums::reload);
    reload();
}

void Albums::reload()
{
    const int loadNumber = ++m_loadNumber;
    setErrorMessage({});
    if (!isLoggedIn())
    {
        setLoading(false);
        setAlbums({});
        return;
    }

    setLoading(true);
    m_connection->client()
        ->albums(m_includeHidden)
        .then(this, [this, loadNumber](const server_client::Result<QList<Album>>& answer) {
            if (loadNumber != m_loadNumber)
            {
                return;
            }

            setLoading(false);
            if (!answer.has_value())
            {
                setErrorMessage(describeError(answer.error()));
                return;  // the albums shown before stay
            }

            setAlbums(*answer);
        });
}

Albums::Sort Albums::sort() const
{
    return m_sort;
}

void Albums::setSort(Sort sort)
{
    if (sort == m_sort)
    {
        return;
    }

    m_sort = sort;
    m_settings->setAlbumSort(wordOf(sort));
    showAlbums();
    Q_EMIT sortChanged();
}

bool Albums::includesHidden() const
{
    return m_includeHidden;
}

void Albums::setIncludeHidden(bool include)
{
    if (include == m_includeHidden)
    {
        return;
    }

    m_includeHidden = include;
    m_settings->setShowAlbumsWithHiddenImages(include);
    reload();  // the server decides which albums hold hidden images
    Q_EMIT includeHiddenChanged();
}

QString Albums::filterText() const
{
    return m_filterText;
}

void Albums::setFilterText(const QString& text)
{
    if (text == m_filterText)
    {
        return;
    }

    m_filterText = text;
    showAlbums();
    Q_EMIT filterTextChanged();
}

bool Albums::isLoading() const
{
    return m_loading;
}

QString Albums::errorMessage() const
{
    return m_errorMessage;
}

int Albums::count() const
{
    return static_cast<int>(m_shown.size());
}

int Albums::totalCount() const
{
    return static_cast<int>(m_albums.size());
}

std::optional<server_client::Album> Albums::albumAt(int row) const
{
    if (row < 0 || row >= count())
    {
        return std::nullopt;
    }

    return m_shown.at(row);
}

int Albums::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant Albums::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
    {
        return {};
    }

    const Album& album = m_shown.at(index.row());
    switch (role)
    {
    case TagRole:
        return album.tag;
    case NameRole:
    case Qt::DisplayRole:
        return shownName(album);
    case ImageCountRole:
        return album.imageCount;
    case CoverUrlRole:
        return isLoggedIn() ? m_connection->client()->thumbnailUrl(
                                  album.coverImageId, server_client::ThumbnailSize::Small, album.coverRevision)
                            : QUrl();
    case LargeCoverUrlRole:
        return isLoggedIn() ? m_connection->client()->thumbnailUrl(
                                  album.coverImageId, server_client::ThumbnailSize::Large, album.coverRevision)
                            : QUrl();
    case LastChangedAtRole:
        return album.lastChangedAt;
    default:
        return {};
    }
}

QHash<int, QByteArray> Albums::roleNames() const
{
    return {
        {TagRole, "tag"},
        {NameRole, "name"},
        {ImageCountRole, "imageCount"},
        {CoverUrlRole, "coverUrl"},
        {LargeCoverUrlRole, "largeCoverUrl"},
        {LastChangedAtRole, "lastChangedAt"},
    };
}

bool Albums::isLoggedIn() const
{
    return m_connection->state() == connection::Connection::State::LoggedIn && m_connection->client() != nullptr;
}

void Albums::setAlbums(const QList<Album>& albums)
{
    const int oldTotal = totalCount();
    m_albums = albums;
    if (oldTotal == 0 && albums.isEmpty())
    {
        return;
    }

    showAlbums();
}

void Albums::showAlbums()
{
    const QString filter = comparable(m_filterText.trimmed());
    QList<Album> shown;
    for (const Album& album : m_albums)
    {
        if (filter.isEmpty() || comparable(shownName(album)).contains(filter))
        {
            shown.append(album);
        }
    }

    std::ranges::stable_sort(
        shown, [this](const Album& first, const Album& second) { return comesBefore(first, second, m_sort); });

    beginResetModel();
    m_shown = shown;
    endResetModel();
    Q_EMIT countChanged();
}

void Albums::setLoading(bool loading)
{
    if (loading != m_loading)
    {
        m_loading = loading;
        Q_EMIT loadingChanged();
    }
}

void Albums::setErrorMessage(const QString& message)
{
    if (message != m_errorMessage)
    {
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }
}

}  // namespace photobook::library
