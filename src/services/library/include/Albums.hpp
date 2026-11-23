/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <optional>

#include <QAbstractListModel>
#include <QList>

#include "ApiTypes.hpp"
#include "Connection.hpp"
#include "Settings.hpp"

namespace photobook::library
{

// The albums the user can see, for the Albums tab: one per album: tag with an image they may see, with its first
// image as the cover. Loads when the user logs in and on reload(), empties when they log out. The sort and
// whether albums with hidden images are listed are remembered; the filter is not.
class Albums : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(Sort sort READ sort WRITE setSort NOTIFY sortChanged)
    Q_PROPERTY(bool includeHidden READ includesHidden WRITE setIncludeHidden NOTIFY includeHiddenChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countChanged)

public:
    enum class Sort
    {
        RecentlyChanged,  // an image added last first
        Name,
        Size,  // most images first
    };
    Q_ENUM(Sort)

    enum Role
    {
        TagRole = Qt::UserRole + 1,  // e.g. "album:vacation_2015"
        NameRole,                    // as shown, e.g. "vacation 2015"
        ImageCountRole,
        CoverUrlRole,       // the small thumbnail of the first image; loading it needs the login token
        LargeCoverUrlRole,  // for large tiles
        LastChangedAtRole,
    };

    // Neither is owned; both must outlive the albums.
    Albums(connection::Connection* connection, settings::Settings* settings, QObject* parent = nullptr);

    Q_INVOKABLE void reload();

    Sort sort() const;
    void setSort(Sort sort);
    // "Show albums with hidden images": also albums with an image the user's hidden filters match
    bool includesHidden() const;
    void setIncludeHidden(bool include);
    // Only albums whose name contains it, in any case; "_" and " " match each other.
    QString filterText() const;
    void setFilterText(const QString& text);

    bool isLoading() const;
    QString errorMessage() const;
    int count() const;                                           // shown, after the filter
    int totalCount() const;                                      // loaded, before the filter
    std::optional<server_client::Album> albumAt(int row) const;  // nothing for a row that is not there

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void sortChanged();
    void includeHiddenChanged();
    void filterTextChanged();
    void loadingChanged();
    void errorMessageChanged();
    void countChanged();

private:
    bool isLoggedIn() const;
    void setAlbums(const QList<server_client::Album>& albums);
    void showAlbums();  // sorted and filtered
    void setLoading(bool loading);
    void setErrorMessage(const QString& message);

    connection::Connection* m_connection;
    settings::Settings* m_settings;
    QList<server_client::Album> m_albums;  // as loaded
    QList<server_client::Album> m_shown;
    Sort m_sort = Sort::RecentlyChanged;
    bool m_includeHidden = false;
    QString m_filterText;
    bool m_loading = false;
    QString m_errorMessage;
    int m_loadNumber = 0;  // answers to an older load are ignored
};

}  // namespace photobook::library
