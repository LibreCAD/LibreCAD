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

#include "rs_actioninfoinside.h"

#include <QAction>
#include <QMouseEvent>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"

RS_ActionInfoInside::RS_ActionInfoInside(RS_EntityContainer& container,
										 RS_GraphicView& graphicView)
	:RS_ActionInterface("Info Inside",
						container, graphicView)
	, pt(new RS_Vector{})
	,contour(new RS_EntityContainer(nullptr, false))
{
	actionType=RS2::ActionInfoInside;
	for(auto e: container){
		if (e->isSelected()) {
			contour->addEntity(e);
		}
	}
}

RS_ActionInfoInside::~RS_ActionInfoInside() = default;

void RS_ActionInfoInside::trigger() {
    bool onContour = false;
	if (RS_Information::isPointInsideContour(*pt, contour.get(), &onContour)) {
        RS_DIALOGFACTORY->commandMessage(tr("Point is inside selected contour."));
    } else {
        RS_DIALOGFACTORY->commandMessage(tr("Point is outside selected contour."));
    }
    finish(false);
}

void RS_ActionInfoInside::mouseMoveEvent(QMouseEvent* e) {
    e->accept();
    //RS_Vector mouse = snapPoint(e);
    //bool onContour = false;
    /*if (RS_Information::isPointInsideContour(mouse, contour, &onContour)) {
    } else {
    }*/
}

void RS_ActionInfoInside::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::RightButton) {
        init(getStatus()-1);
    } else {
		*pt = snapPoint(e);
        trigger();
    }
}

void RS_ActionInfoInside::updateMouseButtonHints() {
    switch (getStatus()) {
    case 0:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify point"),
                                            tr("Cancel"));
        break;
    default:
		RS_DIALOGFACTORY->updateMouseWidget();
        break;
    }
}

void RS_ActionInfoInside::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
