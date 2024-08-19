/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/


#include <QMouseEvent>

#include "rs_actiondimdiametric.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dimdiametric.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_preview.h"

// fixme - sand - options for selection definition point angle (for uniform settigns of dimensions), possibility to define label inside/outside,
// todo - think whether it's practical adding multiple dimensions to selected circles?
RS_ActionDimDiametric::RS_ActionDimDiametric(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionDimension("Draw Diametric Dimensions",
                    container, graphicView)
        , pos{std::make_unique<RS_Vector>()}
        , edata{std::make_unique<RS_DimDiametricData>()}
{
	actionType=RS2::ActionDimDiametric;
    reset();
}

RS_ActionDimDiametric::~RS_ActionDimDiametric() = default;

void RS_ActionDimDiametric::reset(){
    RS_ActionDimension::reset();

    *edata = {{}, 0.0};
    entity = nullptr;
    *pos = {};
    updateOptions();
}

void RS_ActionDimDiametric::trigger(){
    RS_ActionDimension::trigger();

    preparePreview();
    if (entity != nullptr){
        auto *newEntity = createDim(container);
        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        newEntity->update();
        container->addEntity(newEntity);

        addToDocumentUndoable(newEntity);

        graphicView->redraw(RS2::RedrawDrawing);
        RS_Snapper::finish();

    } else {
        RS_DEBUG->print("RS_ActionDimDiametric::trigger: Entity is nullptr\n");
    }
}

RS_DimDiametric *RS_ActionDimDiametric::createDim(RS_EntityContainer *parent) const{
    RS_DimDiametric *newEntity;
    newEntity = new RS_DimDiametric(parent,
                                *data,
                                *edata);
    return newEntity;
}

void RS_ActionDimDiametric::preparePreview(){
    if (entity){
        double radius = entity->getRadius();
        RS_Vector center = entity->getCenter();

        double angle = center.angleTo(*pos);

        data->definitionPoint.setPolar(radius, angle + M_PI);
        data->definitionPoint += center;

        edata->definitionPoint.setPolar(radius, angle);
        edata->definitionPoint += center;
    }
}

void RS_ActionDimDiametric::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDimDiametric::mouseMoveEvent begin");
    RS_Vector snap = snapPoint(e);
    deleteHighlights();
    switch (getStatus()) {
        case SetEntity: {
            RS_Entity *en = catchEntity(e, RS2::ResolveAll);
            if (isArc(en) || isCircle(en)){
                highlightHover(en);
            }
            break;
        }
        case SetPos:{
            if (entity != nullptr){
                highlightSelected(entity);
                deletePreview();
                *pos = getSnapAngleAwarePoint(e, entity->getCenter(), snap, true);
                preparePreview();

                auto *d = createDim(preview.get());
                previewEntity(d);
                d->update();

                previewRefSelectablePoint(d->getDefinitionPoint());

                drawPreview();
            }
            break;
    }
        default:
            break;
    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionDimDiametric::mouseMoveEvent end");
}

void RS_ActionDimDiametric::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetEntity: {
            RS_Entity *en = catchEntity(e, RS2::ResolveAll);
            if (en!= nullptr){
                if (isArc(en) || isCircle(en)){
                    entity = en;
                    const RS_Vector &center = en->getCenter();
                    moveRelativeZero(center);
                    setStatus(SetPos);
                } else {
                    commandMessage(tr("Not a circle or arc entity"));
                }
            }
            break;
        }
        case SetPos: {
            RS_Vector snap = snapPoint(e);
            snap = getSnapAngleAwarePoint(e, entity->getCenter(), snap);
            fireCoordinateEvent(snap);
            break;
        }
        default:
            break;
    }
}

void RS_ActionDimDiametric::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionDimDiametric::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &coord) {
    switch (status) {
        case SetPos: {
            *pos = coord;
            trigger();
            reset();
            setStatus(SetEntity);
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDimDiametric::doProcessCommand(int status, const QString &c) {
    // fixme - check whether the code is duplicated with other dim actions
    bool accept = false;
    // setting new text label:
    if (status == SetText){
        setText(c);
        updateOptions();
        graphicView->enableCoordinateInput();
        setStatus(lastStatus);
        accept = true;
    }
    else if (checkCommand("text", c)){ // command: text
        lastStatus = (Status) status;
        graphicView->disableCoordinateInput();
        setStatus(SetText);
        accept = true;
    }
    else if (status == SetPos){// setting angle
        bool ok;
        double a = RS_Math::eval(c, &ok);
        if (ok){
            accept = true;
            pos->setPolar(1.0, RS_Math::deg2rad(a));
            *pos += data->definitionPoint;
            trigger();
            reset();
            setStatus(SetEntity);
        } else {
            commandMessage(tr("Not a valid expression"));
        }
    }
    return accept;
}

QStringList RS_ActionDimDiametric::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetEntity:
        case SetPos:
            cmd += command("text");
            break;

        default:
            break;
    }

    return cmd;
}

void RS_ActionDimDiametric::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select arc or circle entity"));
            break;
        case SetPos:
            updateMouseWidgetTRCancel(tr("Specify dimension line location"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetText:
            updateMouseWidget(tr("Enter dimension text:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
