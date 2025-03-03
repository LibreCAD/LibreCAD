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

#include "rs_actiondrawlineparallelthrough.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "qg_lineparallelthroughoptions.h"
#include "rs_actioninterface.h"

RS_ActionDrawLineParallelThrough::RS_ActionDrawLineParallelThrough(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
		:RS_PreviewActionInterface("Draw Parallels", container, graphicView)
		,coord(new RS_Vector{})
		,lastStatus(SetEntity){
    actionType=RS2::ActionDrawLineParallelThrough;
    m_SnapDistance=1.;
}

RS_ActionDrawLineParallelThrough::~RS_ActionDrawLineParallelThrough() = default;

void RS_ActionDrawLineParallelThrough::finish(bool updateTB){
    if(entity){
        entity->setHighlighted(false);
        graphicView->drawEntity(entity);
        entity=nullptr;
    }
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionDrawLineParallelThrough::trigger(){
    RS_PreviewActionInterface::trigger();

    if (entity){
        RS_Creation creation(container, graphicView);
        RS_Entity *e = creation.createParallelThrough(*coord,number,entity, symmetric);

        if (!e){
            RS_DEBUG->print("RS_ActionDrawLineParallelThrough::trigger: No parallels added\n");
        }
    }
}

void RS_ActionDrawLineParallelThrough::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDrawLineParallelThrough::mouseMoveEvent begin");

    deleteHighlights();
    switch (getStatus()) {
        case SetEntity: {
            entity = catchEntity(e, RS2::ResolveAll);
            if (entity != nullptr){
                highlightHover(entity);
                if (showRefEntitiesOnPreview) {
                    RS_Vector nearest = entity->getNearestPointOnEntity(*coord, false);
                    previewRefPoint(nearest);
                }
            }
            break;
        }
        case SetPos: {
            *coord = getFreeSnapAwarePoint(e, snapPoint(e));

            deletePreview();
            highlightSelected(entity);
            RS_Creation creation(preview.get(), nullptr, false);
            auto en = creation.createParallelThrough(*coord, number, entity, symmetric);
            if (en != nullptr){
                RS_Vector nearest = entity->getNearestPointOnEntity(*coord, false);
                moveRelativeZero(nearest); // fixme - should we restore original relzero?
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(nearest);
                    previewRefLine(nearest, *coord);

                    if (symmetric && isLine(entity)){
                        RS_Vector otherPoint = coord->mirror(entity->getStartpoint(), entity->getEndpoint());
                        previewRefPoint(otherPoint);
                        previewRefLine(nearest, otherPoint);
                    }
                }
            }

            drawPreview();
            break;
        }
        default:
            break;
    }
    drawHighlights();

    RS_DEBUG->print("RS_ActionDrawLineParallelThrough::mouseMoveEvent end");
}

void RS_ActionDrawLineParallelThrough::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetEntity:
            entity = catchEntity(e, RS2::ResolveAll);
            if (entity){
                setStatus(SetPos);
            }
            break;
        case SetPos: {
            fireCoordinateEventForSnap(e);
            break;
        }
        default:
            break;
    }

}

void RS_ActionDrawLineParallelThrough::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    if (entity){
        entity = nullptr;
    }
    initPrevious(status);
}

void RS_ActionDrawLineParallelThrough::onCoordinateEvent(int status,[[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetPos: {
            *coord = mouse;
            trigger();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDrawLineParallelThrough::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select entity"));
            break;
        case SetPos:
            updateMouseWidgetTRBack(tr("Specify through point"), MOD_SHIFT_FREE_SNAP);
            break;
        case SetNumber:
            updateMouseWidgetTRBack(tr("Number:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

bool RS_ActionDrawLineParallelThrough::doProcessCommand(int status, const QString &c) {
    bool accept = false;

    switch (status) {
        case SetEntity:
        case SetPos: {
            if (checkCommand("number", c)){
                deletePreview();
                lastStatus = (Status) getStatus();
                setStatus(SetNumber);
                accept = true;
            }
            break;
        }
        case SetNumber: {
            bool ok;
            int n = c.toInt(&ok);
            if (ok){
                accept = true;
                if (n > 0 && n < 100){
                    number = n;
                } else {
                    commandMessage(tr("Not a valid number. Try 1..99"));
                }
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList RS_ActionDrawLineParallelThrough::getAvailableCommands(){
    QStringList cmd;

    switch (getStatus()) {
        case SetEntity:
            cmd += command("number");
            break;
        default:
            break;
    }
    return cmd;
}
RS2::CursorType RS_ActionDrawLineParallelThrough::doGetMouseCursor([[maybe_unused]] int status){
    switch (status) {
        case SetEntity:
            return RS2::SelectCursor;
        case SetNumber:
        case SetPos:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}

int RS_ActionDrawLineParallelThrough::getNumber() const{
    return number;
}

void RS_ActionDrawLineParallelThrough::setNumber(int n) {
    number = n;
}

LC_ActionOptionsWidget* RS_ActionDrawLineParallelThrough::createOptionsWidget(){
    return new QG_LineParallelThroughOptions();
}
