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

#ifndef LC_ENTITYPROPERTYCONTAINERPROVIDER_H
#define LC_ENTITYPROPERTYCONTAINERPROVIDER_H

#include "lc_actioncontext.h"
#include "lc_entity_type_propertiesprovider.h"
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
#include "lc_propertiesprovider_hyperbola.h"
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
#include "lc_propertysheetwidget.h"
#include "entities/document/lc_propertiesprovider_document.h"

class LC_PropertySheetWidget;

class LC_EntityPropertyContainerProvider : public QObject, public LC_GraphicViewAware {
    Q_OBJECT
public:
    enum TAG {
        TAG_CONTAINER_NO_SELECTION,
        TAG_CONTAINER_SELECTION_MANY,
        TAG_CONTAINER_SELECTION_ONE
    };
    void init(LC_PropertySheetWidget* widget, LC_ActionContext* context);
    void fillPropertyContainerForSelection(RS_Document* doc, LC_PropertyContainer* container, RS2::EntityType entityType,
                               const QList<RS_Entity*>& entitiesList);
    void fillPropertyContainerForNoSelection(RS_Document* doc, LC_PropertyContainer* container);
    void fillPropertyContainerToolOptions(RS_Document* doc, LC_PropertyContainer* container, LC_ToolOptionsPropertiesContainerProvider* toolOptionsProvider, bool showToolOptions);
    void setGraphicView(RS_GraphicView* gview) override;
    void cleanup();
    void clearEntities();
private:
    void createToolOptions(LC_PropertyContainer* container, LC_ToolOptionsPropertiesContainerProvider* toolOptionsProvider, bool showToolOptions);
    void refillPropertyContainer(RS_Document* doc, LC_PropertyContainer* container) const;
    void clearCachedDimStyles();
    std::unique_ptr<LC_PropertiesProviderMultiple> m_multiple{nullptr};
    std::unique_ptr<LC_PropertiesProviderLine> m_line{nullptr};
    std::unique_ptr<LC_PropertiesProviderCircle> m_circle{nullptr};
    std::unique_ptr<LC_PropertiesProviderArc> m_arc{nullptr};
    std::unique_ptr<LC_PropertiesProviderEllipse> m_ellipse{nullptr};
    std::unique_ptr<LC_PropertiesProviderHyperbola> m_hyperbola{nullptr};
    std::unique_ptr<LC_PropertiesProviderPolyline> m_polyline{nullptr};
    std::unique_ptr<LC_PropertiesProviderPoint> m_point{nullptr};
    std::unique_ptr<LC_PropertiesProviderSpline> m_spline{nullptr};
    std::unique_ptr<LC_PropertiesProviderSplinePoints> m_splinePoints{nullptr};
    std::unique_ptr<LC_PropertiesProviderHatch> m_hatch{nullptr};
    std::unique_ptr<LC_PropertiesProviderInsert> m_insert{nullptr};
    std::unique_ptr<LC_PropertiesProviderText> m_text{nullptr};
    std::unique_ptr<LC_PropertiesProviderMText> m_MText{nullptr};
    std::unique_ptr<LC_PropertiesProviderImage> m_image{nullptr};
    std::unique_ptr<LC_PropertiesProviderDimAligned> m_dimAligned{nullptr};
    std::unique_ptr<LC_PropertiesProviderDimLinear> m_dimLinear{nullptr};
    std::unique_ptr<LC_PropertiesProviderDimRadial> m_dimRadial{nullptr};
    std::unique_ptr<LC_PropertiesProviderDimDiametric> m_dimDiametric{nullptr};
    std::unique_ptr<LC_PropertiesProviderDimAngular> m_dimAngular{nullptr};
    std::unique_ptr<LC_PropertiesProviderDimArc> m_dimArc{nullptr};
    std::unique_ptr<LC_PropertiesProviderDimOrdinate> m_dimOrdinate{nullptr};
    std::unique_ptr<LC_PropertiesProviderLeader> m_leader{nullptr};
    std::unique_ptr<LC_PropertiesProviderTolerance> m_tolerance{nullptr};
    std::unique_ptr<LC_PropertiesProviderParabola> m_parabola{nullptr};
    std::unique_ptr<LC_PropertiesProviderDocument> m_document{nullptr};

    RS2::EntityType m_entityType = RS2::EntityUnknown;
    QList<RS_Entity*> m_entitiesList;
    bool m_lastContainerForNoSelection = false;
};

#endif
