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

#include "lc_propertiesprovider_dim_ordinate.h"

#include <QTimer>

#include "lc_actionselectdimordinatesameorigin.h"
#include "lc_dimordinate.h"
#include "lc_property_action.h"
#include "lc_property_action_link_view.h"
#include "rs_selection.h"

void LC_PropertiesProviderDimOrdinate::doCreateDimGeometrySection(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addVector<LC_DimOrdinate>({"dimFeaturePoint", tr("Feature point"), tr("Position of feature point")},
                              [](const LC_DimOrdinate* e) -> RS_Vector {
                                  return e->getFeaturePoint();
                              }, [](const RS_Vector& v, LC_DimOrdinate* e) -> void {
                                  e->setFeaturePoint(v);
                              }, list, container);

    addVector<LC_DimOrdinate>({"dimLeaderEndPoint", tr("Leader end point"), tr("Position of leader end point")},
                              [](const LC_DimOrdinate* e) -> RS_Vector {
                                  return e->getLeaderEndPoint();
                              }, [](const RS_Vector& v, LC_DimOrdinate* e) -> void {
                                  e->setLeaderPoint(v);
                              }, list, container);

    addBoolean<LC_DimOrdinate>({"dimForX", tr("Is for X axis"), tr("Defines whether ordinate dimension is for X axis")},
                               [](const LC_DimOrdinate* e) -> bool {
                                   return e->isForXDirection();
                               }, [](const bool& v, LC_DimOrdinate* e) -> void {
                                   e->setForXDirection(v);
                               }, list, container);
}

void LC_PropertiesProviderDimOrdinate::rebaseDimensions(const LC_GraphicViewport* viewport, const double horizontalDirection,
                                                        const RS_Vector& origin, const QList<RS_Entity*>& list) const {
    for (const auto e : list) {
        const auto d = dynamic_cast<LC_DimOrdinate*>(e);
        if (d != nullptr) {
            if (viewport != nullptr) {
                const auto clone = static_cast<LC_DimOrdinate*>(d->clone());
                clone->setHDir(horizontalDirection);
                clone->setDefinitionPoint(origin);
                clone->clearSelectionFlag();
                clone->update();
                m_widget->entityModified(d, clone);
            }
        }
    }
}

void LC_PropertiesProviderDimOrdinate::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto dim = static_cast<LC_DimOrdinate*>(entity);
    auto viewport = m_actionContext->getViewport();
   auto doc = m_actionContext->getDocument();
    auto clickHandler = [this, viewport, doc]([[maybe_unused]] LC_DimOrdinate* ent, const int linkIndex) {
        switch (linkIndex) {
            case 0: {
                m_actionContext->saveContextMenuActionContext(ent, ent->getMiddlePoint(), false);
                m_actionContext->setCurrentAction(RS2::ActionDimStyleApply, nullptr);
                break;
            }
            case 1: {
                if (viewport != nullptr) {
                    const QList<RS_Entity*> ordinateDims =
                        LC_ActionSelectDimOrdinateSameOrigin::collectOrdinateDimensionsWithSameBase(ent, doc);
                    if (!ordinateDims.isEmpty()) {
                        RS_Selection::selectEntitiesList(doc, viewport, ordinateDims, true);
                    }
                }
                break;
            }
            default:
                break;
        }
    };
    LC_PropertyProviderUtils::createSingleEntityCommand<LC_DimOrdinate>(cont, "dimOrdCommands", tr("Apply style"),
                                                                        tr("Applies dimension style to other dimensions"),
                                                                        tr("Select by base"),
                                                                        tr("Selects all ordinate dimensions with the same base"), dim,
                                                                        clickHandler, tr("Ordinate dimension operations"));

}

void LC_PropertiesProviderDimOrdinate::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_PropertiesProviderDimBase::doCreateSelectedSetCommands(propertyContainer, list);
    auto viewport = m_actionContext->getViewport();
    const RS_Graphic* g = m_actionContext->getGraphicView()->getGraphic(false);
    if (g != nullptr) {
        // not in block editor
        double horizontalDirection = 0;
        RS_Vector origin{false};
        viewport->fillCurrentUCSInfo(origin, horizontalDirection);

        auto rebaseProperty = new LC_PropertyAction(propertyContainer, true);

        const LC_UCS* currentUCS = g->getCurrentUCS();
        QString ucsName = currentUCS->getName();
        if (ucsName.isEmpty()) {
            ucsName = tr("<No name>");
        }
        delete currentUCS;

        const LC_Property::Names names = {
            "dimReBase",
            tr("Re-Base to current UCS") + " (" + ucsName + ")",
            tr("Rebase selected ordinate dimensions to current UCS - ") + ucsName
        };
        rebaseProperty->setNames(names);
        LC_PropertyViewDescriptor viewDescriptor("Link");
        viewDescriptor[LC_PropertyActionLinkView::ATTR_TITLE] = names.displayName;
        rebaseProperty->setClickHandler(
            [this, viewport, origin, list,horizontalDirection, rebaseProperty]([[maybe_unused]] const LC_PropertyAction* action, [[maybe_unused]]int linkIndex) {
                QTimer::singleShot(20, [this, viewport,list, origin, horizontalDirection, rebaseProperty]()-> void {
                    rebaseDimensions(viewport, horizontalDirection, origin, list);
                    m_widget->onPropertyEdited(rebaseProperty);
                });
            });
        rebaseProperty->setViewDescriptor(viewDescriptor);
        propertyContainer->addChildProperty(rebaseProperty);
    }
}
