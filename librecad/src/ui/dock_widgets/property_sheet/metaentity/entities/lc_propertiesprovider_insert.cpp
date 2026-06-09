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

#include "lc_propertiesprovider_insert.h"

#include "lc_property_double_interactivepick_view.h"
#include "rs_block.h"
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_insert.h"

void LC_PropertiesProviderInsert::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addStringList<RS_Insert>({"name", tr("Block"), tr("Block name")}, [](const RS_Insert* e) -> QString {
                                 return e->getName();
                             }, [](const QString& v, RS_Insert* l) -> void {
                                 l->setName(v);
                             }, [this]([[maybe_unused]] RS_Insert* h, LC_PropertyViewDescriptor& descriptor)-> bool {
                                 const auto graphic = this->m_actionContext->getDocument()->getGraphic();
                                 if (graphic != nullptr) {
                                     QStringList values;
                                     const auto blockList = graphic->getBlockList();
                                     for (const auto& b : *blockList) {
                                         values.append(b->getName());
                                     }
                                     values.sort();
                                     descriptor[LC_PropertyQStringListComboBoxView::ATTR_ITEMS] = values;
                                     return values.isEmpty();
                                 }
                                 return true;
                             }, list, contGeometry);

    addVector<RS_Insert>({"insert", tr("Insertion Point"), tr("Point of block insertion")}, [](const RS_Insert* e) -> RS_Vector {
                             return e->getInsertionPoint();
                         }, [](const RS_Vector& v, RS_Insert* e) -> void {
                             e->setInsertionPoint(v);
                         }, list, contGeometry);

    addLinearDistance<RS_Insert>({"scaleX", tr("Scale X"), tr("Scale factor of block by X")}, [](const RS_Insert* e) -> double {
                                     return e->getScale().x;
                                 }, [](const double& v, RS_Insert* l) -> void {
                                     RS_Vector scale = l->getScale();
                                     scale.setX(v);
                                     l->setScale(scale);
                                 }, list, contGeometry, [](LC_PropertyViewDescriptor* desc) -> void {
                                     desc->attributes[LC_PropertyDoubleInteractivePickView::ATTR_POSITIVIE_VALUES_ONLY] = true;
                                     desc->attributes[LC_PropertyDoubleInteractivePickView::ATTR_NON_MEANINGFUL_DISTANCE] = 1.0;
                                 });

    addLinearDistance<RS_Insert>({"scaleY", tr("Scale Y"), tr("Scale factor of block by Y")}, [](const RS_Insert* e) -> double {
                                     return e->getScale().y;
                                 }, [](const double& v, RS_Insert* l) -> void {
                                     RS_Vector scale = l->getScale();
                                     scale.setY(v);
                                     l->setScale(scale);
                                 }, list, contGeometry, [](LC_PropertyViewDescriptor* desc) -> void {
                                     desc->attributes[LC_PropertyDoubleInteractivePickView::ATTR_POSITIVIE_VALUES_ONLY] = true;
                                     desc->attributes[LC_PropertyDoubleInteractivePickView::ATTR_NON_MEANINGFUL_DISTANCE] = 1.0;
                                 });

    addWCSAngle<RS_Insert>({"angle", tr("Angle"), tr("Block rotation angle")}, [](const RS_Insert* e) -> double {
                               return e->getAngle();
                           }, [](const double& v, RS_Insert* l) -> void {
                               l->setAngle(v);
                           }, list, contGeometry);

    addIntSpinbox<RS_Insert>({"cols", tr("Columns"), tr("Amount of array columns")}, [](const RS_Insert* e) -> int {
                                 const int value = e->getCols();
                                 return value;
                             }, [](const int& value, RS_Insert* e) -> void {
                                 e->setCols(value);
                             }, list, contGeometry);

    addIntSpinbox<RS_Insert>({"cols", tr("Rows"), tr("Amount of array rows")}, [](const RS_Insert* e) -> int {
                                 const int value = e->getRows();
                                 return value;
                             }, [](const int& value, RS_Insert* e) -> void {
                                 e->setRows(value);
                             }, list, contGeometry);

    addLinearDistance<RS_Insert>({"spacingX", tr("Spacing X"), tr("Spacing between columns of array (by X)")},
                                 [](const RS_Insert* e) -> double {
                                     return e->getSpacing().x;
                                 }, [](const double& v, RS_Insert* l) -> void {
                                     RS_Vector spacing = l->getSpacing();
                                     spacing.setX(v);
                                     l->setSpacing(spacing);
                                 }, list, contGeometry);

    addLinearDistance<RS_Insert>({"spacingY", tr("Spacing Y"), tr("Spacing between rows of array (by Y)")},
                                 [](const RS_Insert* e) -> double {
                                     return e->getSpacing().y;
                                 }, [](const double& v, RS_Insert* l) -> void {
                                     RS_Vector spacing = l->getSpacing();
                                     spacing.setY(v);
                                     l->setSpacing(spacing);
                                 }, list, contGeometry);
}

void LC_PropertiesProviderInsert::fillComputedProperites([[maybe_unused]]LC_PropertyContainer* container, [[maybe_unused]]const QList<RS_Entity*>& entitiesList) {
}

void LC_PropertiesProviderInsert::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Block exploding and editing"),
            {RS2::ActionBlocksExplode, tr("Explode"), tr("Explodes insert to individual entities of the block")},
            {RS2::ActionBlocksEdit, tr("Edit block"), tr("Performs editing of insert's block")}
        }
    };
    const auto insert = static_cast<RS_Insert*>(entity);
    createEntityContextCommands<RS_Insert>(commands, cont, insert, "insertCommands", true);
}
