/****************************************************************************
**
 * Draw circle by foci and a point on circle

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "rs_actiondrawcircleinscribe.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_commandevent.h"

/**
 * Constructor.
 *
 */
RS_ActionDrawCircleInscribe::RS_ActionDrawCircleInscribe(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Draw circle inscribed",
                           container, graphicView),
          cData(RS_Vector(0.,0.),1.)
{
}



RS_ActionDrawCircleInscribe::~RS_ActionDrawCircleInscribe() {
    lines.clear();
}


QAction* RS_ActionDrawCircleInscribe::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
    QAction* action;

    action = new QAction(tr("Circle &Inscribed"), NULL);
    action->setIcon(QIcon(":/extui/circleinscribe.png"));
    return action;
}

void RS_ActionDrawCircleInscribe::init(int status) {
    RS_PreviewActionInterface::init(status);

    if (status==SetLine1) {
        lines.clear();
    }
}

void RS_ActionDrawCircleInscribe::finish(bool updateTB){
    for(int i=0;i<lines.size();i++) lines[i]->setHighlighted(false);
    graphicView->redraw(RS2::RedrawDrawing);
    lines.clear();
    RS_PreviewActionInterface::finish(updateTB);
}


void RS_ActionDrawCircleInscribe::trigger() {
    RS_PreviewActionInterface::trigger();


    RS_Circle* circle=new RS_Circle(container, cData);

    deletePreview();
    container->addEntity(circle);

    // upd. undo list:
    if (document!=NULL) {
        document->startUndoCycle();
        document->addUndoable(circle);
        document->endUndoCycle();
    }

    for(int i=0;i<lines.size();i++) lines[i]->setHighlighted(false);
    graphicView->redraw(RS2::RedrawDrawing);
//    drawSnapper();

    lines.clear();
    setStatus(SetLine1);

    RS_DEBUG->print("RS_ActionDrawCircle4Line::trigger():"
                    " entity added: %d", circle->getId());
}



void RS_ActionDrawCircleInscribe::mouseMoveEvent(QMouseEvent* e) {
    RS_DEBUG->print("RS_ActionDrawCircle4Line::mouseMoveEvent begin");

    if(getStatus() == SetLine3) {
        RS_Entity*  en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
        if(en == NULL) return;
        if(!(en->isVisible() && en->rtti()== RS2::EntityLine)) return;
        for(int i=0;i<getStatus();i++) {
            if(en->getId() == lines[i]->getId()) return; //do not pull in the same line again
        }
        if(en->getParent() != NULL) {
            if ( en->getParent()->rtti() == RS2::EntityInsert         /**Insert*/
                 || en->getParent()->rtti() == RS2::EntitySpline
                 || en->getParent()->rtti() == RS2::EntityText         /**< Text 15*/
                 || en->getParent()->rtti() == RS2::EntityDimAligned   /**< Aligned Dimension */
                 || en->getParent()->rtti() == RS2::EntityDimLinear    /**< Linear Dimension */
                 || en->getParent()->rtti() == RS2::EntityDimRadial    /**< Radial Dimension */
                 || en->getParent()->rtti() == RS2::EntityDimDiametric /**< Diametric Dimension */
                 || en->getParent()->rtti() == RS2::EntityDimAngular   /**< Angular Dimension */
                 || en->getParent()->rtti() == RS2::EntityDimLeader    /**< Leader Dimension */
                 ){
                return;
            }
        }
        coord=snapPoint(e);
        deleteSnapper();
        lines.resize(getStatus());
        lines.push_back(static_cast<RS_Line*>(en));
//        lines[getStatus()]=static_cast<RS_Line*>(en);
        if(preparePreview()) {
            deletePreview();
            RS_Circle* e=new RS_Circle(preview, cData);
            preview->addEntity(e);
            drawPreview();
        }

    }
    RS_DEBUG->print("RS_ActionDrawCircle4Line::mouseMoveEvent end");
}


bool RS_ActionDrawCircleInscribe::preparePreview(){
    valid=false;
    if(getStatus() == SetLine3) {
        RS_Circle c(preview,cData);
        valid= c.createInscribe(coord, lines);
        if(valid){
            cData=c.getData();
        }
    }
    return valid;
}

