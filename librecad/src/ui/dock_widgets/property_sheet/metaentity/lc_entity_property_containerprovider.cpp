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

#include "lc_entity_property_containerprovider.h"

#include "lc_propertiesprovider_arc.h"
#include "lc_propertiesprovider_circle.h"
#include "lc_propertiesprovider_dim_aligned.h"
#include "lc_propertiesprovider_dim_angular.h"
#include "lc_propertiesprovider_dim_arc.h"
#include "lc_propertiesprovider_dim_diametric.h"
#include "lc_propertiesprovider_dim_linear.h"
#include "lc_propertiesprovider_dim_ordinate.h"
#include "lc_propertiesprovider_dim_radial.h"
#include "lc_propertiesprovider_ellipse.h"
#include "lc_propertiesprovider_hatch.h"
#include "lc_propertiesprovider_image.h"
#include "lc_propertiesprovider_insert.h"
#include "lc_propertiesprovider_leader.h"
#include "lc_propertiesprovider_line.h"
#include "lc_propertiesprovider_mtext.h"
#include "lc_propertiesprovider_multiple.h"
#include "lc_propertiesprovider_parabola.h"
#include "lc_propertiesprovider_point.h"
#include "lc_propertiesprovider_polyline.h"
#include "lc_propertiesprovider_spline.h"
#include "lc_propertiesprovider_splinepoints.h"
#include "lc_propertiesprovider_text.h"
#include "lc_propertiesprovider_tolerance.h"
#include "rs_graphicview.h"

void LC_EntityPropertyContainerProvider::init(LC_PropertySheetWidget* widget, LC_ActionContext* context) {
    m_multiple = std::make_unique<LC_PropertiesProviderMultiple>(context, widget);
    m_line = std::make_unique<LC_PropertiesProviderLine>(context, widget);
    m_circle = std::make_unique<LC_PropertiesProviderCircle>(context, widget);
    m_arc = std::make_unique<LC_PropertiesProviderArc>(context, widget);
    m_ellipse = std::make_unique<LC_PropertiesProviderEllipse>(context, widget);
    m_hyperbola = std::make_unique<LC_PropertiesProviderHyperbola>(context, widget);
    m_polyline = std::make_unique<LC_PropertiesProviderPolyline>(context, widget);
    m_point = std::make_unique<LC_PropertiesProviderPoint>(context, widget);
    m_spline = std::make_unique<LC_PropertiesProviderSpline>(context, widget);
    m_splinePoints = std::make_unique<LC_PropertiesProviderSplinePoints>(context, widget);
    m_hatch = std::make_unique<LC_PropertiesProviderHatch>(context, widget);
    m_insert = std::make_unique<LC_PropertiesProviderInsert>(context, widget);
    m_text = std::make_unique<LC_PropertiesProviderText>(context, widget);
    m_MText = std::make_unique<LC_PropertiesProviderMText>(context, widget);
    m_image = std::make_unique<LC_PropertiesProviderImage>(context, widget);
    m_dimAligned = std::make_unique<LC_PropertiesProviderDimAligned>(context, widget);
    m_dimLinear = std::make_unique<LC_PropertiesProviderDimLinear>(context, widget);
    m_dimRadial = std::make_unique<LC_PropertiesProviderDimRadial>(context, widget);
    m_dimDiametric = std::make_unique<LC_PropertiesProviderDimDiametric>(context, widget);
    m_dimAngular = std::make_unique<LC_PropertiesProviderDimAngular>(context, widget);
    m_dimArc = std::make_unique<LC_PropertiesProviderDimArc>(context, widget);
    m_dimOrdinate = std::make_unique<LC_PropertiesProviderDimOrdinate>(context, widget);
    m_document = std::make_unique<LC_PropertiesProviderDocument>(context, widget);
    m_leader = std::make_unique<LC_PropertiesProviderLeader>(context, widget);
    m_tolerance = std::make_unique<LC_PropertiesProviderTolerance>(context, widget);
    m_parabola = std::make_unique<LC_PropertiesProviderParabola>(context, widget);
}

