/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
****************************************************************************/

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>

#include "lc_actioncontext.h"
#include "lc_actiongroupmanager.h"
#include "lc_actiontestsupport.h"
#include "lc_properties_sheet.h"
#include "lc_property_container.h"
#include "lc_property_multi.h"
#include "lc_property_qstring.h"
#include "lc_propertysheet_widget_options.h"
#include "lc_propertysheetwidget.h"
#include "rs_arc.h"
#include "rs_graphic.h"
#include "rs_math.h"

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

/// the number a row shows, without its unit sign (the sheet wraps the property of each selected entity)
double shownNumber(LC_Property* property) {
    const auto* multi = dynamic_cast<LC_PropertyMulti*>(property);
    REQUIRE(multi != nullptr);
    const auto* text = dynamic_cast<LC_PropertyQString*>(multi->getFirstProperty());
    REQUIRE(text != nullptr);
    QString shown = text->value();
    shown.remove(QChar(0x00B0));
    return shown.toDouble();
}

}

TEST_CASE("The arc panel shows the arc's linear length as Arc Length and its swept angle as Angle Length",
          "[gui][properties][arc]") {
    (void)lc::test::application();
    LC_ActionContext context;
    LC_ActionGroupManager actions(nullptr);
    LC_PropertySheetWidget sheet(nullptr, &context, &actions); // before the drawing, so that it is destroyed after it
    sheet.getOptions()->noSelectionWorkspace = false;
    sheet.getOptions()->noSelectionGraphicView = false;
    sheet.getOptions()->showComputed = true;

    RS_Graphic graphic;
    graphic.initForNewDocument(); // the layer 0, which the sheet paints in its Layer row
    // radius 30 from 20 to 150 degrees: 130 degrees, which is 30 * 130 degrees in radians = 68.07 long
    auto* arc = new RS_Arc(&graphic, RS_ArcData(RS_Vector(0, 0), 30., RS_Math::deg2rad(20.), RS_Math::deg2rad(150.), false));
    graphic.addEntity(arc);
    graphic.select(QList<RS_Entity*>{arc}, true);
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
    LC_Property* length = findByDisplayName(properties->getPropertyContainer(), "Arc Length");
    REQUIRE(length != nullptr);
    CHECK(shownNumber(length) == Catch::Approx(arc->getLength()).margin(0.001));
    LC_Property* angle = findByDisplayName(properties->getPropertyContainer(), "Angle Length");
    REQUIRE(angle != nullptr);
    CHECK(shownNumber(angle) == Catch::Approx(130.).margin(0.001));

    sheet.setGraphicView(nullptr);
}
