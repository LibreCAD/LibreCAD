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

#include "rs_actionselectwindow.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_overlaybox.h"
#include "rs_preview.h"
#include "rs_selection.h"

struct RS_ActionSelectWindow::Points {
    RS_Vector v1;
    RS_Vector v2;
};

/**
 * Constructor.
 *
 * @param select true: select window. false: deselect window
 */
RS_ActionSelectWindow::RS_ActionSelectWindow(RS_EntityContainer& container,
                                             RS_GraphicView& graphicView,
                                             bool select)
    : RS_PreviewActionInterface("Select Window",
                                container, graphicView)
    , pPoints(std::make_unique<Points>())
    , select(select)
{
    actionType=RS2::ActionSelectWindow;
}

RS_ActionSelectWindow::RS_ActionSelectWindow(
        enum RS2::EntityType typeToSelect,
        RS_EntityContainer& container,
        RS_GraphicView& graphicView,
        bool select)
    : RS_PreviewActionInterface("Select Window",
                                container, graphicView)
    , pPoints(std::make_unique<Points>())
    , typeToSelect(typeToSelect)
    , select(select)
{
    actionType=RS2::ActionSelectWindow;
}

RS_ActionSelectWindow::~RS_ActionSelectWindow() = default;


void RS_ActionSelectWindow::init(int status) {
    RS_PreviewActionInterface::init(status);
    pPoints = std::make_unique<Points>();
    selectIntersecting = false;
    //snapMode.clear();
    //snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionSelectWindow::trigger(){
    RS_PreviewActionInterface::trigger();

    if (pPoints->v1.valid && pPoints->v2.valid){
        if (graphicView->toGuiDX(pPoints->v1.distanceTo(pPoints->v2)) > 10){

            bool cross = (pPoints->v1.x > pPoints->v2.x) || selectIntersecting;
            RS_Selection s(*container, graphicView);
            s.selectWindow(typeToSelect, pPoints->v1, pPoints->v2, select, cross);
            updateSelectionWidget();
            init(SetCorner1);
        }
    }
}

void RS_ActionSelectWindow::mouseMoveEvent(QMouseEvent* e) {
    drawSnapper();
    RS_Vector snapped = toGraph(e);
    updateCoordinateWidgetByRelZero(snapped);
    if (getStatus()==SetCorner2 && pPoints->v1.valid) {
        pPoints->v2 = snapped;
        deletePreview();
        auto* ob=new RS_OverlayBox(preview.get(), RS_OverlayBoxData(pPoints->v1, pPoints->v2));
        preview->addEntity(ob);

        //RLZ: not needed overlay have contour
        /*                RS_Pen pen(RS_Color(218,105,24), RS2::Width00, RS2::SolidLine);

                // TODO change to a rs_box sort of entity
                RS_Line* e=new RS_Line(preview, RS_LineData(RS_Vector(v1->x, v1->y),  RS_Vector(v2->x, v1->y)));
                e->setPen(pen);
        preview->addEntity(e);

                e=new RS_Line(preview, RS_LineData(RS_Vector(v2->x, v1->y),  RS_Vector(v2->x, v2->y)));
                e->setPen(pen);
        preview->addEntity(e);

                e=new RS_Line(preview, RS_LineData(RS_Vector(v2->x, v2->y),  RS_Vector(v1->x, v2->y)));
                e->setPen(pen);
        preview->addEntity(e);

                e=new RS_Line(preview, RS_LineData(RS_Vector(v1->x, v2->y),  RS_Vector(v1->x, v1->y)));
                e->setPen(pen);
        preview->addEntity(e);*/

        drawPreview();
    }
}

void RS_ActionSelectWindow::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetCorner1:
            pPoints->v1 = toGraph(e);
            setStatus(SetCorner2);
            break;
        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionSelectWindow::mousePressEvent(): %f %f",
                    pPoints->v1.x, pPoints->v1.y);
}

void RS_ActionSelectWindow::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionSelectWindow::mouseReleaseEvent()");
    if (status==SetCorner2) {
        pPoints->v2 = toGraph(e);
        selectIntersecting = isShift(e);
        trigger();
    }
}

void RS_ActionSelectWindow::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionSelectWindow::mouseReleaseEvent()");
    if (status==SetCorner2) {
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionSelectWindow::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetCorner1:
            updateMouseWidgetTRCancel(tr("Click and drag for the selection window"));
            break;
        case SetCorner2:
            updateMouseWidgetTRBack(tr("Choose second edge"), MOD_SHIFT(tr("Select Intersecting")));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionSelectWindow::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

enum RS2::EntityType RS_ActionSelectWindow::getTypeToSelect(){
    return typeToSelect;
}
