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

#include "rs_actionselectintersected.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_selection.h"

struct RS_ActionSelectIntersected::Points {
	RS_Vector v1;
	RS_Vector v2;
};

/**
 * Constructor.
 *
 * @param select true: select window. false: deselect window
 */
RS_ActionSelectIntersected::RS_ActionSelectIntersected(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView,
    bool select)
    :RS_PreviewActionInterface("Select Intersected",
                               container, graphicView), pPoints(std::make_unique<Points>()), select(select){
	actionType=RS2::ActionSelectIntersected;
}

RS_ActionSelectIntersected::~RS_ActionSelectIntersected() = default;


void RS_ActionSelectIntersected::init(int status) {
    RS_PreviewActionInterface::init(status);
    pPoints = std::make_unique<Points>();
    snapMode.clear();
    snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionSelectIntersected::doTrigger() {
    if (pPoints->v1.valid && pPoints->v2.valid){
        if (toGuiDX(pPoints->v1.distanceTo(pPoints->v2)) > 10){
            RS_Selection s(*container, viewport);
            s.selectIntersected(pPoints->v1, pPoints->v2, select);
            init(SetPoint1);
        }
    }
}

void RS_ActionSelectIntersected::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    if (status == SetPoint2 && pPoints->v1.valid){
        pPoints->v2 = snap;
        previewLine(pPoints->v1, pPoints->v2);
        // todo - of course, ideally it will be also to highlight entities that will be selected...
        // however, calculating of intersections as it is currently is may be quite costly operation for mouse move
        // todo - review preview for selected entities after indexing

    }
}

void RS_ActionSelectIntersected::onMouseLeftButtonPress(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetPoint1:
            pPoints->v1 = e->snapPoint;
            setStatus(SetPoint2);
            break;

        default:
            break;
    }
}

void RS_ActionSelectIntersected::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionSelectIntersected::mouseReleaseEvent()");
    if (status == SetPoint2){
        pPoints->v2 = e->snapPoint;
        trigger();
    }
}

void RS_ActionSelectIntersected::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionSelectIntersected::mouseReleaseEvent()");
    if (getStatus() == SetPoint2){
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionSelectIntersected::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetPoint1:
            updateMouseWidgetTRCancel(tr("Choose first point of intersection line"));
            break;
        case SetPoint2:
            updateMouseWidgetTRBack(tr("Choose second point of intersection line"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionSelectIntersected::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
