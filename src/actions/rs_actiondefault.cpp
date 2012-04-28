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

#include "rs_actiondefault.h"

#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"
#include "rs_modification.h"
#include "rs_selection.h"
#include "rs_overlaybox.h"



/**
 * Constructor.
 */
RS_ActionDefault::RS_ActionDefault(RS_EntityContainer& container,
                                   RS_GraphicView& graphicView)
        : RS_PreviewActionInterface("Default",
                            container, graphicView) {

	RS_DEBUG->print("RS_ActionDefault::RS_ActionDefault");
	RS_DEBUG->print("RS_ActionDefault::RS_ActionDefault: OK");
}



RS_ActionDefault::~RS_ActionDefault() {
}



QAction* RS_ActionDefault::createGUIAction(RS2::ActionType /*type*/, 
	QObject* /*parent*/) {

	return NULL;
}


void RS_ActionDefault::init(int status) {
	RS_DEBUG->print("RS_ActionDefault::init");
    
	RS_PreviewActionInterface::init(status);
    v1 = v2 = RS_Vector(false);
    snapMode = RS2::SnapFree;
    snapRes = RS2::RestrictNothing;
    restrBak = RS2::RestrictNothing;
	RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
	
	RS_DEBUG->print("RS_ActionDefault::init: OK");
}



void RS_ActionDefault::trigger() {
    RS_PreviewActionInterface::trigger();

}

void RS_ActionDefault::keyPressEvent(QKeyEvent* e) {
	if (e->key()==Qt::Key_Shift) {
		restrBak = snapRes;
		setSnapRestriction(RS2::RestrictOrthogonal);
	}
}

void RS_ActionDefault::keyReleaseEvent(QKeyEvent* e) {
	if (e->key()==Qt::Key_Shift) {
		setSnapRestriction(restrBak);
	}
}


void RS_ActionDefault::mouseMoveEvent(QMouseEvent* e) {

    RS_Vector mouse = graphicView->toGraph(RS_Vector(e->x(), e->y()));
    RS_Vector relMouse = mouse - graphicView->getRelativeZero();

    RS_DIALOGFACTORY->updateCoordinateWidget(mouse, relMouse);

    switch (getStatus()) {
    case Dragging:
        //v2 = graphicView->toGraph(e->x(), e->y());
        v2 = mouse;

        if (graphicView->toGuiDX(v1.distanceTo(v2))>10) {
            // look for reference points to drag:
            double dist;
            RS_Vector ref = container->getNearestSelectedRef(v1, &dist);
            if (ref.valid==true && graphicView->toGuiDX(dist)<8) {
				RS_DEBUG->print("RS_ActionDefault::mouseMoveEvent: "
					"moving reference point");
                setStatus(MovingRef);
                v1 = ref;
    			graphicView->moveRelativeZero(v1);
            } 
			else {
                // test for an entity to drag:
                RS_Entity* en = catchEntity(v1);
                if (en!=NULL && en->isSelected()) {
					RS_DEBUG->print("RS_ActionDefault::mouseMoveEvent: "
						"moving entity");
                    setStatus(Moving);
                    v1 = en->getNearestRef(v1);
    				graphicView->moveRelativeZero(v1);
                }

                // no entity found. start area selection:
                else {
                    setStatus(SetCorner2);
                }
            }
        }
        break;
		
    case MovingRef:
        v2 = snapPoint(e);
        if(v2.valid==false) return;

        deletePreview();
        preview->addSelectionFrom(*container);
		preview->moveRef(v1, v2-v1);
        drawPreview();
        break;

    case Moving:
        v2 = snapPoint(e);
        if(v2.valid==false) return;

        deletePreview();
        preview->addSelectionFrom(*container);
        preview->move(v2-v1);
        drawPreview();
        break;

    case SetCorner2:
        if (v1.valid) {
            //v2 = snapPoint(e);
            v2 = mouse;

            deletePreview();
			
			RS_Pen pen_f(RS_Color(50,50,255,40), RS2::Width00, RS2::SolidLine);
			RS_OverlayBox* ob=new RS_OverlayBox(preview, RS_OverlayBoxData(v1, v2));
			ob->setPen(pen_f);	
			preview->addEntity(ob);
			
			RS_Pen pen(RS_Color(218,105,24), RS2::Width00, RS2::SolidLine);
			pen.setScreenWidth(1);			

			// TODO change to a rs_box sort of entity
			RS_Line* e=new RS_Line(preview, RS_LineData(RS_Vector(v1.x, v1.y),  RS_Vector(v2.x, v1.y)));
			e->setPen(pen);
			preview->addEntity(e);
			
			e=new RS_Line(preview, RS_LineData(RS_Vector(v2.x, v1.y),  RS_Vector(v2.x, v2.y)));			
			e->setPen(pen);
			preview->addEntity(e);
			
			e=new RS_Line(preview, RS_LineData(RS_Vector(v2.x, v2.y),  RS_Vector(v1.x, v2.y)));
			e->setPen(pen);
			preview->addEntity(e);
			
			e=new RS_Line(preview, RS_LineData(RS_Vector(v1.x, v2.y),  RS_Vector(v1.x, v1.y)));
			e->setPen(pen);
			preview->addEntity(e);
			
            drawPreview();
        }

    default:
        break;
    }
}



