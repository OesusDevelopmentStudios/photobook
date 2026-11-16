/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "Library.hpp"

#include <algorithm>
#include <limits>

#include <KLocalizedString>
#include <QRandomGenerator>

namespace
{

using namespace pictorio::server_client;

// The server's answer as a sentence for the user. A search the server cannot read says why.
QString describeError(const ApiError& error)
{
    if (error.code == networkErrorCode)
    {
        return i18nc("@info %1 is the reason, e.g. Connection refused", "The server does not answer: %1", error.detail);
    }

    if (error.detail.isEmpty())
    {
        return error.title;
    }

    return i18nc("@info %1 is a title, %2 an explanation", "%1. %2", error.title, error.detail);
}

}  // namespace

namespace pictorio::library
{

Library::Library(connection::Connection* connection, QObject* parent)
    : QAbstractListModel(parent)
    , m_connection(connection)
{
    connect(m_connection, &connection::Connection::stateChanged, this, &Library::reload);
    reload();
}

void Library::search(const QString& text)
{
    const QString searchText = text.trimmed();
    const bool changed = searchText != m_searchText;
    m_searchText = searchText;
    reload();
    if (changed)
    {
        Q_EMIT searchChanged();
    }
}

void Library::reload()
{
    ++m_searchNumber;
    clearImages();
    setTotalCount(-1);
    setErrorMessage({});
    setLoading(false);
    if (!isLoggedIn())
    {
        return;
    }

    m_complete = false;
    if (m_order == Order::Random)
    {
        m_randomSeed = QRandomGenerator::global()->bounded(Q_INT64_C(1), std::numeric_limits<qint64>::max());
    }

    loadPage({});
    loadTotalCount();
}

void Library::loadMore()
{
    fetchMore({});
}

QString Library::searchText() const
{
    return m_searchText;
}

Library::Order Library::order() const
{
    return m_order;
}

void Library::setOrder(Order order)
{
    if (order != m_order || order == Order::Random)  // choosing random again shuffles again
    {
        m_order = order;
        reload();  // before the announcement, as in search()
        Q_EMIT searchChanged();
    }
}

bool Library::isLoading() const
{
    return m_loading;
}

QString Library::errorMessage() const
{
    return m_errorMessage;
}

QString Library::effectiveQuery() const
{
    return m_effectiveQuery;
}

int Library::count() const
{
    return static_cast<int>(m_images.size());
}

std::optional<server_client::Image> Library::imageAt(int row) const
{
    if (row < 0 || row >= count())
    {
        return std::nullopt;
    }

    return m_images.at(row);
}

int Library::totalCount() const
{
    return m_totalCount;
}

bool Library::hasMoreToLoad() const
{
    return !m_complete;
}

void Library::replaceImage(const server_client::Image& image)
{
    const auto found = std::ranges::find(m_images, image.id, &server_client::Image::id);
    if (found == m_images.end())
    {
        return;
    }

    *found = image;
    const QModelIndex changed = index(static_cast<int>(std::distance(m_images.begin(), found)));
    Q_EMIT dataChanged(changed, changed);
}

void Library::toggleSelected(int row)
{
    const std::optional<server_client::Image> image = imageAt(row);
    if (!image.has_value())
    {
        return;
    }

    QSet<int> selection = m_selectedImageIds;
    if (!selection.remove(image->id))
    {
        selection.insert(image->id);
    }

    m_rangeStartRow = row;
    setSelection(selection);
}

void Library::selectRange(int row, bool addToSelection)
{
    if (row < 0 || row >= count())
    {
        return;
    }

    const int start = m_rangeStartRow >= 0 ? m_rangeStartRow : row;  // no start yet: this image alone
    QSet<int> selection = addToSelection ? m_selectedImageIds : QSet<int>();
    for (int rangeRow = std::min(start, row); rangeRow <= std::max(start, row); ++rangeRow)
    {
        selection.insert(m_images.at(rangeRow).id);
    }

    m_rangeStartRow = start;
    setSelection(selection);
}

void Library::clearSelection()
{
    m_rangeStartRow = -1;
    setSelection({});
}

int Library::selectedCount() const
{
    return static_cast<int>(m_selectedImageIds.size());
}

QList<int> Library::selectedImageIds() const
{
    QList<int> imageIds;
    for (const server_client::Image& image : selectedImages())
    {
        imageIds.append(image.id);
    }

    return imageIds;
}

QList<server_client::Image> Library::selectedImages() const
{
    QList<server_client::Image> images;
    for (const server_client::Image& image : m_images)
    {
        if (m_selectedImageIds.contains(image.id))
        {
            images.append(image);
        }
    }

    return images;
}

// The tiles whose selection changed are told, from the first to the last of them.
void Library::setSelection(const QSet<int>& imageIds)
{
    if (imageIds == m_selectedImageIds)
    {
        return;
    }

    const QSet<int> changed = (imageIds - m_selectedImageIds) + (m_selectedImageIds - imageIds);
    m_selectedImageIds = imageIds;
    int firstRow = -1;
    int lastRow = -1;
    for (int row = 0; row < count(); ++row)
    {
        if (changed.contains(m_images.at(row).id))
        {
            firstRow = firstRow < 0 ? row : firstRow;
            lastRow = row;
        }
    }

    if (firstRow >= 0)
    {
        Q_EMIT dataChanged(index(firstRow), index(lastRow), {SelectedRole});
    }

    Q_EMIT selectionChanged();
}

int Library::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : count();
}

