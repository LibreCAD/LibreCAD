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

#include "rs_arc.h"
#include "rs_modification.h"
#include "rs_math.h"
#include "rs_dialogfactory.h"
#include "rs_actionmodifyrotate.h"
#include <QApplication>
#include <QMouseEvent>
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_point.h"
#include "rs_line.h"
#include "rs_previewactioninterface.h"
#include "rs_coordinateevent.h"

/**
 * Constructor.
 *
 * Sets the entity container on which the action class inherited
 * from this interface operates.
 */
RS_PreviewActionInterface::RS_PreviewActionInterface(const char* name,
                                                     RS_EntityContainer& container,
                                                     RS_GraphicView& graphicView,
                                                     RS2::ActionType actionType) :
    RS_ActionInterface(name, container, graphicView, actionType)
  ,preview(std::make_unique<RS_Preview>(&container)), highlight(std::make_unique<LC_Highlight>())
{

    RS_DEBUG->print("RS_PreviewActionInterface::RS_PreviewActionInterface: Setting up action with preview: \"%s\"", name);

    // preview is linked to the container for getting access to
    //   document settings / dictionary variables

    preview->setLayer(nullptr);

    RS_DEBUG->print("RS_PreviewActionInterface::RS_PreviewActionInterface: Setting up action with preview: \"%s\": OK", name);
}

/** Destructor */
RS_PreviewActionInterface::~RS_PreviewActionInterface() {
    deletePreview();
    deleteHighlights();
}


void RS_PreviewActionInterface::init(int status) {
    deletePreview();
    deleteHighlights();
    RS_ActionInterface::init(status);
}

void RS_PreviewActionInterface::finish(bool updateTB) {
    deletePreview();
    deleteHighlights();
    RS_ActionInterface::finish(updateTB);
}

void RS_PreviewActionInterface::suspend() {
    RS_ActionInterface::suspend();
    deletePreview();
    deleteHighlights();
}



void RS_PreviewActionInterface::resume() {
    RS_ActionInterface::resume();
    drawPreview();
    drawHighlights();
}



void RS_PreviewActionInterface::trigger() {
    RS_ActionInterface::trigger();
    deletePreview();
    deleteHighlights();
}


/**
 * Deletes the preview from the screen.
 */
void RS_PreviewActionInterface::deletePreview() {
		if (hasPreview){
                //avoid deleting NULL or empty preview
            preview->clear();
            hasPreview=false;
        }
	if(!graphicView->isCleanUp()){
		graphicView->getOverlayContainer(RS2::ActionPreviewEntity)->clear();
	}
}


/**
 * Draws / deletes the current preview.
 */
void RS_PreviewActionInterface::drawPreview() {
	// RVT_PORT How does offset work??        painter->setOffset(offset);
	RS_EntityContainer *container=graphicView->getOverlayContainer(RS2::ActionPreviewEntity);
	container->clear();
	container->setOwner(false); // Little hack for now so we don't delete the preview twice
	container->addEntity(preview.get());
	graphicView->redraw(RS2::RedrawOverlay);
	hasPreview=true;
}

void RS_PreviewActionInterface::deleteHighlights(){
    highlight->clear();
    if(!graphicView->isCleanUp()){
        graphicView->getOverlayContainer(RS2::OverlayEffects)->clear();
    }
}

void RS_PreviewActionInterface::drawHighlights(){
    RS_EntityContainer *container=graphicView->getOverlayContainer(RS2::OverlayEffects);
    container->clear();
    container->setOwner(false); // Little hack for now so we don't delete the preview twice
    highlight->addEntitiesToContainer(container);
//    container->addEntity(highlight.get());
//    graphicView->redraw(RS2::RedrawDrawing);
    graphicView->redraw(RS2::RedrawOverlay);
}

void RS_PreviewActionInterface::addToHighlights(RS_Entity *e, bool enable){
    if (enable){
        highlight->addEntity(e);
    }
    else{
        highlight ->removeEntity(e);
    }
}

void RS_PreviewActionInterface::addReferencePointToPreview(const RS_Vector &coord){
    auto *point = new RS_Point(this->preview.get(), coord);
    this->preview->addEntity(point);
}

void RS_PreviewActionInterface::addPointToPreview(const RS_Vector &coord){
    auto *point = new RS_Point(this->preview.get(), coord);
    this->preview->addEntity(point);
}

void RS_PreviewActionInterface::addReferenceLineToPreview(const RS_Vector &start, const RS_Vector &end){
    auto *line = new RS_Line(this->preview.get(), start, end);
    this->preview->addEntity(line);
}

void RS_PreviewActionInterface::addLineToPreview(const RS_Vector &start, const RS_Vector &end){
    auto *line = new RS_Line(this->preview.get(), start, end);
    this->preview->addEntity(line);
}

bool RS_PreviewActionInterface::trySnapToRelZeroCoordinateEvent(const QMouseEvent *e){
    bool result = false;
    bool shiftPressed = e->modifiers() & Qt::ShiftModifier;
    if (shiftPressed){
        RS_Vector relZero = graphicView->getRelativeZero();
        if (relZero.valid){
            RS_CoordinateEvent ce(relZero);
            coordinateEvent(&ce);
            result = true;
        }
    }
    return result;
}

RS_Vector RS_PreviewActionInterface::getSnapAngleAwarePoint(const QMouseEvent *e, const RS_Vector& basepoint, const RS_Vector& pos){
    RS_Vector result = pos;
    bool shiftPressed = e->modifiers() & Qt::ShiftModifier;
    if (shiftPressed){
        result = snapToAngle(pos, basepoint);
    }
    return result;
}

RS_Vector RS_PreviewActionInterface::getRelZeroAwarePoint(const QMouseEvent *e, const RS_Vector& pos){
    RS_Vector result = pos;
    bool shiftPressed = e->modifiers() & Qt::ShiftModifier;
    if (shiftPressed){
        RS_Vector relZero = graphicView->getRelativeZero();
        if (relZero.valid){
           result = relZero;
        }
    }
    return result;
}

void RS_PreviewActionInterface::addReferenceArcToPreview(const RS_Vector &center, const RS_Vector &startPoint, const RS_Vector &mouse, bool determineReversal){
    double radius = center.distanceTo(startPoint);
    double angle1 = center.angleTo(mouse);
    double angle2 = center.angleTo(startPoint);
    bool reversed = determineReversal ? RS_Math::getAngleDifference(angle2, angle1) < M_PI : true;
    auto arc = new RS_Arc(preview.get(), RS_ArcData(center, radius, angle1, angle2, reversed));
    preview->addEntity(arc);
}