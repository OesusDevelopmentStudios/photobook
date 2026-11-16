/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 OesusDevelopmentStudios
 */

#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include "BulkTagEditor.hpp"
#include "Connection.hpp"
#include "Library.hpp"
#include "LibraryView.hpp"
#include "Navigator.hpp"
#include "Slideshow.hpp"
#include "TagEditor.hpp"
#include "TagRowsEditor.hpp"
#include "TagSuggestions.hpp"
#include "ViewTransform.hpp"
#include "Viewer.hpp"

// Makes service classes known to QML without the services depending on QML. QML only receives
// these objects from the AppManager; it cannot create them.

namespace pictorio::app_manager
{

struct ForeignViewer
{
    Q_GADGET
    QML_FOREIGN(pictorio::viewer::Viewer)
    QML_NAMED_ELEMENT(Viewer)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignViewTransform
{
    Q_GADGET
    QML_FOREIGN(pictorio::view_transform::ViewTransform)
    QML_NAMED_ELEMENT(ViewTransform)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignNavigator
{
    Q_GADGET
    QML_FOREIGN(pictorio::navigator::Navigator)
    QML_NAMED_ELEMENT(Navigator)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignSlideshow
{
    Q_GADGET
    QML_FOREIGN(pictorio::slideshow::Slideshow)
    QML_NAMED_ELEMENT(Slideshow)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignConnection
{
    Q_GADGET
    QML_FOREIGN(pictorio::connection::Connection)
    QML_NAMED_ELEMENT(Connection)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignLibrary
{
    Q_GADGET
    QML_FOREIGN(pictorio::library::Library)
    QML_NAMED_ELEMENT(Library)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignLibraryView
{
    Q_GADGET
    QML_FOREIGN(pictorio::library_view::LibraryView)
    QML_NAMED_ELEMENT(LibraryView)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignTagEditor
{
    Q_GADGET
    QML_FOREIGN(pictorio::tag_editor::TagEditor)
    QML_NAMED_ELEMENT(TagEditor)
    QML_UNCREATABLE("Provided by AppManager")
};

struct ForeignBulkTagEditor
{
    Q_GADGET
    QML_FOREIGN(pictorio::tag_editor::BulkTagEditor)
    QML_NAMED_ELEMENT(BulkTagEditor)
    QML_UNCREATABLE("Provided by AppManager")
};

// The parts the edit views' rows share; QML reads Change from here
struct ForeignTagRowsEditor
{
    Q_GADGET
    QML_FOREIGN(pictorio::tag_editor::TagRowsEditor)
    QML_NAMED_ELEMENT(TagRowsEditor)
    QML_UNCREATABLE("A base of TagEditor and BulkTagEditor")
};

struct ForeignTagSuggestions
{
    Q_GADGET
    QML_FOREIGN(pictorio::tag_editor::TagSuggestions)
    QML_NAMED_ELEMENT(TagSuggestions)
    QML_UNCREATABLE("Provided by TagEditor")
};

}  // namespace pictorio::app_manager
