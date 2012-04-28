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

#include "rs_actionmodifyscale.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"



RS_ActionModifyScale::RS_ActionModifyScale(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Scale Entities",
                           container, graphicView) {}


QAction* RS_ActionModifyScale::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// "Scale"
    QAction* action = new QAction(tr("&Scale"),  NULL);
	action->setIcon(QIcon(":/extui/modifyscale.png"));
    //action->zetStatusTip(tr("Scale Entities"));
    return action;
}

void RS_ActionModifyScale::init(int status) {
    RS_ActionInterface::init(status);

}



void RS_ActionModifyScale::trigger() {

    RS_DEBUG->print("RS_ActionModifyScale::trigger()");

    RS_Modification m(*container, graphicView);
    m.scale(data);

    RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
}



void RS_ActionModifyScale::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyScale::mouseMoveEvent begin");

    if (getStatus()==SetReferencePoint) {

        RS_Vector mouse = snapPoint(e);
        if(mouse.valid==false) return;
        switch (getStatus()) {
        case SetReferencePoint:
            referencePoint = mouse;
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionModifyScale::mouseMoveEvent end");
}



void RS_ActionModifyScale::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {

        if (getStatus()== SetReferencePoint){
            RS_Vector vp=snapPoint(e);
            if(vp.valid==false) return;
            RS_CoordinateEvent ce(vp);
            coordinateEvent(&ce);
        }
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void RS_ActionModifyScale::coordinateEvent(RS_CoordinateEvent* e) {

    if (e==NULL || getStatus() != SetReferencePoint) {
        return;
    }

    RS_Vector mouse = e->getCoordinate();
    setStatus(ShowDialog);
    if (RS_DIALOGFACTORY->requestScaleDialog(data)) {
        data.referencePoint = mouse;
        trigger();
        finish();
    }
}

void RS_ActionModifyScale::updateMouseButtonHints() {
    switch (getStatus()) {
        /*case Select:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Pick entities to scale"),
                                           tr("Cancel"));
            break;*/
    case SetReferencePoint:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Specify reference point"),
                                            tr("Cancel"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyScale::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyScale::updateToolBar() {
    switch (getStatus()) {
        /*case Select:
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSelect);
            break;*/
    case SetReferencePoint:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
        break;
    default:
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
        break;
    }
}


// EOF
