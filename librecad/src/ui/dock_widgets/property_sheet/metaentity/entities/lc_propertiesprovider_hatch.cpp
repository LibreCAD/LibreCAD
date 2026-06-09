/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_propertiesprovider_hatch.h"

#include "lc_property_qstring_list_combobox_view.h"
#include "rs_hatch.h"
#include "rs_patternlist.h"

void LC_PropertiesProviderHatch::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto cont = createGeometrySection(container);

    addBoolean<RS_Hatch>({"solid", tr("Is Solid"), tr("Determines whether hatch is solid or not")}, [](const RS_Hatch* e) -> bool {
                             return e->isSolid();
                         }, [](const bool& v, RS_Hatch* e) -> void {
                             e->setSolid(v);
                         }, list, cont);

    addStringList<RS_Hatch>({"pattern", tr("Pattern"), tr("Hatch pattern name")}, [](const RS_Hatch* e) -> QString {
                                return e->getPattern();
                            }, [](const QString& v, RS_Hatch* l) -> void {
                                l->setPattern(v);
                            }, []([[maybe_unused]] RS_Hatch* h, LC_PropertyViewDescriptor& descriptor)-> bool {
                                QStringList values;
                                for (const auto& [fst, snd] : *RS_PATTERNLIST) {
                                    values.append(fst);
                                }
                                values.sort();
                                descriptor[LC_PropertyQStringListComboBoxView::ATTR_ITEMS] = values;
                                return values.isEmpty();
                            }, list, cont);

    addLinearDistance<RS_Hatch>({"scale", tr("Scale"), tr("Hatch scale")}, [](const RS_Hatch* e) -> double {
                                    return e->getScale();
                                }, [](const double& v, RS_Hatch* l) -> void {
                                    l->setScale(v);
                                }, list, cont);

    addWCSAngle<RS_Hatch>({"angle", tr("Angle"), tr("Hatch rotation angle")}, [](const RS_Hatch* e) -> double {
                              return e->getAngle();
                          }, [](const double& v, RS_Hatch* l) -> void {
                              l->setAngle(v);
                          }, list, cont);
}

void LC_PropertiesProviderHatch::doCreateCalculatedProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addReadOnlyString<RS_Hatch>({"area", tr("Area"), tr("Area of hatch")}, [this](const RS_Hatch* e) -> QString {
        const double area = e->getTotalArea();
        QString value = formatLinear(area);
        return value;
    }, list, container);
}

void LC_PropertiesProviderHatch::fillSingleEntityCommands([[maybe_unused]] LC_PropertyContainer* container,
                                                          [[maybe_unused]] const QList<RS_Entity*>& entitiesList) {
}
