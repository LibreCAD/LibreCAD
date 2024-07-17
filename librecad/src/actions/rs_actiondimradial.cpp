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

#include <QMouseEvent>

#include "rs_actiondimradial.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dimradial.h"
#include "rs_graphicview.h"
#include "rs_math.h"
#include "rs_preview.h"

// fixme - options for selection definition point,
// fixme - adding dimensions to already selected items

RS_ActionDimRadial::RS_ActionDimRadial(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionDimension("Draw Radial Dimensions",
					container, graphicView)
        , entity(nullptr)
        , pos(std::make_unique<RS_Vector>())
        , edata{ std::make_unique<RS_DimRadialData>()}
        , lastStatus(SetEntity)
{
	actionType=RS2::ActionDimRadial;
    reset();
}

RS_ActionDimRadial::~RS_ActionDimRadial() = default;

void RS_ActionDimRadial::reset(){
    RS_ActionDimension::reset();

    *edata = {};
    entity = nullptr;
    *pos = {};
    lastStatus = SetEntity;
    updateOptions();
}

void RS_ActionDimRadial::trigger(){
    RS_ActionDimension::trigger();

    preparePreview();
    if (entity!= nullptr){
        auto *newEntity = createDim(container);
        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        newEntity->update();
        container->addEntity(newEntity);

        addToDocumentUndoable(newEntity);

        graphicView->redraw(RS2::RedrawDrawing);
        RS_Snapper::finish();

    } else {
        RS_DEBUG->print("RS_ActionDimRadial::trigger: Entity is nullptr\n");
    }
}

RS_DimRadial *RS_ActionDimRadial::createDim(RS_EntityContainer *parent) const{
    auto *newEntity = new RS_DimRadial(parent, *data, *edata);
    return newEntity;
}

void RS_ActionDimRadial::preparePreview(){
    if (entity != nullptr){
        double angle = data->definitionPoint.angleTo(*pos);
        double radius = entity->getRadius();
        edata->definitionPoint.setPolar(radius, angle);
        edata->definitionPoint += data->definitionPoint;
    }
}

void RS_ActionDimRadial::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDimRadial::mouseMoveEvent begin");
    RS_Vector snap = snapPoint(e);
    deleteHighlights();
    switch (getStatus()) {
        case SetEntity: {
            RS_Entity *en = catchEntity(e, RS2::ResolveAll);
            if (en != nullptr){
                if (isArc(en) || isCircle(en)){
                    highlightHover(en);
                }
            }
            break;
        }
        case SetPos: {
            if (entity != nullptr){
                highlightSelected(entity);
                deletePreview();

                *pos = getSnapAngleAwarePoint(e,entity->getCenter(), snap, true);
                preparePreview();

                auto* d = createDim(preview.get());
                previewEntity(d);
                d->update();

                previewRefSelectablePoint(getDefinitionPoint());

                drawPreview();
            }
            break;
        }
        default:
            break;
    }
    drawHighlights();
    RS_DEBUG->print("RS_ActionDimRadial::mouseMoveEvent end");
}

const RS_Vector &RS_ActionDimRadial::getDefinitionPoint() const{return edata->definitionPoint;}

void RS_ActionDimRadial::mouseLeftButtonReleaseEvent(int status, QMouseEvent *e) {
    switch (status) {
        case SetEntity: {
            RS_Entity *en = catchEntity(e, RS2::ResolveAll);
            if (en != nullptr){
                RS2::EntityType rtti = en->rtti();
                if (rtti == RS2::EntityArc || rtti == RS2::EntityCircle){
                    entity = en;
                    const RS_Vector &center = en->getCenter();
                    data->definitionPoint = center;
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

void RS_ActionDimRadial::mouseRightButtonReleaseEvent(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    init(status - 1);
}

void RS_ActionDimRadial::coordinateEvent(RS_CoordinateEvent *e){
    if (e == nullptr) return;

    switch (getStatus()) {
        case SetPos: {
            *pos = e->getCoordinate();
            trigger();
            reset();
            setStatus(SetEntity);
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDimRadial::doProcessCommand(int status, const QString &c) {
    bool accept = true;
    // setting new text label:

    // fixme - check logic, restructure if possible
    if (status == SetText){
        setText(c);
        updateOptions();
        graphicView->enableCoordinateInput();
        setStatus(lastStatus);
        accept = true;
    }
    else if (checkCommand("text", c)) { // command: text
        lastStatus = (Status) status;
        graphicView->disableCoordinateInput();
        setStatus(SetText);
        accept = true;
    }
    else if (status == SetPos) { // setting angle
        bool ok;
        double a = RS_Math::eval(c, &ok);
        if (ok) {
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

QStringList RS_ActionDimRadial::getAvailableCommands(){
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

void RS_ActionDimRadial::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel("Select arc or circle entity");
            break;
        case SetPos:
            updateMouseWidgetTRCancel("Specify dimension line position or enter angle:", MOD_SHIFT_ANGLE_SNAP);
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