QVariant Library::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
    {
        return {};
    }

    const server_client::Image& image = m_images.at(index.row());
    switch (role)
    {
    case ImageIdRole:
        return image.id;
    case FileNameRole:
    case Qt::DisplayRole:
        return image.originalFilename;
    case ThumbnailUrlRole:
        return isLoggedIn() ? m_connection->client()->thumbnailUrl(image.id, ThumbnailSize::Small) : QUrl();
    case LargeThumbnailUrlRole:
        return isLoggedIn() ? m_connection->client()->thumbnailUrl(image.id, ThumbnailSize::Large) : QUrl();
    case TakenAtRole:
        return image.takenAt;
    case WidthRole:
        return image.width;
    case HeightRole:
        return image.height;
    case SelectedRole:
        return m_selectedImageIds.contains(image.id);
    default:
        return {};
    }
}

QHash<int, QByteArray> Library::roleNames() const
{
    return {
        {ImageIdRole, "imageId"},
        {FileNameRole, "fileName"},
        {ThumbnailUrlRole, "thumbnailUrl"},
        {LargeThumbnailUrlRole, "largeThumbnailUrl"},
        {TakenAtRole, "takenAt"},
        {WidthRole, "imageWidth"},
        {HeightRole, "imageHeight"},
        {SelectedRole, "selected"},
    };
}

// After an error the grid stops asking; a new search or reload() tries again.
bool Library::canFetchMore(const QModelIndex& parent) const
{
    return !parent.isValid() && !m_complete && !m_loading && m_errorMessage.isEmpty() && isLoggedIn();
}

void Library::fetchMore(const QModelIndex& parent)
{
    if (canFetchMore(parent))
    {
        loadPage(m_nextCursor);
    }
}

bool Library::isLoggedIn() const
{
    return m_connection->state() == connection::Connection::State::LoggedIn && m_connection->client() != nullptr;
}

void Library::loadPage(const QString& cursor)
{
    setLoading(true);
    const int searchNumber = m_searchNumber;
    m_connection->client()
        ->searchImages(queryFor(cursor))
        .then(this, [this, searchNumber](const Result<ImagePage>& page) {
            if (searchNumber != m_searchNumber)
            {
                return;  // the user has searched for something else since
            }

            if (!page.has_value())
            {
                setErrorMessage(describeError(page.error()));
                setLoading(false);
                return;
            }

            addPage(*page);
        });
}

// Without the total the grid works as well, so a failure is not shown; the page's own error says enough.
void Library::loadTotalCount()
{
    const int searchNumber = m_searchNumber;
    m_connection->client()->countImages(queryFor({})).then(this, [this, searchNumber](const Result<ImageCount>& found) {
        if (searchNumber == m_searchNumber && found.has_value())
        {
            setTotalCount(found->count);
        }
    });
}

void Library::setTotalCount(int totalCount)
{
    if (totalCount != m_totalCount)
    {
        m_totalCount = totalCount;
        Q_EMIT totalCountChanged();
    }
}

// The next page's cursor is kept before anyone hears of the new images: the grid asks for more as soon as
// they arrive.
void Library::addPage(const server_client::ImagePage& page)
{
    m_nextCursor = page.nextCursor;
    m_complete = page.nextCursor.isEmpty();
    setLoading(false);
    if (page.effectiveQuery != m_effectiveQuery)
    {
        m_effectiveQuery = page.effectiveQuery;
        Q_EMIT effectiveQueryChanged();
    }

    if (!page.items.isEmpty())
    {
        beginInsertRows({}, count(), count() + static_cast<int>(page.items.size()) - 1);
        m_images.append(page.items);
        endInsertRows();
        Q_EMIT countChanged();
    }
}

// The old search's next page is forgotten before the grid hears of the reset: the grid asks for more as soon
// as it is empty, and must not get the old search's next page then.
void Library::clearImages()
{
    m_nextCursor.clear();
    m_complete = true;
    const bool hadSelection = !m_selectedImageIds.isEmpty();
    m_selectedImageIds.clear();  // the new results start without a selection
    m_rangeStartRow = -1;
    if (!m_images.isEmpty())
    {
        beginResetModel();
        m_images.clear();
        endResetModel();
        Q_EMIT countChanged();
    }

    if (hadSelection)
    {
        Q_EMIT selectionChanged();
    }
}

void Library::setLoading(bool loading)
{
    if (loading != m_loading)
    {
        m_loading = loading;
        Q_EMIT loadingChanged();
    }
}

void Library::setErrorMessage(const QString& message)
{
    if (message != m_errorMessage)
    {
        m_errorMessage = message;
        Q_EMIT errorMessageChanged();
    }
}

server_client::ImageQuery Library::queryFor(const QString& cursor) const
{
    server_client::ImageQuery query;
    query.text = m_searchText;
    query.limit = pageSize;
    query.cursor = cursor;
    switch (m_order)
    {
    case Order::NewestFirst:
        break;  // the server's default: by date taken, newest first
    case Order::OldestFirst:
        query.order = SortOrder::Ascending;
        break;
    case Order::RecentlyAdded:
        query.sortBy = SortBy::ImportedAt;
        break;
    case Order::Random:
        query.sortBy = SortBy::Random;
        query.seed = m_randomSeed;
        break;
    }

    return query;
}

}  // namespace pictorio::library
