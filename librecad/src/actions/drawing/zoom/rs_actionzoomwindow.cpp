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


#include "rs_actionzoomwindow.h"

#include <QMouseEvent>

#include "lc_graphicviewport.h"
#include "rs_debug.h"
#include "rs_preview.h"

struct RS_ActionZoomWindow::ActionData {
 RS_Vector ucsV1;
	RS_Vector v1;
	RS_Vector v2;
};

/**
 * Default constructor.
 *
 * @param keepAspectRatio Keep the aspect ratio. true: the factors
 *          in x and y will stay the same. false Exactly the chosen
 *          area will be fit to the viewport.
 */
RS_ActionZoomWindow::RS_ActionZoomWindow(LC_ActionContext *actionContext, bool keepAspectRatio)
    :RS_PreviewActionInterface("Zoom Window",actionContext, RS2::ActionZoomWindow)
    , m_actionData(std::make_unique<ActionData>()), m_keepAspectRatio(keepAspectRatio){
}

RS_ActionZoomWindow::~RS_ActionZoomWindow() = default;

void RS_ActionZoomWindow::init(int status){
    RS_DEBUG->print("RS_ActionZoomWindow::init()");

    RS_PreviewActionInterface::init(status);
    m_actionData.reset(new ActionData{});
//deleteSnapper();
    // snapMode.clear();
    // snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionZoomWindow::doTrigger() {
    RS_DEBUG->print("RS_ActionZoomWindow::trigger()");
    if (m_actionData->v1.valid && m_actionData->v2.valid){
        if (m_viewport->toGuiDX(m_actionData->v1.distanceTo(m_actionData->v2)) > 5){
            RS_Vector point1 = toUCS(m_actionData->v1);
            RS_Vector point2 = toUCS(m_actionData->v2);
            m_viewport->zoomWindow(point1, point2, m_keepAspectRatio);
            init(SetFirstCorner);
        }
    }
}

void RS_ActionZoomWindow::mouseMoveEvent(QMouseEvent *e){
    deletePreview();
    snapFree(e);
    drawSnapper();
    if (getStatus() == SetSecondCorner && m_actionData->v1.valid){
        m_actionData->v2 = snapFree(e);

        RS_Vector worldCorner1 = m_actionData->v1;
        RS_Vector worldCorner3 = m_actionData->v2;

        RS_Vector worldCorner2,worldCorner4;
        calcRectCorners(worldCorner1, worldCorner3, worldCorner2, worldCorner4);

        m_preview->addRectangle(worldCorner1, worldCorner2, worldCorner3, worldCorner4);
    }
    drawPreview();
}

void RS_ActionZoomWindow::mousePressEvent(QMouseEvent *e){
    if (e->button() == Qt::LeftButton){
        switch (getStatus()) {
            case SetFirstCorner:
                m_actionData->v1 = snapFree(e);
                drawSnapper();
                setStatus(SetSecondCorner);
                break;

            default:
                break;
        }
    }

    RS_DEBUG->print("RS_ActionZoomWindow::mousePressEvent(): %f %f",
                    m_actionData->v1.x, m_actionData->v1.y);
}

void RS_ActionZoomWindow::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionZoomWindow::mouseReleaseEvent()");
    if (status == SetSecondCorner){
        m_actionData->v2 = e->graphPoint;
        if (fabs(m_actionData->v1.x - m_actionData->v2.x) < RS_TOLERANCE
            || fabs(m_actionData->v1.y - m_actionData->v2.y) < RS_TOLERANCE){//invalid zoom window
            deletePreview();
            initPrevious(status);
        }
        trigger();
    }
}

void RS_ActionZoomWindow::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionZoomWindow::mouseReleaseEvent()");
    if (status == SetSecondCorner){
        deletePreview();
    }
    initPrevious(status);
}

void RS_ActionZoomWindow::updateMouseButtonHints(){
    RS_DEBUG->print("RS_ActionZoomWindow::updateMouseButtonHints()");

    switch (getStatus()) {
        case SetFirstCorner:
            updateMouseWidgetTRCancel(tr("Specify first edge"));
            break;
        case SetSecondCorner:
            updateMouseWidgetTRBack(tr("Specify second edge"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionZoomWindow::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::MagnifierCursor;
}
