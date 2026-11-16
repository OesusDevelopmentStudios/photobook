/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#include "Navigator.hpp"

#include <QDir>
#include <QFileInfo>
#include <QtConcurrentRun>

namespace
{

// Loading the next page starts this many images before the end of the loaded results, so stepping rarely
// has to wait for it.
constexpr int libraryRowsAhead = 10;

// Same form as the paths the folder scanner returns, so they can be compared.
QString normalizedPath(const QString& filePath)
{
    return QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
}

}  // namespace

namespace pictorio::navigator
{

Navigator::Navigator(std::unique_ptr<folder_scanner::FolderScanner> folderScanner, QObject* parent)
    : QObject(parent)
    , m_folderScanner(std::move(folderScanner))
{
    connect(&m_listingWatcher, &QFutureWatcher<FolderListing>::finished, this, &Navigator::takeFolderListing);
}

Navigator::~Navigator() = default;

void Navigator::openImage(const QString& filePath)
{
    stopBrowsingLibrary();
    m_currentPath = normalizedPath(filePath);
    Q_EMIT imageRequested(m_currentPath);

    const QString folderPath = QFileInfo(m_currentPath).path();
    m_requestedFolderPath = folderPath;
    if (folderPath == m_folderPath)
    {
        m_currentIndex = m_imagePaths.indexOf(m_currentPath);
    }
    else
    {
        clearListing();
        startListingFolder(folderPath);
    }

    Q_EMIT positionChanged();
}

void Navigator::openFolder(const QString& folderPath)
{
    stopBrowsingLibrary();
    m_currentPath.clear();  // the first image follows once the folder is listed
    clearListing();
    m_requestedFolderPath = normalizedPath(folderPath);
    startListingFolder(m_requestedFolderPath);
    Q_EMIT positionChanged();
}

void Navigator::clear()
{
    stopBrowsingLibrary();
    m_currentPath.clear();
    m_requestedFolderPath.clear();
    clearListing();
    Q_EMIT positionChanged();
}

void Navigator::openLibraryImage(library::Library* library, int row)
{
    stopBrowsingLibrary();
    m_currentPath.clear();
    m_requestedFolderPath.clear();  // a folder listing still on its way is ignored
    clearListing();

    m_library = library;
    connect(m_library, &library::Library::rowsInserted, this, &Navigator::takeLibraryRows);
    connect(m_library, &library::Library::modelReset, this, [this]() {
        stopBrowsingLibrary();
        Q_EMIT positionChanged();
    });
    connect(m_library, &library::Library::totalCountChanged, this, &Navigator::positionChanged);
    goToLibraryRow(row);
}

void Navigator::setSorting(const folder_scanner::Sorting& sorting)
{
    if (sorting == m_sorting)
    {
        return;
    }

    m_sorting = sorting;
    if (m_requestedFolderPath.isEmpty())
    {
        return;
    }

    clearListing();
    startListingFolder(m_requestedFolderPath);
    Q_EMIT positionChanged();
}

void Navigator::setWrapAround(bool wrapAround)
{
    if (wrapAround != m_wrapAround)
    {
        m_wrapAround = wrapAround;
        Q_EMIT positionChanged();
    }
}

void Navigator::goToPrevious()
{
    if (!canGoToPrevious())
    {
        return;
    }

    if (isBrowsingLibrary())
    {
        goToLibraryRow(m_currentIndex > 0 ? m_currentIndex - 1 : loadedLibraryRows() - 1);
        return;
    }

    goToIndex((m_currentIndex + imageCount() - 1) % imageCount());
}

void Navigator::goToNext()
{
    if (!canGoToNext())
    {
        return;
    }

    if (isBrowsingLibrary())
    {
        goToNextLibraryRow();
        return;
    }

    goToIndex((m_currentIndex + 1) % imageCount());
}

int Navigator::currentIndex() const
{
    return m_currentIndex;
}

int Navigator::imageCount() const
{
    return isBrowsingLibrary() ? m_library->totalCount() : static_cast<int>(m_imagePaths.size());
}

bool Navigator::canGoToPrevious() const
{
    if (isBrowsingLibrary())
    {
        const bool allLoaded = !m_library->hasMoreToLoad();
        return m_currentIndex > 0 || (m_wrapAround && allLoaded && loadedLibraryRows() > 1);
    }

    if (m_currentIndex < 0 || imageCount() < 2)
    {
        return false;
    }

    return m_wrapAround || m_currentIndex > 0;
}

bool Navigator::canGoToNext() const
{
    if (isBrowsingLibrary())
    {
        const bool moreAfter = m_currentIndex + 1 < loadedLibraryRows() || m_library->hasMoreToLoad();
        return moreAfter || (m_wrapAround && loadedLibraryRows() > 1);
    }

    if (m_currentIndex < 0 || imageCount() < 2)
    {
        return false;
    }

    return m_wrapAround || m_currentIndex < imageCount() - 1;
}

bool Navigator::isBrowsingLibrary() const
{
    return m_library != nullptr;
}

void Navigator::startListingFolder(const QString& folderPath)
{
    m_listingWatcher.setFuture(QtConcurrent::run([folderScanner = m_folderScanner, folderPath, sorting = m_sorting]() {
        return FolderListing{folderPath, sorting, folderScanner->listImages(folderPath, sorting)};
    }));
}

void Navigator::clearListing()
{
    m_folderPath.clear();
    m_imagePaths.clear();
    m_currentIndex = -1;
}

void Navigator::takeFolderListing()
{
    FolderListing listing = m_listingWatcher.result();

    // In the meantime another folder may have been opened, or the sorting changed.
    if (listing.folderPath != m_requestedFolderPath || listing.sorting != m_sorting)
    {
        return;
    }

    m_folderPath = listing.folderPath;
    m_imagePaths = std::move(listing.imagePaths);
    if (!m_currentPath.isEmpty())
    {
        m_currentIndex = m_imagePaths.indexOf(m_currentPath);
        Q_EMIT positionChanged();
        return;
    }

    // A folder was opened: show its first image.
    if (m_imagePaths.isEmpty())
    {
        Q_EMIT positionChanged();
        Q_EMIT folderHasNoImages(m_folderPath);
        return;
    }

    goToIndex(0);
}

void Navigator::goToIndex(int index)
{
    m_currentIndex = index;
    m_currentPath = m_imagePaths.at(index);
    Q_EMIT imageRequested(m_currentPath);
    Q_EMIT positionChanged();
}

// Leaves the position as it is; the caller sets the new one.
void Navigator::stopBrowsingLibrary()
{
    if (m_library != nullptr)
    {
        disconnect(m_library, nullptr, this, nullptr);
        m_library = nullptr;
        m_currentIndex = -1;
    }

    m_waitingForRow = -1;
}

void Navigator::goToLibraryRow(int row)
{
    m_waitingForRow = -1;
    m_currentIndex = row;
    Q_EMIT libraryImageRequested(row);
    Q_EMIT positionChanged();
    loadMoreIfNearTheEnd();
}

void Navigator::goToNextLibraryRow()
{
    const int nextRow = m_currentIndex + 1;
    if (nextRow < loadedLibraryRows())
    {
        goToLibraryRow(nextRow);
    }
    else if (m_library->hasMoreToLoad())
    {
        m_waitingForRow = nextRow;  // takeLibraryRows() shows it
        m_library->loadMore();
    }
    else
    {
        goToLibraryRow(0);  // wrap-around, which canGoToNext() allowed
    }
}

void Navigator::takeLibraryRows()
{
    if (m_waitingForRow >= 0 && m_waitingForRow < loadedLibraryRows())
    {
        goToLibraryRow(m_waitingForRow);
        return;
    }

    Q_EMIT positionChanged();  // e.g. the last loaded image has a next one now
    loadMoreIfNearTheEnd();
}

void Navigator::loadMoreIfNearTheEnd()
{
    if (m_currentIndex + libraryRowsAhead >= loadedLibraryRows())
    {
        m_library->loadMore();  // does nothing if all is loaded or a page is already on its way
    }
}

int Navigator::loadedLibraryRows() const
{
    return m_library->count();
}

}  // namespace pictorio::navigator
