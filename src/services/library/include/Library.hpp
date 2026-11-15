/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QAbstractListModel>
#include <QList>

#include "ApiTypes.hpp"
#include "Connection.hpp"

namespace pictorio::library
{

// The images of a search on the server, as a list for the library grid. The first page loads with the
// search; the grid asks for more through canFetchMore() and fetchMore() as it scrolls. Follows the
// connection: loads when the user logs in, empties when they log out.
class Library : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString searchText READ searchText NOTIFY searchChanged)
    Q_PROPERTY(Order order READ order WRITE setOrder NOTIFY searchChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString effectiveQuery READ effectiveQuery NOTIFY effectiveQueryChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

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
        ThumbnailUrlRole,  // the small thumbnail; loading it needs the login token
        TakenAtRole,
        WidthRole,
        HeightRole,
    };

    static constexpr int pageSize = 100;

    // The connection is not owned and must outlive the library.
    explicit Library(connection::Connection* connection, QObject* parent = nullptr);

    // The server's search syntax, e.g. "category:photo summer -rating:nsfw"; empty: all images.
    Q_INVOKABLE void search(const QString& text);
    // The same search from the start, e.g. after an upload.
    Q_INVOKABLE void reload();

    QString searchText() const;
    Order order() const;
    void setOrder(Order order);
    bool isLoading() const;
    QString errorMessage() const;
    QString effectiveQuery() const;  // the search with the user's hidden-by-default filters added
    int count() const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

Q_SIGNALS:
    void searchChanged();
    void loadingChanged();
    void errorMessageChanged();
    void effectiveQueryChanged();
    void countChanged();

private:
    bool isLoggedIn() const;
    void loadPage(const QString& cursor);
    void addPage(const server_client::ImagePage& page);
    void clearImages();
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);
    server_client::ImageQuery queryFor(const QString& cursor) const;

    connection::Connection* m_connection;
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
};

}  // namespace pictorio::library
