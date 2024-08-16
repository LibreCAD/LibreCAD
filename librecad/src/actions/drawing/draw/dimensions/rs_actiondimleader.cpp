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
#include<vector>


#include <QMouseEvent>

#include "rs_actiondimleader.h"
#include "rs_commandevent.h"
#include "rs_coordinateevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_leader.h"
#include "rs_line.h"
#include "rs_preview.h"
#include "rs_actioninterface.h"

struct RS_ActionDimLeader::Points {
std::vector<RS_Vector> points;
};

RS_ActionDimLeader::RS_ActionDimLeader(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw leaders",container, graphicView)
	, pPoints(std::make_unique<Points>()) {
	actionType=RS2::ActionDimLeader;
    reset();
}

RS_ActionDimLeader::~RS_ActionDimLeader() = default;

void RS_ActionDimLeader::reset() {
    //data = RS_LineData(RS_Vector(false), RS_Vector(false));
    //start = RS_Vector(false);
    //history.clear();
    pPoints->points.clear();
}

void RS_ActionDimLeader::init(int status) {
    RS_PreviewActionInterface::init(status);

    reset();
}

void RS_ActionDimLeader::trigger(){
    RS_PreviewActionInterface::trigger();

    if (!pPoints->points.empty()){

        auto *leaderEntity = new RS_Leader(container, RS_LeaderData(true));
        leaderEntity->setLayerToActive();
        leaderEntity->setPenToActive();

        for (const auto &vp: pPoints->points) {
            leaderEntity->addVertex(vp);
        }

        container->addEntity(leaderEntity);

        addToDocumentUndoable(leaderEntity);

        deletePreview();
        RS_Vector rz = graphicView->getRelativeZero();
        graphicView->redraw(RS2::RedrawDrawing);
        moveRelativeZero(rz);
        //drawSnapper();

        RS_DEBUG->print("RS_ActionDimLeader::trigger(): leaderEntity added: %lu",
                        leaderEntity->getId());
    }
}

void RS_ActionDimLeader::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionDimLeader::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
    int status = getStatus();
    switch (status) {
        case SetStartpoint:
            trySnapToRelZeroCoordinateEvent(e);
            break;
        case SetEndpoint: {
            deletePreview();
            if (!pPoints->points.empty()){
                mouse = getSnapAngleAwarePoint(e, pPoints->points.back(), mouse, true);

                // fill in lines that were already set:
                RS_Vector last(false);
                for (const auto &v: pPoints->points) {
                    if (last.valid){
                        previewLine(last, v);
                    }
                    last = v;
                }

                RS_Vector const &p = pPoints->points.back();
                if (showRefEntitiesOnPreview) {
                    previewRefPoint(p);
                    previewRefSelectablePoint(mouse);
                }
                previewLine(p, mouse);

                drawPreview();
            }
            break;
        }
        default:
            break;
    }

    RS_DEBUG->print("RS_ActionDimLeader::mouseMoveEvent end");
}

void RS_ActionDimLeader::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector snapped = snapPoint(e);
    switch (status){
        case SetStartpoint:{
            break;
        }
        case SetEndpoint:{
            if (!pPoints->points.empty()){
                snapped = getSnapAngleAwarePoint(e, pPoints->points.back(), snapped);
            }
            break;
        }
        default:
            break;
    }
    fireCoordinateEvent(snapped);
}

void RS_ActionDimLeader::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
    if (status == SetEndpoint) {
        trigger();
        reset();
        setStatus(SetStartpoint);
    } else {
        deletePreview();
        initPrevious(status);
    }
}

void RS_ActionDimLeader::keyPressEvent(QKeyEvent* e) {
    if (getStatus()==SetEndpoint && e->key()==Qt::Key_Enter) {
        trigger();
        reset();
        setStatus(SetStartpoint);
    }
}

void RS_ActionDimLeader::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &mouse) {
    switch (status) {
        case SetStartpoint: {
            pPoints->points.clear();
            pPoints->points.push_back(mouse);
            setStatus(SetEndpoint);
            moveRelativeZero(mouse);
            break;
        }
        case SetEndpoint: {
            pPoints->points.push_back(mouse);
            moveRelativeZero(mouse);
            break;
        }
        default:
            break;
    }
}

bool RS_ActionDimLeader::doProcessCommand([[maybe_unused]]int status, const QString &c) {
    // enter to finish
    bool accept = false;
    // fixme - EMPTY COMMAND!!! change to enter processing?
    if (c == ""){
        trigger();
        reset();
        setStatus(SetStartpoint);
        accept = true;
        //finish();
    }
    return accept;
}

void RS_ActionDimLeader::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetStartpoint:
            updateMouseWidgetTRCancel(tr("Specify target point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case SetEndpoint:
            updateMouseWidget(tr("Specify next point"),tr("Finish"), MOD_SHIFT_ANGLE_SNAP);
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionDimLeader::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
