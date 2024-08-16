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

#include "rs_actionsnapintersectionmanual.h"
#include "rs_circle.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_preview.h"

/**
 * @param both Trim both entities.
 */
RS_ActionSnapIntersectionManual::RS_ActionSnapIntersectionManual(
		RS_EntityContainer& container,
		RS_GraphicView& graphicView)
	:RS_PreviewActionInterface("Trim Entity",
							   container, graphicView)
	,entity1(nullptr)
    ,entity2(nullptr)
    ,coord(std::make_unique<RS_Vector>()){
}

RS_ActionSnapIntersectionManual::~RS_ActionSnapIntersectionManual()=default;

void RS_ActionSnapIntersectionManual::init(int status){
    RS_ActionInterface::init(status);
    snapMode.clear();
}

void RS_ActionSnapIntersectionManual::trigger(){

    RS_DEBUG->print("RS_ActionSnapIntersectionManual::trigger()");

    if (entity2 && entity2->isAtomic() &&
        entity1 && entity1->isAtomic()){

        RS_VectorSolutions sol =
            RS_Information::getIntersection(entity1, entity2, false);

        entity2 = nullptr;
        entity1 = nullptr;
        if (predecessor){
            RS_Vector ip = sol.getClosest(*coord);

            if (ip.valid){
                RS_CoordinateEvent e(ip);
                predecessor->coordinateEvent(&e);
            }
        }
        finish(false);
    }
}

void RS_ActionSnapIntersectionManual::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionSnapIntersectionManual::mouseMoveEvent begin");

    RS_Entity *se = catchEntity(e);
    RS_Vector mouse = toGraph(e);

    switch (getStatus()) {
        case ChooseEntity1:
            entity1 = se;
            break;

        case ChooseEntity2: {
            entity2 = se;
            *coord = mouse;

            RS_VectorSolutions sol =
                RS_Information::getIntersection(entity1, entity2, false);

            //for (int i=0; i<sol.getNumber(); i++) {
            //    ip = sol.get(i);
            //    break;
            //}

            RS_Vector ip = sol.getClosest(*coord);

            if (ip.valid){
                deletePreview();
                preview->addEntity(
                    new RS_Circle(preview.get(),
                                  {ip, graphicView->toGraphDX(4)}));
                drawPreview();

                updateCoordinateWidgetByRelZero(ip);

            }
        }
            break;

        default:
            break;
    }

    RS_DEBUG->print("RS_ActionSnapIntersectionManual::mouseMoveEvent end");
}

void RS_ActionSnapIntersectionManual::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector mouse = toGraph(e);
    RS_Entity *se = catchEntity(e);

    switch (status) {
        case ChooseEntity1:
            entity1 = se;
            if (entity1 && entity1->isAtomic()){
                setStatus(ChooseEntity2);
            }
            break;

        case ChooseEntity2:
            entity2 = se;
            *coord = mouse;
            if (entity2 && entity2->isAtomic() && coord->valid){
                trigger();
            }
            break;

        default:
            break;
    }
}

void RS_ActionSnapIntersectionManual::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionSnapIntersectionManual::updateMouseButtonHints() {
    switch (getStatus()) {
        case ChooseEntity1:
            updateMouseWidgetTRCancel(tr("Select first entity"));
            break;
        case ChooseEntity2:
            updateMouseWidgetTRBack(tr("Select second entity"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionSnapIntersectionManual::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
