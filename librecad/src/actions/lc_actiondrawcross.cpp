/****************************************************************************
**
* Action that draws a cross in the center of selected arc, circle,
* ellipse or ellipse arc

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

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

#include <cmath>
#include "rs_line.h"
#include "rs_ellipse.h"
#include "rs_math.h"
#include "lc_actiondrawcross.h"
#include "lc_crossoptions.h"

// todo - it is possible to duplicate UI by commands, but it seems that's not too practical
// todo - the action should be initiated by mouse anyway, so in order to make the action fully scriptable,
// todo - it is necessary either have command for entity selection or skip commands for now

/**
 * Simple action that draws a cross located in the center of selected circle, arc, ellipse or ellipse arc.
 * Size and angle of cross is controlled by options.
 * @param container
 * @param graphicView
 */
LC_ActionDrawCross::LC_ActionDrawCross(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview("Draw Cross", container, graphicView), entity(nullptr){
    actionType = RS2::ActionDrawCross;
}

LC_ActionDrawCross::~LC_ActionDrawCross() = default;

// support of creation cross for already selected entities on action invocation

bool LC_ActionDrawCross::doCheckMayTriggerOnInit(int status){
    return status == SetEntity;
}

bool LC_ActionDrawCross::isAcceptSelectedEntityToTriggerOnInit(RS_Entity *pEntity){
    // from all selected entities, we'll accept only ones that suits for the cross creation
    int rtti = pEntity->rtti();
    return rtti == RS2::EntityCircle || rtti == RS2::EntityEllipse || rtti == RS2::EntityArc;
}

void LC_ActionDrawCross::doCreateEntitiesOnTrigger(RS_Entity *en, QList<RS_Entity *> &list){
    LC_CrossData crossData = createCrossDataForEntity(en);
    addCrossDataEntities(list, crossData);
}

/**
 * Creates entities (2 lines) that will added as result of trigger to the drawing
 * @param list
 */
void LC_ActionDrawCross::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    doCreateEntitiesOnTrigger(entity, list);
}

/**
 * Creates line entities based on cross data and adds them to the list
 * @param list
 * @param crossData
 */
void LC_ActionDrawCross::addCrossDataEntities(QList<RS_Entity *> &list, const LC_CrossData &crossData) const{
    // create horizontal and vertical lines

    RS_LineData horizontalData = crossData.horizontal;
    RS_Entity *horizontalLine = new RS_Line(container, horizontalData);

    list << horizontalLine;

    RS_LineData verticalData = crossData.vertical;
    RS_Entity *verticalLine = new RS_Line(container, verticalData);

    list << verticalLine;
}

bool LC_ActionDrawCross::doCheckMayTrigger(){
    return entity != nullptr; // we need entity selected
}

RS_Vector LC_ActionDrawCross::doGetRelativeZeroAfterTrigger(){
    return entity->getCenter(); // just put in center of circle
}

void LC_ActionDrawCross::doAfterTrigger(){
    entity = nullptr; // cleanup
}

/**
 * Major function that calculates sizes for cross lines based on given options and selected entity properties
 * @return data for cross creation
 */
