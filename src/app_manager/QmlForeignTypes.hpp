/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include "Albums.hpp"
#include "BulkTagEditor.hpp"
#include "Connection.hpp"
#include "HiddenFilterEditor.hpp"
#include "ImageActions.hpp"
#include "Library.hpp"
#include "LibraryView.hpp"
#include "Navigator.hpp"
#include "SearchSuggestions.hpp"
#include "Slideshow.hpp"
#include "TagEditor.hpp"
#include "TagRowsEditor.hpp"
#include "TagSuggestions.hpp"
#include "UploadTagEditor.hpp"
#include "Uploader.hpp"
#include "ViewTransform.hpp"
#include "Viewer.hpp"

// Makes service classes known to QML without the services depending on QML. QML only receives
// these objects from the AppManager; it cannot create them.

namespace photobook::app_manager
{

struct ForeignViewer
{
    Q_GADGET
    QML_FOREIGN(photobook::viewer::Viewer)
    QML_NAMED_ELEMENT(Viewer)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignViewTransform
{
    Q_GADGET
    QML_FOREIGN(photobook::view_transform::ViewTransform)
    QML_NAMED_ELEMENT(ViewTransform)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignNavigator
{
    Q_GADGET
    QML_FOREIGN(photobook::navigator::Navigator)
    QML_NAMED_ELEMENT(Navigator)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignSlideshow
{
    Q_GADGET
    QML_FOREIGN(photobook::slideshow::Slideshow)
    QML_NAMED_ELEMENT(Slideshow)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignConnection
{
    Q_GADGET
    QML_FOREIGN(photobook::connection::Connection)
    QML_NAMED_ELEMENT(Connection)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignLibrary
{
    Q_GADGET
    QML_FOREIGN(photobook::library::Library)
    QML_NAMED_ELEMENT(Library)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignLibraryView
{
    Q_GADGET
    QML_FOREIGN(photobook::library_view::LibraryView)
    QML_NAMED_ELEMENT(LibraryView)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignTagEditor
{
    Q_GADGET
    QML_FOREIGN(photobook::tag_editor::TagEditor)
    QML_NAMED_ELEMENT(TagEditor)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignBulkTagEditor
{
    Q_GADGET
    QML_FOREIGN(photobook::tag_editor::BulkTagEditor)
    QML_NAMED_ELEMENT(BulkTagEditor)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignUploadTagEditor
{
    Q_GADGET
    QML_FOREIGN(photobook::tag_editor::UploadTagEditor)
    QML_NAMED_ELEMENT(UploadTagEditor)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignImageActions
{
    Q_GADGET
    QML_FOREIGN(photobook::image_actions::ImageActions)
    QML_NAMED_ELEMENT(ImageActions)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignUploader
{
    Q_GADGET
    QML_FOREIGN(photobook::uploader::Uploader)
    QML_NAMED_ELEMENT(Uploader)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignHiddenFilterEditor
{
    Q_GADGET
    QML_FOREIGN(photobook::tag_editor::HiddenFilterEditor)
    QML_NAMED_ELEMENT(HiddenFilterEditor)
    QML_UNCREATABLE("Provided by AppManager")
};

// The parts the edit views' rows share; QML reads Change from here
struct ForeignTagRowsEditor
{
    Q_GADGET
    QML_FOREIGN(photobook::tag_editor::TagRowsEditor)
    QML_NAMED_ELEMENT(TagRowsEditor)
    QML_UNCREATABLE("A base of the tag editors and HiddenFilterEditor")
};

struct ForeignAlbums
{
    Q_GADGET
    QML_FOREIGN(photobook::library::Albums)
    QML_NAMED_ELEMENT(Albums)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignSearchSuggestions
{
    Q_GADGET
    QML_FOREIGN(photobook::tag_editor::SearchSuggestions)
    QML_NAMED_ELEMENT(SearchSuggestions)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignTagSuggestions
{
    Q_GADGET
    QML_FOREIGN(photobook::tag_editor::TagSuggestions)
    QML_NAMED_ELEMENT(TagSuggestions)
    QML_UNCREATABLE("Provided by TagEditor and SearchSuggestions")
};

}  // namespace photobook::app_manager
