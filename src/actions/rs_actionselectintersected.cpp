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

#include "rs_actionselectintersected.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_selection.h"



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
                            container, graphicView) {

    this->select = select;
}


QAction* RS_ActionSelectIntersected::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action;
    if (type==RS2::ActionSelectIntersected) {
        action = new QAction(tr("Select Intersected Entities"),  NULL);
		action->setIcon(QIcon(":/extui/selectinters.png"));
        //action->zetStatusTip(tr("Selects all entities intersected by a line"));
    } else {
        action = new QAction(tr("Deselect Intersected Entities"), NULL);
		action->setIcon(QIcon(":/extui/deselectinters.png"));
        //action->zetStatusTip(tr("Deselects all entities intersected by a line"));
    }
    return action;
}


void RS_ActionSelectIntersected::init(int status) {
    RS_PreviewActionInterface::init(status);

    v1 = v2 = RS_Vector(false);
    snapMode = RS2::SnapFree;
    snapRes = RS2::RestrictNothing;
}



void RS_ActionSelectIntersected::trigger() {
    RS_PreviewActionInterface::trigger();

    if (v1.valid && v2.valid) {
        if (graphicView->toGuiDX(v1.distanceTo(v2))>10) {

            RS_Selection s(*container, graphicView);
            s.selectIntersected(v1, v2, select);

            if (RS_DIALOGFACTORY!=NULL) {
                RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
            }

            init();
        }
    }
}



void RS_ActionSelectIntersected::mouseMoveEvent(QMouseEvent* e) {
    if (getStatus()==SetPoint2 && v1.valid) {
        v2 = snapPoint(e);
        if(v2.valid==false) return;
        deletePreview();
        preview->addEntity(new RS_Line(preview,
                                       RS_LineData(RS_Vector(v1.x, v1.y),
                                                   RS_Vector(v2.x, v2.y))));
        drawPreview();
    }
}



void RS_ActionSelectIntersected::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case SetPoint1:
            v1 = snapPoint(e);
            if(v1.valid==false) return;
            setStatus(SetPoint2);
            break;

        default:
            break;
        }
    }

    RS_DEBUG->print("RS_ActionSelectIntersected::mousePressEvent(): %f %f",
                    v1.x, v1.y);
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
            v2 = snapPoint(e);
            if(v2.valid==false) return;
            trigger();
        }
    }
}



void RS_ActionSelectIntersected::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetPoint1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Choose first point of intersection line"), tr("Cancel"));
            break;
        case SetPoint2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Choose second point of intersection line"), tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionSelectIntersected::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}



void RS_ActionSelectIntersected::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (!isFinished()) {
            //RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSelect);
        } else {
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSelect);
        }
    }
}

// EOF
