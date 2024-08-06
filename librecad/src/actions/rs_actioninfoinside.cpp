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

#include "rs_actioninfoinside.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"

RS_ActionInfoInside::RS_ActionInfoInside(RS_EntityContainer& container,
										 RS_GraphicView& graphicView)
	:RS_ActionInterface("Info Inside",
                        container, graphicView)
    , pt(std::make_unique<RS_Vector>())
    ,contour(std::make_unique<RS_EntityContainer>()){
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
        commandMessage(tr("Point is inside selected contour."));
    } else {
        commandMessage(tr("Point is outside selected contour."));
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

void RS_ActionInfoInside::onMouseLeftButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    *pt = snapPoint(e);
    trigger();
}

void RS_ActionInfoInside::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    initPrevious(status);
}

void RS_ActionInfoInside::updateMouseButtonHints() {
    switch (getStatus()) {
        case 0:
            updateMouseWidgetTRCancel(tr("Specify point"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionInfoInside::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
