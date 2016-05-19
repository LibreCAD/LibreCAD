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
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondimleader.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_leader.h"
#include "rs_line.h"
#include "rs_coordinateevent.h"
#include "rs_preview.h"
#include "rs_debug.h"

struct RS_ActionDimLeader::Points {
std::vector<RS_Vector> points;
};

RS_ActionDimLeader::RS_ActionDimLeader(RS_EntityContainer& container,
                                       RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw leaders",
                           container, graphicView)
	, pPoints(new Points{})
 {
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



void RS_ActionDimLeader::trigger() {
    RS_PreviewActionInterface::trigger();

	if (pPoints->points.size()){

        RS_Leader* leader = new RS_Leader(container, RS_LeaderData(true));
        leader->setLayerToActive();
        leader->setPenToActive();

		for(const auto& vp: pPoints->points){
			leader->addVertex(vp);
		}

        container->addEntity(leader);

        // upd. undo list:
		if (document) {
            document->startUndoCycle();
            document->addUndoable(leader);
            document->endUndoCycle();
        }

        deletePreview();
        RS_Vector rz = graphicView->getRelativeZero();
		graphicView->redraw(RS2::RedrawDrawing);
        graphicView->moveRelativeZero(rz);
        //drawSnapper();

        RS_DEBUG->print("RS_ActionDimLeader::trigger(): leader added: %d",
                        leader->getId());
    }
}



void RS_ActionDimLeader::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDimLeader::mouseMoveEvent begin");

    RS_Vector mouse = snapPoint(e);
	if (getStatus()==SetEndpoint && pPoints->points.size()) {
        deletePreview();

		// fill in lines that were already set:
		RS_Vector last(false);
		for(const auto& v: pPoints->points){
			if (last.valid) {
				preview->addEntity(new RS_Line{preview.get(), last, v});
			}
			last = v;
		}

		if (pPoints->points.size() ) {
			RS_Vector const& p = pPoints->points.back();
			preview->addEntity(new RS_Line{preview.get(), p, mouse});
        }
        drawPreview();
    }

    RS_DEBUG->print("RS_ActionDimLeader::mouseMoveEvent end");
}



void RS_ActionDimLeader::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        if (getStatus()==SetEndpoint) {
            trigger();
            reset();
            setStatus(SetStartpoint);
        } else {
            deletePreview();
            init(getStatus()-1);
        }
    }
}



void RS_ActionDimLeader::keyPressEvent(QKeyEvent* e) {
    if (getStatus()==SetEndpoint && e->key()==Qt::Key_Enter) {
        trigger();
        reset();
        setStatus(SetStartpoint);
    }
}



void RS_ActionDimLeader::coordinateEvent(RS_CoordinateEvent* e) {
	if (!e) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();

    switch (getStatus()) {
    case SetStartpoint:
        //data.startpoint = mouse;
        pPoints->points.clear();
	pPoints->points.push_back(mouse);
        //start = data.startpoint;
        setStatus(SetEndpoint);
        graphicView->moveRelativeZero(mouse);
        break;

    case SetEndpoint:
        //data.endpoint = mouse;
	pPoints->points.push_back(mouse);
        //trigger();
        //data.startpoint = data.endpoint;
        graphicView->moveRelativeZero(mouse);
        break;

    default:
        break;
    }
}



void RS_ActionDimLeader::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

	if (checkCommand("help", c)) {
		RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
										 + getAvailableCommands().join(", "));
		return;
    }

    // enter to finish
    if (c=="") {
        trigger();
        reset();
        setStatus(SetStartpoint);
        //finish();
    }
}



QStringList RS_ActionDimLeader::getAvailableCommands() {
    QStringList cmd;

    return cmd;
}

void RS_ActionDimLeader::updateMouseButtonHints() {
	switch (getStatus()) {
	case SetStartpoint:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify target point"),
											tr("Cancel"));
		break;
	case SetEndpoint:
		RS_DIALOGFACTORY->updateMouseWidget(tr("Specify next point"),
											tr("Finish"));
		break;
	default:
		RS_DIALOGFACTORY->updateMouseWidget();
		break;
	}
}


void RS_ActionDimLeader::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

// EOF
