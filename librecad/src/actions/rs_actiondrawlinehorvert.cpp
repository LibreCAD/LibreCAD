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
#include "rs_actiondrawlinehorvert.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDrawLineHorVert::Points {
	/**
	 * Line data.
	 */
	RS_LineData data;
	/**
	 * 2 points
	 */
	RS_Vector p1;
	RS_Vector p2;
};



RS_ActionDrawLineHorVert::RS_ActionDrawLineHorVert(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw horizontal/vertical lines",
						   container, graphicView)
		, pPoints(new Points{})
{
    reset();
    RS_DEBUG->print("RS_ActionDrawLineHorVert::constructor");
}



RS_ActionDrawLineHorVert::~RS_ActionDrawLineHorVert() = default;

void RS_ActionDrawLineHorVert::reset() {
	pPoints->data = {{}, {}};
}



void RS_ActionDrawLineHorVert::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
    RS_DEBUG->print("RS_ActionDrawLineHorVert::init");
}



void RS_ActionDrawLineHorVert::trigger() {
    RS_PreviewActionInterface::trigger();

	RS_Line* line = new RS_Line(container, pPoints->data);
    line->setLayerToActive();
    line->setPenToActive();
    container->addEntity(line);

    // upd. undo list:
    if (document) {
        document->startUndoCycle();
        document->addUndoable(line);
        document->endUndoCycle();
    }

        graphicView->redraw(RS2::RedrawDrawing);
    graphicView->moveRelativeZero(line->getMiddlePoint());
    RS_DEBUG->print("RS_ActionDrawLineHorVert::trigger():"
                    " line added: %d", line->getId());

}



void RS_ActionDrawLineHorVert::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawLineHorVert::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
	if (getStatus()==SetEndpoint && pPoints->p1.valid) {
		RS_Vector p2x = RS_Vector(mouse.x, pPoints->p1.y);
		RS_Vector p2y = RS_Vector(pPoints->p1.x, mouse.y);
        if (mouse.distanceTo(p2y) > mouse.distanceTo(p2x))
			pPoints->p2 = p2x;
        else
			pPoints->p2 = p2y;
        deletePreview();
		pPoints->data = {pPoints->p1, pPoints->p2};
		preview->addEntity(new RS_Line{preview.get(), pPoints->data});
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDrawLineHorVert::mouseMoveEvent end");
}



void RS_ActionDrawLineHorVert::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_Vector mouse = snapPoint(e);

        switch (getStatus()) {
        case SetStartpoint:
			pPoints->p1 = mouse;
            setStatus(SetEndpoint);
            break;

        case SetEndpoint:
			pPoints->p2 = mouse;
            trigger();
            setStatus(SetStartpoint);
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawLineHorVert::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetStartpoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify first point"),
                                            tr("Cancel"));
        break;
    case SetEndpoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify second point"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}


void RS_ActionDrawLineHorVert::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
