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

#include "rs_actioninfodist2.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_units.h"

RS_ActionInfoDist2::RS_ActionInfoDist2(RS_EntityContainer &container, RS_GraphicView &graphicView, bool fromPoint)
    :RS_PreviewActionInterface("Info Dist2", container, graphicView), entity(nullptr){
    actionType = RS2::ActionInfoDist2;
    fromPointToEntity = fromPoint;
}

RS_ActionInfoDist2::~RS_ActionInfoDist2(){}

void RS_ActionInfoDist2::init(int status){
    RS_PreviewActionInterface::init(status);
    if (status == 0 && fromPointToEntity){
        setStatus(SetPoint);
    }    
}

// fixme - consider displaying information in EntityInfo widget
void RS_ActionInfoDist2::trigger(){

    RS_DEBUG->print("RS_ActionInfoDist2::trigger()");

    if (point.valid && entity){
        double dist = entity->getDistanceToPoint(point);
        QString str = RS_Units::formatLinear(dist, graphic->getUnit(),
                                             graphic->getLinearFormat(), graphic->getLinearPrecision());
        RS_DIALOGFACTORY->commandMessage(tr("Distance: %1").arg(str));
        graphicView->redraw(RS2::RedrawDrawing);
    }
}

void RS_ActionInfoDist2::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionInfoDist2::mouseMoveEvent begin");
    RS_Vector snap = snapPoint(e);
    deleteHighlights();
    deletePreview();
    switch (getStatus()) {
        case SetEntity: {
            auto en = catchEntity(e);
            if (en != nullptr){
                addToHighlights(en);

                deleteSnapper();
                if (point.valid){
                    if (fromPointToEntity){
                        RS_Vector nearest = en->getNearestPointOnEntity(point, true);
                        addReferenceLineToPreview(nearest, point);
                        addReferencePointToPreview(point);
                        addReferencePointToPreview(nearest);
                    } else {
                        // no more
                    }
                }
            }
            else{
                if (fromPointToEntity){
                    addReferenceLineToPreview(snap, point);
                }
            }
            break;
        }
        case SetPoint: {
            if (fromPointToEntity){
                trySnapToRelZeroCoordinateEvent(e);
                addReferencePointToPreview(snap);

            } else if (entity){
                addToHighlights(entity);
                if (!trySnapToRelZeroCoordinateEvent(e)){
                    RS_Vector nearest = entity->getNearestPointOnEntity(snap, true);
                    addReferenceLineToPreview(nearest, snap);
                    addReferencePointToPreview(nearest);
                    addReferencePointToPreview(snap);
                }
            }
            break;
        }
        default:
            break;
    }
    drawHighlights();
    drawPreview();

    RS_DEBUG->print("RS_ActionInfoDist2::mouseMoveEvent end");
}

void RS_ActionInfoDist2::mouseReleaseEvent(QMouseEvent *e){
    int status = getStatus();
    if (e->button() == Qt::LeftButton){
        switch (status) {
            case SetEntity: {
                entity = catchEntity(e);
                if (entity != nullptr){
                    if (fromPointToEntity){
                        trigger();
                        setStatus(SetPoint);
                    } else {
                        setStatus(SetPoint);
                    }
                }
                break;
            }
            case SetPoint: {
                const RS_Vector &snap = snapPoint(e);
                if (fromPointToEntity){
                    point = snap;
                    graphicView->moveRelativeZero(point);
                    setStatus(SetEntity);
                }
                else {
                    RS_CoordinateEvent ce(snap);
                    coordinateEvent(&ce);
                }
                break;
            }
            default:
                break;
        }
    } else if (e->button() == Qt::RightButton){
        deletePreview();
        int newStatus = -1;
        switch (status) {
            case SetEntity:
                newStatus = fromPointToEntity ? SetPoint : -1;
                break;
            case SetPoint:
                newStatus = fromPointToEntity ? -1 : SetEntity;
                break;
            default:
                break;
        }
        setStatus(newStatus);
    }
}

void RS_ActionInfoDist2::coordinateEvent(RS_CoordinateEvent *e){
    if (e == nullptr){
        return;
    }

    int status = getStatus();
    if (status == SetPoint){
        point = e->getCoordinate();
        graphicView->moveRelativeZero(point);
        if (fromPointToEntity){
            setStatus(SetEntity);
        }
        else {
            trigger();
            setStatus(SetEntity);
        }
    }
}

void RS_ActionInfoDist2::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify entity"), tr("Cancel"));
            break;
        case SetPoint:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify point"), tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void RS_ActionInfoDist2::updateMouseCursor(){
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