LC_CrossData LC_ActionDrawCross::createCrossDataForEntity(RS_Entity* ent) const{
    RS_Vector cp = ent->getCenter();
    double lengthX=0., lengthY=0.;
    double ellipseangle = 0.0;
    RS2::EntityType rtti = ent->rtti();
    bool arcShape = rtti == RS2::EntityArc;
    bool isCircle = rtti == RS2::EntityCircle;
    bool isEllipse = rtti == RS2::EntityEllipse;
    bool isEllipseArcShape = false;

    // check whether we are in ellipse arc
    RS_Ellipse* ellipse = nullptr;
    if (isEllipse){
        ellipse = dynamic_cast<RS_Ellipse *>(ent);
        isEllipseArcShape = ellipse->isEllipticArc();
    }

    double lenYToUse = lenY;

    if (std::abs(lenY) < RS_TOLERANCE){
        lenYToUse = lenX;
    }

    // first, determine size of cross based on specified mode
    switch (crossSizeMode) {
        case CROSS_SIZE_EXTEND: // cross should extend shape on specified length
            if (arcShape || isCircle){
                // for arc and circle we rely on radius
                double radius = ent->getRadius();
                lengthX = radius + lenX;
                lengthY = radius + lenYToUse;
                ellipseangle = 0.0; // we'll draw for circle, so no ellipse angle there

            } else if (ellipse != nullptr) {
                // for ellipses - we rely on axis radiuses
                lengthX = ellipse->getMajorRadius() + lenX;
                lengthY = ellipse->getMinorRadius() + lenYToUse;
                ellipseangle = ellipse->getAngle();
            }
            else{
                // should not be possible unless new entities are added
                lengthX = -1;
                lengthY = -1;
            }
            break;
        case CROSS_SIZE_LENGTH:  // cross should have fixed length
            // divide length by 2 because + operator on vector
            // adds the length to both ends of the line.
            lengthX = lenX / 2;
            lengthY = lenYToUse / 2;
            if (isEllipse || isEllipseArcShape){
                ellipseangle = ellipse->getAngle();
            } else {
                ellipseangle = 0.0;
            }
            break;
        case CROSS_SIZE_PERCENT:  //Length is value in percents of radius
            if (arcShape || isCircle){ // for circle, it will be percent of radius
                double radius = ent->getRadius();
                lengthX = radius * lenX / 100.0;
                lengthY = radius * lenYToUse / 100.0;
                ellipseangle = 0.0;
            } else if (isEllipse || isEllipseArcShape){  // for ellipse, calculate percents for each axis
                lengthX = ellipse->getMajorRadius() * lenX / 100.0;
                lengthY = ellipse->getMinorRadius() * lenYToUse / 100.0;
                ellipseangle = ellipse->getAngle();
            }
            else{
                // should not be possible unless new entities are added
                lengthX = -1;
                lengthY = -1;
            }
            break;
    }

    RS_Vector v = RS_Vector();

    // points for lines
    RS_Vector horStart;
    RS_Vector horEnd;
    RS_Vector vertStart;
    RS_Vector vertEnd;

    // convert angle in degrees to radians
    double orientationAngle = RS_Math::deg2rad(angle);

    // determine start and end points for cross lines based on calculated lengths and angle

    // calculate horizontal line
    if (lengthX <= 0.0){ // check for zero
        horStart = cp;
        horEnd = cp;
    } else {
        // normal line - taking care of given angle (plus, take care of ellipse angle, if needed)
        // calculate start point
        v.setPolar(lengthX, orientationAngle + M_PI + ellipseangle);
        horStart = cp + v;
        // end point of horizontal line
        v.setPolar(lengthX, orientationAngle + ellipseangle);
        horEnd = cp + v;
    }

    // calculate vertical line
    if (lengthY <= 0.0){
        vertStart = cp;
        vertEnd = cp;
    } else {
        v.setPolar(lengthY, orientationAngle - M_PI / 2 + ellipseangle);
        vertStart = cp + v;
        v.setPolar(lengthY, orientationAngle + M_PI / 2 + ellipseangle);
        vertEnd = cp + v;
    }
    // return result
    LC_CrossData result = LC_CrossData(horStart, horEnd, vertStart, vertEnd, cp);
    return result;
}


bool LC_ActionDrawCross::doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, int status){
    return status == SetEntity;
}

/**
 * creates entities to show in preview
 * @param e
 * @param snap
 * @param list
 * @param status
 */
void LC_ActionDrawCross::doPreparePreviewEntities(QMouseEvent *e, [[maybe_unused]]RS_Vector &snap, QList<RS_Entity *> &list,[[maybe_unused]] int status){
    RS_Entity *en = catchEntity(e, circleType, RS2::ResolveAll);
    // check whether entity is ok for drawing cross
    if (en != nullptr){
        bool isArc = en->isArc();
        if (!isArc){
            if (en->is(RS2::EntityEllipse)){
                // for ellipse arcs
                isArc = true;
            }
        }
        if (isArc){ // can draw
            // handle visual highlighting
            entity = en;

            highlightHover(en);

            // prepare data for preview
            LC_CrossData crossData = createCrossDataForEntity(en);
            // create lines
            addCrossDataEntities(list, crossData);
            if (showRefEntitiesOnPreview) {
                // ref point
                createRefPoint(crossData.centerPoint, list);
            }
        }
    }
}

/**
 * Just invoke trigger on left mouse click
 * @param e
 * @param status
 * @param snapPoint
 */
void LC_ActionDrawCross::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, int status, [[maybe_unused]]const RS_Vector &snapPoint){
    if (status == SetEntity){
        trigger(); // just draw cross on click
    }
}

void LC_ActionDrawCross::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select circle, arc or ellipse"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

/**
 * Returns mouse cursor for action
 * @param status
 * @return
 */
RS2::CursorType LC_ActionDrawCross::doGetMouseCursor([[maybe_unused]]int status){
  return RS2::SelectCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawCross::createOptionsWidget(){
    return new LC_CrossOptions();
}
