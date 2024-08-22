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

#include "rs_actionselectcontour.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_selection.h"

RS_ActionSelectContour::RS_ActionSelectContour(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
		:RS_PreviewActionInterface("Select Contours", container, graphicView)
		,en(nullptr)
{
	actionType=RS2::ActionSelectContour;
}

void RS_ActionSelectContour::mouseMoveEvent(QMouseEvent *event){
    snapPoint(event);
    deleteHighlights();
    auto ent = catchEntity(event);
    if (ent != nullptr){
        // fixme - proper highlighting of planned selection - yet after fixing underlying logic!
//        RS_Selection s(*container, graphicView);
//        s.selectContour(en);
        // fixme - temporarily highlight only caught entity only
        highlightHover(ent);
    }
    drawHighlights();
}

void RS_ActionSelectContour::trigger(){
    if (en){
        if (en->isAtomic()){ // fixme - why it is so??? why it's not suitable to select, say, polyline here too?
            RS_Selection s(*container, graphicView);
            s.selectContour(en);
            updateSelectionWidget();
        } else
           commandMessage(tr("Entity must be an Atomic Entity."));
    } else
        RS_DEBUG->print("RS_ActionSelectContour::trigger: Entity is NULL\n");
}

void RS_ActionSelectContour::onMouseLeftButtonRelease([[maybe_unused]] int status, QMouseEvent *e) {
    en = catchEntity(e);
    trigger();
}

void RS_ActionSelectContour::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    initPrevious(status);
}
RS2::CursorType RS_ActionSelectContour::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