void LC_EntityPropertyContainerProvider::fillPropertyContainerForSelection(RS_Document* doc, LC_PropertyContainer* container,
                                                               const RS2::EntityType entityType, const QList<RS_Entity*>& entitiesList) {
    m_entityType = entityType;
    if (!m_entitiesList.empty()) {
        m_entitiesList.clear();
    }
    m_entitiesList = entitiesList;
    if (container != nullptr) {
        refillPropertyContainer(doc, container);
        container->setTag((m_entitiesList.size() > 1) ? TAG_CONTAINER_SELECTION_MANY : TAG_CONTAINER_SELECTION_ONE);
    }
}

void LC_EntityPropertyContainerProvider::fillPropertyContainerForNoSelection([[maybe_unused]] RS_Document* doc, LC_PropertyContainer* container) {
    if (!m_entitiesList.empty()) {
        m_entitiesList.clear();
    }
    if (container == nullptr) {
        return;
    }
    m_document->fillDocumentProperties(container);
    container->setTag(TAG_CONTAINER_NO_SELECTION);
}

void LC_EntityPropertyContainerProvider::fillPropertyContainerToolOptions([[maybe_unused]] RS_Document* doc,
                                                                          LC_PropertyContainer* container,
                                                                          LC_ToolOptionsPropertiesContainerProvider* toolOptionsProvider,
                                                                          bool showToolOptions) {
    if (!m_entitiesList.empty()) {
        m_entitiesList.clear();
    }
    if (container == nullptr) {
        return;
    }
    if (toolOptionsProvider != nullptr) {
        createToolOptions(container, toolOptionsProvider, showToolOptions);
    }
    m_document->fillDocumentPropertiesForToolOptions(container);
}

void LC_EntityPropertyContainerProvider::createToolOptions(LC_PropertyContainer* container, LC_ToolOptionsPropertiesContainerProvider* toolOptionsProvider, bool showToolOptions) {
    if (toolOptionsProvider->hasSnapOptions()) {
        LC_PropertyContainer* toolOptionsSectionContainer = m_document->createSnapToolOptionsSection(container);
        toolOptionsProvider->fillSnapToolOptionsContainer(toolOptionsSectionContainer);
    }
    if (showToolOptions) {
        LC_PropertyContainer* toolOptionsSectionContainer = m_document->createToolOptionsSection(container);
        toolOptionsProvider->fillToolOptionsContainer(toolOptionsSectionContainer);
    }
}

void LC_EntityPropertyContainerProvider::refillPropertyContainer([[maybe_unused]] RS_Document* doc, LC_PropertyContainer* container) const {
    switch (m_entityType) {
        case RS2::EntityUnknown: {
            m_multiple->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityLine: {
            m_line->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityCircle: {
            m_circle->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityArc: {
            m_arc->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityEllipse: {
            m_ellipse->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityHyperbola: {
            m_hyperbola->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityPolyline: {
            m_polyline->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityPoint: {
            m_point->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntitySpline: {
            m_spline->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntitySplinePoints: {
            m_splinePoints->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityHatch: {
            m_hatch->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityInsert: {
            m_insert->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityText: {
            m_text->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityMText: {
            m_MText->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityImage: {
            m_image->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityDimAligned: {
            m_dimAligned->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityDimLinear: {
            m_dimLinear->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityDimRadial: {
            m_dimRadial->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityDimDiametric: {
            m_dimDiametric->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityDimAngular: {
            m_dimAngular->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityDimArc: {
            m_dimArc->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityDimOrdinate: {
            m_dimOrdinate->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityDimLeader: {
            m_leader->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityTolerance: {
            m_tolerance->fillEntityProperties(container, m_entitiesList);
            break;
        }
        case RS2::EntityParabola: {
            m_parabola->fillEntityProperties(container, m_entitiesList);
            break;
        }
        default:
            m_multiple->fillEntityProperties(container, m_entitiesList);
            break;
    }
}

void LC_EntityPropertyContainerProvider::setGraphicView([[maybe_unused]]RS_GraphicView* gview) {
}

void LC_EntityPropertyContainerProvider::clearCachedDimStyles() {
    if (RS2::isDimensionalEntity(m_entityType)) {
        for (const auto d : std::as_const(m_entitiesList)) {
            auto* dim = static_cast<RS_Dimension*>(d);
            dim->clearCachedDimStyle();
        }
    }
}

void LC_EntityPropertyContainerProvider::cleanup() {
    clearCachedDimStyles();
    m_entitiesList.clear();
}

void LC_EntityPropertyContainerProvider::clearEntities() {
    m_entitiesList.clear();
}