void RS_ActionDefault::mousePressEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        switch (getStatus()) {
        case Neutral:
            v1 = graphicView->toGraph(e->x(), e->y());
            setStatus(Dragging);
            break;

        case Moving: {
                v2 = snapPoint(e);
                if(v2.valid==false) return;
                deletePreview();
                RS_Modification m(*container, graphicView);
                RS_MoveData data;
                data.number = 0;
                data.useCurrentLayer = false;
                data.useCurrentAttributes = false;
                data.offset = v2-v1;
                m.move(data);
                setStatus(Neutral);
                RS_DIALOGFACTORY->updateSelectionWidget(
                        container->countSelected());
                                RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
            }
            break;
			
        case MovingRef: {
        		v2 = snapPoint(e);
                if(v2.valid==false) return;
                deletePreview();
                RS_Modification m(*container, graphicView);
                RS_MoveRefData data;
				data.ref = v1;
				data.offset = v2-v1;
                m.moveRef(data);
				//container->moveSelectedRef(v1, v2-v2);
                setStatus(Neutral);
                RS_DIALOGFACTORY->updateSelectionWidget(
                        container->countSelected());
				RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
            }
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        //cleanup
        cleanUpAction(e);
    }
}



void RS_ActionDefault::mouseReleaseEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDefault::mouseReleaseEvent()");

    if (e->button()==Qt::LeftButton) {
        v2 = graphicView->toGraph(e->x(), e->y());
        switch (getStatus()) {
        case Dragging: {
                // select single entity:
                RS_Entity* en = catchEntity(e);

                if (en!=NULL) {
                    deletePreview();

                    RS_Selection s(*container, graphicView);
                    s.selectSingle(en);

                    RS_DIALOGFACTORY->updateSelectionWidget(
                        container->countSelected());

                    e->accept();

                    setStatus(Neutral);
                } else {
                    setStatus(SetCorner2);
                }
            }
            break;

        case SetCorner2: {
                //v2 = snapPoint(e);
                v2 = graphicView->toGraph(e->x(), e->y());

                // select window:
                //if (graphicView->toGuiDX(v1.distanceTo(v2))>20) {
                deletePreview();

                bool cross = (v2.y>v1.y);
                RS_Selection s(*container, graphicView);
                s.selectWindow(v1, v2, true, cross);

                RS_DIALOGFACTORY->updateSelectionWidget(
                    container->countSelected());

                setStatus(Neutral);
                e->accept();
                //}
            }
            break;


        default:
            break;

        }
    } else if (e->button()==Qt::RightButton) {
        //cleanup
        cleanUpAction(e);
    }
}


// cancel the current action and cleanup
void RS_ActionDefault::cleanUpAction(QMouseEvent* e){
    switch (getStatus()) {
    case SetCorner2:
    case Moving:
    case MovingRef:
        deletePreview();
        deleteSnapper();
        setStatus(Neutral);
        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
        e->accept();
        break;

    default:
        RS_DIALOGFACTORY->requestPreviousMenu();
        e->accept();
        break;
    }
}


void RS_ActionDefault::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    // if the current action can't deal with the command,
    //   it might be intended to launch a new command
    //if (!e.isAccepted()) {
    // command for new action:
    //RS2::ActionType type = RS_COMMANDS->cmdToAction(c);
    //if (type!=RS2::ActionNone) {
    //graphicView->setCurrentAction(type);
    //return true;
    //}
    //}
}



QStringList RS_ActionDefault::getAvailableCommands() {
    QStringList cmd;

    //cmd += "line";
    //cmd += "rectangle";

    return cmd;
}


void RS_ActionDefault::updateMouseButtonHints() {
    switch (getStatus()) {
    case Neutral:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    case SetCorner2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Choose second edge"),
                                            tr("Back"));
        break;
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionDefault::updateMouseCursor() {
	switch (getStatus()) {
	case Neutral:
    	graphicView->setMouseCursor(RS2::ArrowCursor);
		break;
	case Moving:
	case MovingRef:
    	graphicView->setMouseCursor(RS2::SelectCursor);
		break;
	default:
		break;
	}
}



void RS_ActionDefault::updateToolBar() {
    //RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
	switch (getStatus()) {
	case Neutral:
		// would switch back to main in edit / measure / .. modes
		//RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);
		break;
	case Moving:
	case MovingRef:
		RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarSnap);
		break;
	default:
		break;
	}
}

// EOF
