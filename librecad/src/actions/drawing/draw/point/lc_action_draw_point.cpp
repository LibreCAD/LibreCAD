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

#include "lc_action_draw_point.h"

#include "rs_document.h"
#include "rs_point.h"

RS_ActionDrawPoint::RS_ActionDrawPoint(LC_ActionContext *actionContext)
    :LC_SingleEntityCreationAction("ActionDrawPoint",actionContext, RS2::ActionDrawPoint), m_pointPosition(new RS_Vector{}){
}

RS_ActionDrawPoint::~RS_ActionDrawPoint() = default;


RS_Entity* RS_ActionDrawPoint::doTriggerCreateEntity() {
    if (m_pointPosition->valid) {
        auto *point = new RS_Point(m_document, RS_PointData(*m_pointPosition));
        return point;
    }
    return nullptr;
}

bool RS_ActionDrawPoint::isInVisualSnapStatus([[maybe_unused]]int status) {
    return true;
}

void RS_ActionDrawPoint::doTriggerCompletion([[maybe_unused]]bool success) {
    moveRelativeZero(*m_pointPosition);
}

RS_Vector RS_ActionDrawPoint::getFreeSnapAwarePointAlt(const LC_MouseEvent *e, const RS_Vector &pos) const{
    const RS_Vector mouse = e->isControl ?  e->graphPoint : pos;
    return mouse;
}

void RS_ActionDrawPoint::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    if (!trySnapToRelZeroCoordinateEvent(e)){
        RS_Vector pos = e->snapPoint;
        pos = getFreeSnapAwarePointAlt(e, pos);
        previewToCreatePoint(pos); // is it really necessary??
        previewRefSelectablePoint(pos);
    }
}

void RS_ActionDrawPoint::onMouseLeftButtonRelease([[maybe_unused]]int status, const LC_MouseEvent* e) {
    RS_Vector snap = e->snapPoint;
    snap = getFreeSnapAwarePointAlt(e, snap);
    fireCoordinateEvent(snap);
}

void RS_ActionDrawPoint::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    initPrevious(status);
}

void RS_ActionDrawPoint::onCoordinateEvent( [[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &coord) {
    *m_pointPosition = coord;
    addSnappedPointToVisualSnap(coord);
    trigger();
}

void RS_ActionDrawPoint::updateActionPrompt(){
    switch (getStatus()) {
        case 0: {
            updatePromptTRCancel(tr("Specify location"), MOD_SHIFT_AND_CTRL(MSG_REL_ZERO,MSG_FREE_SNAP));
            break;
        }
        default:
            updatePrompt();
            break;
    }
}
RS2::CursorType RS_ActionDrawPoint::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
