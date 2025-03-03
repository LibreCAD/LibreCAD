/****************************************************************************
*
* Actions that is used for selecting entity for which properties should be
* shown in QuickInfo widget
*
Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include <QMouseEvent>
#include "rs.h"
#include "lc_actioninfoproperties.h"
#include "lc_quickinfowidget.h"
#include "qc_applicationwindow.h"

namespace {

// Glowing effects on Mouse hover
    constexpr double minimumHoverTolerance = 3.0;
    constexpr double hoverToleranceFactor1 = 1.0;
    constexpr double hoverToleranceFactor2 = 10.0;

    // whether the entity supports glowing effects on mouse hovering
    bool allowMouseOverGlowing(const RS_Entity* entity)
    {
        if (entity == nullptr)
            return false;
        switch (entity->rtti())
        {
            case RS2::EntityHatch:
            case RS2::EntityImage:
            case RS2::EntitySolid:
            case RS2::EntityUnknown:
            case RS2::EntityPattern:
                return false;
            default:
                return true;
        }
    }
}

// todo - consider later relying on default action (probably with some specific flag) instead of this function...

LC_ActionInfoProperties::LC_ActionInfoProperties(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Entity Info", container, graphicView, RS2::ActionInfoProperties){}

void LC_ActionInfoProperties::init(int status){
    RS_PreviewActionInterface::init(status);
    if (status >=0){
        // update widget
        LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
        if (entityInfoWidget != nullptr){
            entityInfoWidget->setWidgetMode(LC_QuickInfoWidget::MODE_ENTITY_INFO);
        }
    }
}

void LC_ActionInfoProperties::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    // notify widget
    highlightAndShowEntityInfo(e);
    setStatus(-1);
    finish();
}

void LC_ActionInfoProperties::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
    setStatus(-1);
    finish();
}

void LC_ActionInfoProperties::mouseMoveEvent(QMouseEvent* e) {
    highlightAndShowEntityInfo(e);
}

void LC_ActionInfoProperties::highlightAndShowEntityInfo(QMouseEvent *e){
    RS_Vector mouse = toGraph(e);
    updateCoordinateWidgetByRelZero(mouse);

    // clear any existing hovering
    clearHighLighting();
    deleteSnapper();
    highlightHoveredEntity(e);
}


void LC_ActionInfoProperties::highlightHoveredEntity(QMouseEvent* event)
{
    bool shouldShowQuickInfoWidget = true; // todo - read from options as there will be support

    RS_Entity* entity = catchEntity(event);
    if (entity == nullptr)
        return;
    if (!entity->isVisible() || (entity->isLocked() && !shouldShowQuickInfoWidget)){
        return;
    }

    const double hoverToleranceFactor = entity->is(RS2::EntityEllipse)
                                        ? hoverToleranceFactor1
                                        : hoverToleranceFactor2;

    const double hoverTolerance { hoverToleranceFactor / graphicView->getFactor().magnitude() };

    double hoverTolerance_adjusted = ((entity->rtti() != RS2::EntityEllipse) && (hoverTolerance < minimumHoverTolerance))
                                     ? minimumHoverTolerance
                                     : hoverTolerance;

    double screenTolerance = graphicView->toGraphDX( (int)(0.01*std::min(graphicView->getWidth(), graphicView->getHeight())));
    hoverTolerance_adjusted = std::min(hoverTolerance_adjusted, screenTolerance);
    bool isPointOnEntity = false;

    RS_Vector currentMousePosition = toGraph(event);
    if (((entity->rtti() >= RS2::EntityDimAligned) && (entity->rtti() <= RS2::EntityDimLeader))
        ||   (entity->rtti() == RS2::EntityText)       || (entity->rtti() == RS2::EntityMText))
    {
        double nearestDistanceTo_pointOnEntity = 0.;

        entity->getNearestPointOnEntity(currentMousePosition, true, &nearestDistanceTo_pointOnEntity);

        if (nearestDistanceTo_pointOnEntity <= hoverTolerance_adjusted) isPointOnEntity = true;
    }
    else
    {
        isPointOnEntity = entity->isPointOnEntity(currentMousePosition, hoverTolerance_adjusted);
    }

    // Glowing effect on mouse hovering
    if (isPointOnEntity){
        highlightEntity(entity);
        if (shouldShowQuickInfoWidget){
            updateQuickInfoWidget(entity);
        }
    }
}

void LC_ActionInfoProperties::clearHighLighting()
{
    auto* hContainer = graphicView->getOverlayContainer(RS2::OverlayEffects);
    if (hContainer->count()==0)
        return;
    hContainer->clear();
    highlightedEntity=nullptr;
    graphicView->redraw(RS2::RedrawOverlay);
    clearQuickInfoWidget();
}

void LC_ActionInfoProperties::highlightEntity(RS_Entity* entity) {
    if (!allowMouseOverGlowing(entity))
        return;

    // The container for highlighting effects
    auto hContainer = graphicView->getOverlayContainer(RS2::OverlayEffects);
    hContainer->clear();

    highlightedEntity = entity;

    RS_Entity* duplicatedEntity = highlightedEntity->clone();

    duplicatedEntity->reparent(hContainer);
    duplicatedEntity->setHighlighted(true);
    hContainer->addEntity(duplicatedEntity);

    graphicView->redraw(RS2::RedrawOverlay);
}

void LC_ActionInfoProperties::updateQuickInfoWidget(RS_Entity *pEntity){
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        entityInfoWidget->processEntity(pEntity);
    }
}

void LC_ActionInfoProperties::clearQuickInfoWidget(){
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        entityInfoWidget->processEntity(nullptr);
    }
}

void LC_ActionInfoProperties::updateMouseButtonHints(){
    updateMouseWidgetTRCancel(tr("Select entity"));
}
