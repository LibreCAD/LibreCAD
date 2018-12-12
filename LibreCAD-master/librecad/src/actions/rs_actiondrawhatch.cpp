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
#include <iostream>
#include <QAction>
#include <QMouseEvent>
#include "rs_actiondrawhatch.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_hatch.h"
#include "rs_debug.h"

RS_ActionDrawHatch::RS_ActionDrawHatch(RS_EntityContainer& container, RS_GraphicView& graphicView)
                                :RS_PreviewActionInterface("Draw Hatch", container, graphicView)
								, data{new RS_HatchData{}}
{
    actionType = RS2::ActionDrawHatch;
}

RS_ActionDrawHatch::~RS_ActionDrawHatch() = default;

void RS_ActionDrawHatch::setShowArea(bool s){
	m_bShowArea=s;
}

void RS_ActionDrawHatch::init(int status) {
    RS_ActionInterface::init(status);

	RS_Hatch tmp(container, *data);
    tmp.setLayerToActive();
    tmp.setPenToActive();
    if (RS_DIALOGFACTORY->requestHatchDialog(&tmp)) {
		*data = tmp.getData();
        trigger();
        finish(false);
        graphicView->redraw(RS2::RedrawDrawing); 

    } else {
        finish(false);
    }
}

void RS_ActionDrawHatch::trigger() {

    RS_DEBUG->print("RS_ActionDrawHatch::trigger()");

    //if (pos.valid) {
    //deletePreview();
	RS_Entity* e;

	// deselect unhatchable entities:
	for(auto e: *container){
        if (e->isSelected() && 
            (e->rtti()==RS2::EntityHatch ||
            /* e->rtti()==RS2::EntityEllipse ||*/ e->rtti()==RS2::EntityPoint ||
             e->rtti()==RS2::EntityMText || e->rtti()==RS2::EntityText ||
			 RS_Information::isDimension(e->rtti()))) {
			e->setSelected(false);
        }
    }
	for (e=container->firstEntity(RS2::ResolveAll); e;
            e=container->nextEntity(RS2::ResolveAll)) {
        if (e->isSelected() && 
            (e->rtti()==RS2::EntityHatch ||
            /* e->rtti()==RS2::EntityEllipse ||*/ e->rtti()==RS2::EntityPoint ||
             e->rtti()==RS2::EntityMText || e->rtti()==RS2::EntityText ||
			 RS_Information::isDimension(e->rtti()))) {
			e->setSelected(false);
        }
    }

	// look for selected contours:
    bool haveContour = false;
	for (e=container->firstEntity(RS2::ResolveAll); e;
            e=container->nextEntity(RS2::ResolveAll)) {
        if (e->isSelected()) {
            haveContour = true;
        }
    }

    if (!haveContour) {
        std::cerr << "no contour selected\n";
        return;
    }

	std::unique_ptr<RS_Hatch> hatch{new RS_Hatch{container, *data}};
    hatch->setLayerToActive();
    hatch->setPenToActive();
	RS_EntityContainer* loop = new RS_EntityContainer(hatch.get());
    loop->setPen(RS_Pen(RS2::FlagInvalid));

    // add selected contour:
	for (RS_Entity* e=container->firstEntity(RS2::ResolveAll); e;
            e=container->nextEntity(RS2::ResolveAll)) {

        if (e->isSelected()) {
            e->setSelected(false);
			// entity is part of a complex entity (spline, polyline, ..):
			if (e->getParent() &&
// RVT - Don't de-delect the parent EntityPolyline, this is messing up the getFirst and getNext iterators
//			    (e->getParent()->rtti()==RS2::EntitySpline ||
//				 e->getParent()->rtti()==RS2::EntityPolyline)) {
                (e->getParent()->rtti()==RS2::EntitySpline)) {
                e->getParent()->setSelected(false);
            }
            RS_Entity* cp = e->clone();
            cp->setPen(RS_Pen(RS2::FlagInvalid));
            cp->reparent(loop);
            loop->addEntity(cp);
        }
    }

    hatch->addEntity(loop);
	if (hatch->validate()) {
		container->addEntity(hatch.get());

		if (document) {
			document->startUndoCycle();
			document->addUndoable(hatch.get());
			document->endUndoCycle();
		}
		hatch->update();

		graphicView->redraw(RS2::RedrawDrawing);

		bool printArea = true;
        switch( hatch->getUpdateError()) {
        case RS_Hatch::HATCH_OK :
            RS_DIALOGFACTORY->commandMessage(tr("Hatch created successfully."));
            break;
        case RS_Hatch::HATCH_INVALID_CONTOUR :
            RS_DIALOGFACTORY->commandMessage(tr("Hatch Error: Invalid contour found!"));
			printArea = false;
            break;
        case RS_Hatch::HATCH_PATTERN_NOT_FOUND :
            RS_DIALOGFACTORY->commandMessage(tr("Hatch Error: Pattern not found!"));
            break;
        case RS_Hatch::HATCH_TOO_SMALL :
            RS_DIALOGFACTORY->commandMessage(tr("Hatch Error: Contour or pattern too small!"));
            break;
        case RS_Hatch::HATCH_AREA_TOO_BIG :
            RS_DIALOGFACTORY->commandMessage(tr("Hatch Error: Contour too big!"));
            break;
        default :
            RS_DIALOGFACTORY->commandMessage(tr("Hatch Error: Undefined Error!"));
			printArea = false;
            break;
        }
		if(m_bShowArea && printArea){
            RS_DIALOGFACTORY->commandMessage(tr("Total hatch area = %1").
                                             arg(hatch->getTotalArea(),10,'g',8));
        }

		hatch.release();

	} else {
		hatch.reset();
		RS_DIALOGFACTORY->commandMessage(tr("Invalid hatch area. Please check that "
		"the entities chosen form one or more closed contours."));
	}
    //}
}



void RS_ActionDrawHatch::mouseMoveEvent(QMouseEvent*) {
    RS_DEBUG->print("RS_ActionDrawHatch::mouseMoveEvent begin");

    /*if (getStatus()==SetPos) {
        RS_Vector mouse = snapPoint(e);
        pos = mouse;


        deletePreview();
		if (hatch && !hatch->isVisible()) {
            hatch->setVisible(true);
        }
        offset = RS_Vector(graphicView->toGuiDX(pos.x),
                           -graphicView->toGuiDY(pos.y));
        drawPreview();
}*/

    RS_DEBUG->print("RS_ActionDrawHatch::mouseMoveEvent end");
}



void RS_ActionDrawHatch::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
		snapPoint(e);

        switch (getStatus()) {
        case ShowDialog:
            break;

        default:
            break;
        }
    } else if (e->button()==Qt::RightButton) {
        //deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionDrawHatch::updateMouseButtonHints() {
	RS_DIALOGFACTORY->updateMouseWidget();
}

void RS_ActionDrawHatch::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::SelectCursor);
}

// EOF