void RS_ActionDrawCircleInscribe::mouseReleaseEvent(QMouseEvent* e) {
    // Proceed to next status
    if (e->button()==Qt::LeftButton) {
        if (e==NULL) {
            return;
        }
        RS_Entity*  en = catchEntity(e, RS2::EntityLine, RS2::ResolveAll);
        if(en == NULL) return;
        if(!(en->isVisible() && en->rtti()== RS2::EntityLine)) return;
        for(int i=0;i<getStatus();i++) {
            if(en->getId() == lines[i]->getId()) return; //do not pull in the same line again
        }
        if(en->getParent() != NULL) {
            if ( en->getParent()->rtti() == RS2::EntityInsert         /**Insert*/
                    || en->getParent()->rtti() == RS2::EntitySpline
                    || en->getParent()->rtti() == RS2::EntityText         /**< Text 15*/
                    || en->getParent()->rtti() == RS2::EntityDimAligned   /**< Aligned Dimension */
                    || en->getParent()->rtti() == RS2::EntityDimLinear    /**< Linear Dimension */
                    || en->getParent()->rtti() == RS2::EntityDimRadial    /**< Radial Dimension */
                    || en->getParent()->rtti() == RS2::EntityDimDiametric /**< Diametric Dimension */
                    || en->getParent()->rtti() == RS2::EntityDimAngular   /**< Angular Dimension */
                    || en->getParent()->rtti() == RS2::EntityDimLeader    /**< Leader Dimension */
                    ){
                return;
        }
        }
        lines.resize(getStatus());
        lines.push_back(static_cast<RS_Line*>(en));
coord=snapPoint(e);
        deleteSnapper();
        switch (getStatus()) {
        case SetLine1:
        case SetLine2:
            en->setHighlighted(true);
            setStatus(getStatus()+1);
            graphicView->redraw(RS2::RedrawDrawing);
            break;
        case SetLine3:
            if( preparePreview()) {
                trigger();
            }

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        // Return to last status:
        if(getStatus()>0){
            lines[getStatus()-1]->setHighlighted(false);
            lines.pop_back();
            graphicView->redraw(RS2::RedrawDrawing);
            deletePreview();

        }
        init(getStatus()-1);
    }
}


//void RS_ActionDrawCircleInscribe::coordinateEvent(RS_CoordinateEvent* e) {

//}

//fixme, support command line

/*
void RS_ActionDrawCircle4Line::commandEvent(RS_CommandEvent* e) {
    QString c = e->getCommand().toLower();

    if (checkCommand("help", c)) {
        if (RS_DIALOGFACTORY!=NULL) {
            RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                             + getAvailableCommands().join(", "));
        }
        return;
    }

    switch (getStatus()) {
    case SetFocus1: {
            bool ok;
            double m = RS_Math::eval(c, &ok);
            if (ok==true) {
                ratio = m / major.magnitude();
                if (!isArc) {
                    trigger();
                } else {
                    setStatus(SetAngle1);
                }
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    case SetAngle1: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                angle1 = RS_Math::deg2rad(a);
                setStatus(SetAngle2);
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    case SetAngle2: {
            bool ok;
            double a = RS_Math::eval(c, &ok);
            if (ok==true) {
                angle2 = RS_Math::deg2rad(a);
                trigger();
            } else {
                if (RS_DIALOGFACTORY!=NULL) {
                    RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
                }
            }
        }
        break;

    default:
        break;
    }
}
*/


QStringList RS_ActionDrawCircleInscribe::getAvailableCommands() {
    QStringList cmd;
    return cmd;
}



void RS_ActionDrawCircleInscribe::updateMouseButtonHints() {
    if (RS_DIALOGFACTORY!=NULL) {
        switch (getStatus()) {
        case SetLine1:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the first line"),
                                                tr("Cancel"));
            break;

        case SetLine2:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the second line"),
                                                tr("Back"));
            break;

        case SetLine3:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Specify the third line"),
                                                tr("Back"));
            break;

        default:
            RS_DIALOGFACTORY->updateMouseWidget("", "");
            break;
        }
    }
}



void RS_ActionDrawCircleInscribe::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionDrawCircleInscribe::updateToolBar() {
    if (RS_DIALOGFACTORY!=NULL) {
        if (isFinished()) {
            RS_DIALOGFACTORY->resetToolBar();
        }
    }
}

// EOF
