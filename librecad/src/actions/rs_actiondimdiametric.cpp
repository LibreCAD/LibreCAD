/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#include <QAction>
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

// fixme - options for selection definition point,
// fixme - adding dimensions to selected items
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
    RS_DIALOGFACTORY->requestOptions(this, true, true);
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
        RS_DEBUG->print("RS_ActionDimDiametric::trigger:"
                        " Entity is nullptr\n");
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

void RS_ActionDimDiametric::mouseLeftButtonReleaseEvent(int status, QMouseEvent *e) {
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
                    commandMessageTR("Not a circle or arc entity");
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

void RS_ActionDimDiametric::mouseRightButtonReleaseEvent(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    init(status - 1);
}

void RS_ActionDimDiametric::coordinateEvent(RS_CoordinateEvent *e){
    if (e == nullptr) return;

    switch (getStatus()) {
        case SetPos:
            *pos = e->getCoordinate();
            trigger();
            reset();
            setStatus(SetEntity);
            break;

        default:
            break;
    }
}

bool RS_ActionDimDiametric::doProcessCommand(int status, const QString &c) {
    // fixme - check whether the code is duplicated with other dim actions
    bool accept = false;
    // setting new text label:
    if (getStatus() == SetText){
        setText(c);
        updateOptions();
        graphicView->enableCoordinateInput();
        setStatus(lastStatus);
        accept = true;
    }
    else if (checkCommand("text", c)){ // command: text
        lastStatus = (Status) getStatus();
        graphicView->disableCoordinateInput();
        setStatus(SetText);
        accept = true;
    }
    else if (getStatus() == SetPos){// setting angle
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
            commandMessageTR("Not a valid expression");
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
            updateMouseWidgetTRCancel("Select arc or circle entity");
            break;
        case SetPos:
            updateMouseWidgetTRCancel("Specify dimension line location", MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetText:
            updateMouseWidgetTR("Enter dimension text:", "");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

// EOF
