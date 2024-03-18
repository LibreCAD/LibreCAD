//
// Created by sand1 on 17/02/2024.
//

#include <cmath>
#include "lc_actiondrawcross.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_document.h"
#include "rs_previewactioninterface.h"
#include "rs_entitycontainer.h"
#include "rs_debug.h"
#include "rs_preview.h"
#include "rs_ellipse.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"
#include "lc_crossoptions.h"

LC_ActionDrawCross::LC_ActionDrawCross(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview("Draw Cross", container, graphicView)
    , entity(nullptr){
    actionType = RS2::ActionDrawCross;
}

LC_ActionDrawCross::~LC_ActionDrawCross() = default;

void LC_ActionDrawCross::doPrepareTriggerEntities(QList<RS_Entity *> &list){
    // prepare data
    LC_CrossData crossData = createCrossData();

    // create lines
    RS_LineData horizontalData = crossData.horizontal;
    RS_Entity *horizontalLine = new RS_Line(container, horizontalData);

    list << horizontalLine;

    RS_LineData verticalData = crossData.vertical;
    RS_Entity *verticalLine = new RS_Line(container, verticalData);

    list << verticalLine;
}

bool LC_ActionDrawCross::doCheckMayTrigger(){
    return entity != nullptr;
}

RS_Vector LC_ActionDrawCross::doGetRelativeZeroAfterTrigger(){
    return entity->getCenter();
}

void LC_ActionDrawCross::doAfterTrigger(){
    entity = nullptr;
}

LC_CrossData LC_ActionDrawCross::createCrossData(){
    double lengthX, lengthY;
    double ellipseangle = 0.0;
    RS_Vector cp = entity->getCenter();
    RS2::EntityType circleRtti = entity->rtti();
    bool arcShape = circleRtti == RS2::EntityArc;
    bool isCircle = circleRtti == RS2::EntityCircle;
    bool isEllipseShape = circleRtti == RS2::EntityEllipse;
    bool isEllipseArcShape = false;
    double radius = entity->getRadius();
    double lenYToUse = lenY;

    if (std::abs(lenY) < RS_TOLERANCE){
        lenYToUse = lenX;
    }

    // first, determine size of cross based on specified mode
    switch (crossSizeMode) {
        case CROSS_SIZE_EXTEND:
            if (arcShape || isCircle){
                // for arc and circle we rely on radius
                lengthX = radius + lenX;
                lengthY = radius + lenYToUse;
                ellipseangle = 0.0;
            } else if (isEllipseShape || isEllipseArcShape){
                // for ellipses - we rely on axis radiuses
                RS_Ellipse *ellipse = static_cast<RS_Ellipse *>(entity);
                lengthX = ellipse->getMajorRadius() + lenX;
                lengthY = ellipse->getMinorRadius() + lenYToUse;
                ellipseangle = ellipse->getAngle();
            }
            break;
        case CROSS_SIZE_LENGTH:
            // divide length by 2 because + operator on vector
            // adds the length to both ends of the line.
            lengthX = lenX / 2;
            lengthY = lenYToUse / 2;
            if (isEllipseShape || isEllipseArcShape){
                RS_Ellipse *ellipse = static_cast<RS_Ellipse *>(entity);
                ellipseangle = ellipse->getAngle();
            } else {
                ellipseangle = 0.0;
            }
            break;
        case CROSS_SIZE_PERCENT:  //Length is value in percents of radius
            if (arcShape || isCircle){
                lengthX = radius * lenX / 100.0;
                lengthY = radius * lenYToUse / 100.0;
                ellipseangle = 0.0;
            } else if (isEllipseShape || isEllipseArcShape){
                RS_Ellipse *ellipse = static_cast<RS_Ellipse *>(entity);
                lengthX = ellipse->getMajorRadius() * lenX / 100.0;
                lengthY = ellipse->getMinorRadius() * lenYToUse / 100.0;
                ellipseangle = ellipse->getAngle();
            }
            break;
    }

    RS_Vector v = RS_Vector();
    RS_Vector horStart;
    RS_Vector horEnd;
    RS_Vector vertStart;
    RS_Vector vertEnd;

    // convert angle in degrees to radians
    double orientationAngle = RS_Math::deg2rad(angle);

    // determine start and end points for cross lines based on calculated lengths and angle

    // calculate horizontal line
    if (lengthX <= 0.0){
        lengthX = 0.0;
        horStart = cp;
        horEnd = cp;
    } else {
        v.setPolar(lengthX, orientationAngle + M_PI + ellipseangle);
        horStart = cp + v;
        v.setPolar(lengthX, orientationAngle + ellipseangle);
        horEnd = cp + v;
    }

    // calculcate vertical line
    if (lengthY <= 0.0){
        lengthY = 0.0;
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


// flag that controls whether target circle or arc should be highlighted during
// mouse selection. In general, it might be candidate to moving to options?
// todo - consider this later whether option is needed
#define HIGHLIGHT_TARGET_ENTITY_ON_MOVE false

void LC_ActionDrawCross::doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status){
    if (status == SetEntity){
        RS_Entity* en = catchEntity(e, circleType, RS2::ResolveAll);
        if (en && (en->isArc() /*||
                       en->rtti()==RS2::EntitySplinePoints)*/)) {
            // handle visual highlighting
            entity = en;
            if (HIGHLIGHT_TARGET_ENTITY_ON_MOVE){
                highlightEntity(en);
            }

            // prepare data fro preview
            LC_CrossData crossData = createCrossData();
            RS_LineData horizontalData = crossData.horizontal;
            RS_Entity *horizontalLine = new RS_Line(container, horizontalData);
            RS_LineData verticalData = crossData.vertical;
            RS_Entity *verticalLine = new RS_Line(container, verticalData);

            list << horizontalLine;
            list << verticalLine;
        }
    }
}

void LC_ActionDrawCross::doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint){
    if (status == SetEntity){
        trigger();
    }
}


void LC_ActionDrawCross::coordinateEvent(RS_CoordinateEvent *e){
//    RS_ActionInterface::coordinateEvent(e);
// todo - it is possible to duplicate UI by commands, but it seems that's not too practical
// todo - the action should be initiated by mouse anyway, so in order to make the action fully scriptable,
// todo - it is necessary either have command for entity selection or skip commands for now
}

void LC_ActionDrawCross::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTR("Select circle, arc or ellipse","Back");
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType LC_ActionDrawCross::doGetMouseCursor(int status){
    switch (status)    {
        case SetEntity:
            return RS2::SelectCursor;
        default:
            return RS2::CadCursor;
    }
}

void LC_ActionDrawCross::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_CrossOptions>(nullptr);
}


void LC_ActionDrawCross::setXLength(double d){
    lenX = d;
}

void LC_ActionDrawCross::setYLength(double d){
    lenY = d;
}

void LC_ActionDrawCross::setCrossAngle(double d){
    angle = d;
}

void LC_ActionDrawCross::setCrossMode(int i){
    crossSizeMode = i;
}
