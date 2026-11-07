/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD (librecad.org)
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
**********************************************************************/

// Issue #1774: checking "Is Closed" for a polyline in the Properties sheet only
// set the closed flag. The segment that closes the outline was not added, so
// the polyline kept being drawn open until the drawing was saved and reopened;
// unchecking it left the closing segment in place.

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QList>

#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_properties_sheet.h"
#include "lc_property_bool.h"
#include "lc_property_container.h"
#include "lc_property_multi.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_propertysheetwidget.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_polyline.h"

namespace {

/// the row shown under this name, searched through all sections of the sheet
LC_Property* findByDisplayName(LC_PropertyContainer* container, const QString& displayName) {
    for (LC_Property* property : container->childProperties()) {
        if (property->getDisplayName() == displayName) {
            return property;
        }
        if (LC_PropertyContainer* section = property->asContainer()) {
            if (LC_Property* found = findByDisplayName(section, displayName)) {
                return found;
            }
        }
    }
    return nullptr;
}

/// The sheet's view reports a finished edit, and the sheet applies it from a short timer: do both.
void finishEdit(LC_PropertySheetWidget& sheet, LC_Property* edited) {
    sheet.onPropertyEdited(edited);
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 150) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
}

/// The sheet edits a clone and swaps it for the original, so ask the drawing for its polyline.
RS_Polyline* polylineOf(RS_Graphic& graphic) {
    for (unsigned i = 0; i < graphic.count(); ++i) {
        auto* polyline = dynamic_cast<RS_Polyline*>(graphic.entityAt(i));
        if (polyline != nullptr && !polyline->isDeleted()) {
            return polyline;
        }
    }
    return nullptr;
}

} // namespace

TEST_CASE("Checking Is Closed in the sheet draws the polyline closed, and unchecking opens it again",
          "[gui][properties][polyline][issue1774]") {
    (void)lc::test::application();
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions); // before the drawing, so that it is destroyed after it
    sheet.getOptions()->noSelectionWorkspace = false;
    sheet.getOptions()->noSelectionGraphicView = false;
    sheet.getOptions()->showComputed = false;

    RS_Graphic graphic;
    graphic.initForNewDocument();
    // an open polyline through (0,0), (40,0), (40,30)
    auto* polyline = new RS_Polyline(&graphic, RS_PolylineData());
    polyline->addVertex(RS_Vector(0.0, 0.0));
    polyline->addVertex(RS_Vector(40.0, 0.0));
    polyline->addVertex(RS_Vector(40.0, 30.0));
    polyline->endPolyline();
    graphic.addEntity(polyline);
    REQUIRE_FALSE(polyline->isClosed());
    REQUIRE(polyline->count() == 2);

    graphic.select(QList<RS_Entity*>{polyline}, true);
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    context.setDocumentAndView(&graphic, &view);
    sheet.show();
    QCoreApplication::processEvents();
    sheet.setGraphicView(&view);
    QCoreApplication::processEvents();

    auto* properties = sheet.findChild<LC_PropertiesSheet*>();
    REQUIRE(properties != nullptr);
    REQUIRE(properties->getPropertyContainer() != nullptr);
    LC_Property* row = findByDisplayName(properties->getPropertyContainer(), "Is Closed");
    REQUIRE(row != nullptr);
    const auto* multi = dynamic_cast<LC_PropertyMulti*>(row);
    REQUIRE(multi != nullptr);
    auto* closed = dynamic_cast<LC_PropertyBool*>(multi->getFirstProperty());
    REQUIRE(closed != nullptr);

    closed->setValue(true);
    finishEdit(sheet, row);
    RS_Polyline* edited = polylineOf(graphic);
    REQUIRE(edited != nullptr);
    CHECK(edited->isClosed());
    // the closing segment from (40,30) back to (0,0) is drawn
    REQUIRE(edited->count() == 3);
    const auto* closing = dynamic_cast<const RS_Line*>(edited->last());
    REQUIRE(closing != nullptr);
    CHECK(closing->getStartpoint().distanceTo(RS_Vector(40.0, 30.0)) < 1e-6);
    CHECK(closing->getEndpoint().distanceTo(RS_Vector(0.0, 0.0)) < 1e-6);

    // the sheet rebuilds its rows for the edited polyline: uncheck the new one
    LC_Property* rowAfter = findByDisplayName(properties->getPropertyContainer(), "Is Closed");
    REQUIRE(rowAfter != nullptr);
    const auto* multiAfter = dynamic_cast<LC_PropertyMulti*>(rowAfter);
    REQUIRE(multiAfter != nullptr);
    auto* closedAfter = dynamic_cast<LC_PropertyBool*>(multiAfter->getFirstProperty());
    REQUIRE(closedAfter != nullptr);
    closedAfter->setValue(false);
    finishEdit(sheet, rowAfter);
    RS_Polyline* reopened = polylineOf(graphic);
    REQUIRE(reopened != nullptr);
    CHECK_FALSE(reopened->isClosed());
    CHECK(reopened->count() == 2);
    CHECK(reopened->getEndpoint().distanceTo(RS_Vector(40.0, 30.0)) < 1e-6);

    sheet.setGraphicView(nullptr);
}
