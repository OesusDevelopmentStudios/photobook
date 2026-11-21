/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <memory>

#include <QFutureWatcher>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

#include "FolderScanner.hpp"
#include "Library.hpp"

namespace pictorio::navigator
{

// The neighbours are a local folder's images, or the library's results. A folder is listed in the background:
// until then, next and previous are not available. It is in name order, without wrap-around, unless set
// otherwise (e.g. by a slideshow). Library images keep the library's order; pages load ahead near the end of
// the loaded results, and wrapping from the first to the last needs all of them loaded.
class Navigator : public QObject
{
    Q_OBJECT

    struct FolderListing
    {
        QString folderPath;
        folder_scanner::Sorting sorting;
        QStringList imagePaths;
    };

    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY positionChanged)
    Q_PROPERTY(int imageCount READ imageCount NOTIFY positionChanged)
    Q_PROPERTY(bool canGoToPrevious READ canGoToPrevious NOTIFY positionChanged)
    Q_PROPERTY(bool canGoToNext READ canGoToNext NOTIFY positionChanged)
    Q_PROPERTY(bool browsingLibrary READ isBrowsingLibrary NOTIFY positionChanged)

public:
    explicit Navigator(std::unique_ptr<folder_scanner::FolderScanner> folderScanner, QObject* parent = nullptr);
    ~Navigator() override;

    void openImage(const QString& filePath);
    // Its first image in the current order, once listed
    void openFolder(const QString& folderPath);
    // Previous and next do nothing until an image or folder is opened again; a listing on its way is ignored.
    void clear();
    // The library is not owned. Its results changing, e.g. by a new search, ends the browsing; the image on
    // screen stays.
    void openLibraryImage(library::Library* library, int row);
    void setSorting(const folder_scanner::Sorting& sorting);  // lists the folder again if it changed
    void setWrapAround(bool wrapAround);  // after the last image comes the first, and the other way round
    Q_INVOKABLE void goToPrevious();
    Q_INVOKABLE void goToNext();

    int currentIndex() const;  // -1 while the folder is being listed, or if the image is not in the list
    int imageCount() const;    // in the library, the total of its results; -1 until the server has counted
    bool canGoToPrevious() const;
    bool canGoToNext() const;
    bool isBrowsingLibrary() const;

Q_SIGNALS:
    void imageRequested(const QString& filePath);
    void libraryImageRequested(int row);
    void positionChanged();
    void folderHasNoImages(const QString& folderPath);  // after openFolder()

private:
    void startListingFolder(const QString& folderPath);
    void clearListing();
    void takeFolderListing();
    void goToIndex(int index);

    void stopBrowsingLibrary();
    void goToLibraryRow(int row);
    void goToNextLibraryRow();
    void takeLibraryRows();
    void loadMoreIfNearTheEnd();
    int loadedLibraryRows() const;

    // Shared with the background listing, which can outlive this object.
    std::shared_ptr<const folder_scanner::FolderScanner> m_folderScanner;
    QFutureWatcher<FolderListing> m_listingWatcher;

    folder_scanner::Sorting m_sorting;
    bool m_wrapAround = false;

    QString m_currentPath;
    QString m_requestedFolderPath;  // the folder being shown, whose listing is wanted
    QString m_folderPath;           // the folder m_imagePaths belongs to
    QStringList m_imagePaths;
    int m_currentIndex = -1;  // in the folder, or the row in the library

    QPointer<library::Library> m_library;  // set while browsing the library
    int m_waitingForRow = -1;              // stepped past the loaded results; shown once its page is in
};

}  // namespace pictorio::navigator
