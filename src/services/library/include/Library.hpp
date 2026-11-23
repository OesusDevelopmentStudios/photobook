/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QAbstractListModel>
#include <QList>
#include <QSet>

#include "ApiTypes.hpp"
#include "Connection.hpp"

namespace photobook::library
{

// The first page loads with the search; the grid asks for more through canFetchMore() and fetchMore() as it
// scrolls. Loads when the user logs in, empties when they log out. A library made for albums shows one album
// at a time instead of a search, and nothing until openAlbum().
class Library : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString searchText READ searchText NOTIFY searchChanged)
    Q_PROPERTY(Order order READ order WRITE setOrder NOTIFY searchChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool selecting READ isSelecting NOTIFY selectingChanged)

public:
    enum class Order
    {
        NewestFirst,  // by the date the photo was taken
        OldestFirst,
        RecentlyAdded,  // by the date it came into the library
        Random,
    };
    Q_ENUM(Order)

    enum Role
    {
        ImageIdRole = Qt::UserRole + 1,
        FileNameRole,
        ThumbnailUrlRole,       // the small thumbnail; loading it needs the login token
        LargeThumbnailUrlRole,  // for large tiles, where the small one would look blurry
        TakenAtRole,
        WidthRole,
        HeightRole,
        SelectedRole,  // chosen for changing the tags of many images at once
    };

    enum class Content
    {
        Search,  // the images a search finds, as the Library tab shows them
        Album,   // the images of an album opened from the Albums tab
    };

    static constexpr int pageSize = 100;

    explicit Library(connection::Connection* connection, Content content = Content::Search, QObject* parent = nullptr);

    // For a library made for albums: the album's images, in the order they were added to it and without the hidden
    // filters, as it was opened on purpose. The name is without "album:", e.g. "vacation_2015". An album without
    // images (its tag removed since it was listed) is an error, with its message.
    void openAlbum(const QString& albumName);
    QString albumName() const;

    // The server's search syntax, e.g. "category:photo summer -rating:private"; empty: all images.
    Q_INVOKABLE void search(const QString& text);
    // The same search from the start, e.g. after an upload.
    Q_INVOKABLE void reload();
    // The next page, if there is one and none is loading; the grid calls it as it nears the end.
    Q_INVOKABLE void loadMore();

    QString searchText() const;
    Order order() const;
    void setOrder(Order order);
    bool isLoading() const;
    QString errorMessage() const;
    QString effectiveQuery() const;  // the search with the hidden filters added; the app does not show it
    int count() const;               // loaded so far
    int totalCount() const;          // over all pages; -1 until the server has said, or if it could not
    bool hasMoreToLoad() const;      // the server has more results than the pages loaded so far
    std::optional<server_client::Image> imageAt(int row) const;  // nothing for a row that is not there
    // The image with the same id gets this one's data, e.g. after its tags changed on the server.
    void replaceImage(const server_client::Image& image);

    // Selecting images for changing their tags at once. There is a selection only while selecting: selecting an
    // image starts it, stopping empties the selection, and so does logging out. The selection is kept by image,
    // so it stays while pages load; a new search empties it and goes on selecting.
    Q_INVOKABLE void startSelecting();
    Q_INVOKABLE void stopSelecting();
    bool isSelecting() const;
    Q_INVOKABLE void toggleSelected(int row);  // a click while selecting, or Ctrl+click; also where a range starts
    // Shift+click: the images from where the last range started to this one; added to the selection, or
    // instead of it.
    Q_INVOKABLE void selectRange(int row, bool addToSelection);
    Q_INVOKABLE void clearSelection();
    QList<server_client::Image> selectedImages() const;  // in the order of the results

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

Q_SIGNALS:
    void searchChanged();
    void loadingChanged();
    void errorMessageChanged();
    void countChanged();
    void totalCountChanged();
    void selectionChanged();
    void selectingChanged();

private:
    bool isLoggedIn() const;
    void loadPage(const QString& cursor);
    void loadTotalCount();
    void setTotalCount(int totalCount);
    void setSelection(const QSet<int>& imageIds);
    void setSelecting(bool selecting);
    void addPage(const server_client::ImagePage& page);
    void clearImages();
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);
    server_client::ImageQuery queryFor(const QString& cursor) const;

    connection::Connection* m_connection;
    Content m_content;
    QString m_albumName;  // none yet: nothing to show
    QList<server_client::Image> m_images;
    QString m_searchText;
    Order m_order = Order::NewestFirst;
    qint64 m_randomSeed = 0;  // a random order stays the same while paging
    QString m_nextCursor;
    bool m_complete = true;  // no more pages to load
    bool m_loading = false;
    int m_searchNumber = 0;  // answers to an older search are ignored
    QString m_errorMessage;
    QString m_effectiveQuery;
    int m_totalCount = -1;
    QSet<int> m_selectedImageIds;
    int m_rangeStartRow = -1;  // none yet
    bool m_selecting = false;
};

}  // namespace photobook::library
