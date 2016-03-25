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
#include "rs_actionselectintersected.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_selection.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_debug.h"

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
    RS_EntityContainer& container,
    RS_GraphicView& graphicView,
    bool select)
        : RS_PreviewActionInterface("Select Intersected",
							container, graphicView)
		, pPoints(new Points{})
		,select(select)
{
	actionType=RS2::ActionSelectIntersected;
}

RS_ActionSelectIntersected::~RS_ActionSelectIntersected() = default;

void RS_ActionSelectIntersected::init(int status) {
    RS_PreviewActionInterface::init(status);
	pPoints.reset(new Points{});
    snapMode.clear();
    snapMode.restriction = RS2::RestrictNothing;
}



void RS_ActionSelectIntersected::trigger() {
    RS_PreviewActionInterface::trigger();

	if (pPoints->v1.valid && pPoints->v2.valid) {
		if (graphicView->toGuiDX(pPoints->v1.distanceTo(pPoints->v2))>10) {

            RS_Selection s(*container, graphicView);
			s.selectIntersected(pPoints->v1, pPoints->v2, select);

			RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected(),container->totalSelectedLength());

            init();
        }
    }
}



void RS_ActionSelectIntersected::mouseMoveEvent(QMouseEvent* e) {
	if (getStatus()==SetPoint2 && pPoints->v1.valid) {
		pPoints->v2 = snapPoint(e);
        deletePreview();
		preview->addEntity(new RS_Line{preview.get(), pPoints->v1, pPoints->v2});
        drawPreview();
    }
}



void RS_ActionSelectIntersected::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetPoint1:
			pPoints->v1 = snapPoint(e);
            setStatus(SetPoint2);
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionSelectIntersected::mousePressEvent(): %f %f",
					pPoints->v1.x, pPoints->v1.y);
}



void RS_ActionSelectIntersected::mouseReleaseEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionSelectIntersected::mouseReleaseEvent()");
    if (e->button()==Qt::RightButton) {
        if (getStatus()==SetPoint2) {
            deletePreview();
        }
        init(getStatus()-1);
    } else if (e->button()==Qt::LeftButton) {
        if (getStatus()==SetPoint2) {
			pPoints->v2 = snapPoint(e);
            trigger();
        }
    }
}



void RS_ActionSelectIntersected::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetPoint1:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Choose first point of intersection line"), tr("Cancel"));
		break;
	case SetPoint2:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Choose second point of intersection line"), tr("Back"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}

void RS_ActionSelectIntersected::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
