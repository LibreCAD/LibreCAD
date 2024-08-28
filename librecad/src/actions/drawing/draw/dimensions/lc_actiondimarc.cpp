/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo <carlo.melwyn@outlook.com>
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


#include <iostream>

#include <QMouseEvent>

#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_dialogfactory.h"
#include "rs_coordinateevent.h"

#include "lc_actiondimarc.h"

LC_ActionDimArc::LC_ActionDimArc(RS_EntityContainer &container, RS_GraphicView &graphicView):
    RS_ActionDimension("Draw Arc Dimensions", container, graphicView){
    reset();
}

LC_ActionDimArc::~LC_ActionDimArc() = default;

void LC_ActionDimArc::reset(){
    RS_ActionDimension::reset();

    actionType = RS2::ActionDimArc;

    dimArcData.radius = 0.0;
    dimArcData.arcLength = 0.0;

    dimArcData.centre = RS_Vector(false);
    dimArcData.endAngle = RS_Vector(false);
    dimArcData.startAngle = RS_Vector(false);

    selectedArcEntity = nullptr;

    updateOptions(); // fixme - check whether it's necessary there
}

void LC_ActionDimArc::trigger(){
    RS_PreviewActionInterface::trigger();

    if (selectedArcEntity == nullptr){
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDimArc::trigger: selectedArcEntity is nullptr.\n");
        return;
    }

    if (!dimArcData.centre.valid){
        RS_DEBUG->print(RS_Debug::D_ERROR, "LC_ActionDimArc::trigger: dimArcData.centre is not valid.\n");
        return;
    }

    auto newEntity= new LC_DimArc(container, *data, dimArcData);

    newEntity->setLayerToActive();
    newEntity->setPenToActive();
    newEntity->update();
    container->addEntity(newEntity);

    addToDocumentUndoable(newEntity);

    setStatus(SetEntity);
    graphicView->redraw(RS2::RedrawDrawing);

    RS_Snapper::finish();
}

void LC_ActionDimArc::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("LC_ActionDimArc::mouseMoveEvent begin");
    RS_Vector snap = snapPoint(e);
    deleteHighlights();
    switch (getStatus()) {
        case SetEntity:{
            auto en = catchEntity(e, RS2::EntityArc, RS2::ResolveAll);
            if (en != nullptr){
                highlightHover(en);
            }
            break;
        }
        case SetPos: {
            snap = getFreeSnapAwarePoint(e, snap);
            highlightSelected(selectedArcEntity);
            setRadius(snap);

            // fixme - determine why DimArc is drawn on preview by preview pen, while other dimension entities - using normal pen...

            LC_DimArc *temp_dimArc_entity{new LC_DimArc(preview.get(), *data, dimArcData)};

            deletePreview();
            previewEntity(temp_dimArc_entity);
            drawPreview();
            break;
        }
        default:
            break;
    }
    drawHighlights();
    RS_DEBUG->print("LC_ActionDimArc::mouseMoveEvent end");
}

void LC_ActionDimArc::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetEntity: {
            selectedArcEntity = catchEntity(e, RS2::ResolveAll);

            if (selectedArcEntity != nullptr){
                if (selectedArcEntity->is(RS2::EntityArc)){
                    dimArcData.centre = selectedArcEntity->getCenter();
                    dimArcData.arcLength = selectedArcEntity->getLength();

                    dimArcData.startAngle = RS_Vector(((RS_Arc *) selectedArcEntity)->getAngle1());
                    dimArcData.endAngle = RS_Vector(((RS_Arc *) selectedArcEntity)->getAngle2());

                    data->definitionPoint = selectedArcEntity->getStartpoint();

                    if (((RS_Arc *) selectedArcEntity)->isReversed()){
                        const RS_Vector tempAngle = RS_Vector(dimArcData.startAngle);

                        dimArcData.startAngle = dimArcData.endAngle;
                        dimArcData.endAngle = tempAngle;

                        data->definitionPoint = selectedArcEntity->getEndpoint();
                    }

                    setStatus(SetPos);
                } else {
                    RS_DEBUG->print(RS_Debug::D_ERROR,
                                    "LC_ActionDimArc::mouseReleaseEvent: selectedArcEntity is not an arc.");

                    selectedArcEntity = nullptr;
                }
            }
            break;
        }
        case SetPos: {
            RS_Vector snap = snapPoint(e);
            snap = getFreeSnapAwarePoint(e, snap);
            fireCoordinateEvent(snap);
            break;
        }
        default:
            break;
    }

}

void LC_ActionDimArc::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionDimArc::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &pos) {
    switch (status) {
        case SetPos: {
            setRadius(pos);
            trigger();
            reset();
            setStatus(SetEntity);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDimArc::doProcessCommand([[maybe_unused]]int status, const QString& c){
    // fixme - support other commands
    bool accept = false;
    if (checkCommand("exit", c)){
        init(-1);
        accept = true;
    }
    return accept;
}

QStringList LC_ActionDimArc::getAvailableCommands(){
    QStringList availableCommandsList{"help", "exit"};
    return availableCommandsList;
}

void LC_ActionDimArc::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select arc entity"));
            break;
        case SetPos:
            updateMouseWidgetTRBack(tr("Specify dimension arc location"),MOD_SHIFT_FREE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void LC_ActionDimArc::setRadius(const RS_Vector &selectedPosition){
    const double minimum_dimArc_gap = 0.0;
    dimArcData.radius = selectedPosition.distanceTo(dimArcData.centre);
    const double minimumRadius = selectedArcEntity->getRadius() + minimum_dimArc_gap;
    if (dimArcData.radius < minimumRadius) {
        dimArcData.radius = minimumRadius;
    }
}
