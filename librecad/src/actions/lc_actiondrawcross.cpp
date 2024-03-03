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
    :RS_PreviewActionInterface("Draw Cross", container, graphicView)
    , point(new RS_Vector{}), circle(nullptr){
    actionType = RS2::ActionDrawCross;
}

LC_ActionDrawCross::~LC_ActionDrawCross() = default;

void LC_ActionDrawCross::trigger(){
    RS_PreviewActionInterface::trigger();

    if (circle){
        LC_CrossData crossData = createCrossData();
        RS_LineData horizontalData = crossData.horizontal;
        RS_Entity *horizontalLine = new RS_Line(container, horizontalData);
        RS_LineData verticalData = crossData.vertical;
        RS_Entity *verticalLine = new RS_Line(container, verticalData);

        circle->setHighlighted(false);
        graphicView->drawEntity(circle);

        horizontalLine->setLayerToActive();
        horizontalLine->setPenToActive();
        container->addEntity(horizontalLine);

        verticalLine->setLayerToActive();
        verticalLine->setPenToActive();
        container->addEntity(verticalLine);

// upd. undo list:
        if (document){
            document->startUndoCycle();
            document->addUndoable(horizontalLine);
            document->addUndoable(verticalLine);
            document->endUndoCycle();
        }

        graphicView->redraw(RS2::RedrawDrawing);

        // fixme - relative point to center??

        setStatus(SetCircle);

    } else {
        RS_DEBUG->print("RS_ActionDrawCroww::trigger:"
                        " Circle is nullptr\n");
    }
}

void LC_ActionDrawCross::showOptions(){
    RS_ActionInterface::showOptions();
}
//    RS_ActionInterface::showOptions();
//    RS_DIALOGFACTORY->requestOptions (this, true);
//}
//
//void LC_ActionDrawCross::hideOptions(){
//    RS_ActionInterface::hideOptions();
//    RS_DIALOGFACTORY->requestOptions (this, false);
//}

LC_CrossData LC_ActionDrawCross::createCrossData(){
    double lengthX, lengthY;
    double ellipseangle = 0.0;
    RS_Vector cp = circle->getCenter();
    RS2::EntityType circleRtti = circle->rtti();
    bool arcShape = circleRtti == RS2::EntityArc;
    bool isCircle = circleRtti == RS2::EntityCircle;
    bool isEllipseShape = circleRtti == RS2::EntityEllipse;
    bool isEllipseArcShape = false;
    double radius = circle->getRadius();
    double lenYToUse = lenY;

    if (std::abs(lenY) < RS_TOLERANCE){
        lenYToUse = lenX;
    }

    switch (crossMode) {
        case 0: //Extend
            if (arcShape || isCircle){
                lengthX = radius + lenX;
                lengthY = radius + lenYToUse;
                ellipseangle = 0.0;
            } else if (isEllipseShape || isEllipseArcShape){
                RS_Ellipse *ellipse = static_cast<RS_Ellipse *>(circle);
                lengthX = ellipse->getMajorRadius() + lenX;
                lengthY = ellipse->getMinorRadius() + lenYToUse;
                ellipseangle = ellipse->getAngle();
            }
            break;
        case 1:    //Length
            // divide length by 2 because + operator on vector
            // adds the length to both ends of the line.
            lengthX = lenX / 2;
            lengthY = lenYToUse / 2;
            if (isEllipseShape || isEllipseArcShape){
                RS_Ellipse *ellipse = static_cast<RS_Ellipse *>(circle);
                ellipseangle = ellipse->getAngle();
            } else {
                ellipseangle = 0.0;
            }
            break;
        case 2:  //Percent
            if (arcShape || isCircle){
                lengthX = radius * lenX / 100.0;
                lengthY = radius * lenYToUse / 100.0;
                ellipseangle = 0.0;
            } else if (isEllipseShape || isEllipseArcShape){
                RS_Ellipse *ellipse = static_cast<RS_Ellipse *>(circle);
                lengthX = ellipse->getMajorRadius() * lenX / 100.0;
                lengthY = ellipse->getMinorRadius() * lenYToUse / 100.0;
                ellipseangle = ellipse->getAngle();
            }
            break;
    }

    RS_Vector v = RS_Vector();
    RS_Vector point1;
    RS_Vector point2;
    RS_Vector point3;
    RS_Vector point4;

    // convert angle in degrees to radians
    double orientationAngle = RS_Math::deg2rad(orientation);
    if (lengthX <= 0.0){
        lengthX = 0.0;
        point1 = cp;
        point2 = cp;
    } else {
        v.setPolar(lengthX, orientationAngle + M_PI + ellipseangle);
        point1 = cp + v;
        v.setPolar(lengthX, orientationAngle + ellipseangle);
        point2 = cp + v;
    }
    if (lengthY <= 0.0){
        lengthY = 0.0;
        point3 = cp;
        point4 = cp;
    } else {
        v.setPolar(lengthY, orientationAngle - M_PI / 2 + ellipseangle);
        point3 = cp + v;
        v.setPolar(lengthY, orientationAngle + M_PI / 2 + ellipseangle);
        point4 = cp + v;
    }

    LC_CrossData result = LC_CrossData(point1, point2, point3, point4);
    return result;
}

void LC_ActionDrawCross::setXLength(double d){
    lenX = d;
}

void LC_ActionDrawCross::setYLength(double d){
    lenY = d;
}

void LC_ActionDrawCross::setCrossAngle(double d){
    orientation = d;
}

void LC_ActionDrawCross::setCrossMode(int i){
    crossMode = i;
}

void LC_ActionDrawCross::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("LC_ActionDrawCross::mouseMoveEvent begin");

    RS_Vector mouse(graphicView->toGraphX(e->x()),
                    graphicView->toGraphY(e->y()));

    switch (getStatus()) {
        case SetCircle: {
            RS_Entity* en = catchEntity(e, circleType, RS2::ResolveAll);
            if (en && (en->isArc() /*||
                       en->rtti()==RS2::EntitySplinePoints)*/)) {
                if(circle){
                    circle->setHighlighted(false);
                    graphicView->drawEntity(en);
                }
                circle = en;
                circle->setHighlighted(true);
                graphicView->drawEntity(en);

                deletePreview();

                LC_CrossData crossData = createCrossData();
                RS_LineData horizontalData = crossData.horizontal;
                RS_Entity *horizontalLine = new RS_Line(container, horizontalData);
                RS_LineData verticalData = crossData.vertical;
                RS_Entity *verticalLine = new RS_Line(container, verticalData);

                preview->addEntity(horizontalLine);
                preview->addEntity(verticalLine);

                drawPreview();
            }
            else{
                deletePreview();
                if(circle){
                    circle->setHighlighted(false);
                    graphicView->drawEntity(en);
                }

                graphicView->redraw(RS2::RedrawOverlay);
            }
        }
            break;

        default:
            break;
    }

    RS_DEBUG->print("LC_ActionDrawCross::mouseMoveEvent end");
}

void LC_ActionDrawCross::mouseReleaseEvent(QMouseEvent *e){
    if (e->button()==Qt::RightButton) {
        deletePreview();
        if(circle){
            circle->setHighlighted(false);
            graphicView->drawEntity(circle);
        }
        init(getStatus()-1);
    } else {
        switch (getStatus()) {
            case SetCircle:
              trigger();
              break;
        }
    }
}
// fixme - commands processing for duplication of UI options ??
void LC_ActionDrawCross::coordinateEvent(RS_CoordinateEvent *e){
//    RS_ActionInterface::coordinateEvent(e);
}

void LC_ActionDrawCross::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetCircle:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Select circle, arc or ellipse"),
                                                tr("Back"));
            break;
        default:
            RS_DIALOGFACTORY->updateMouseWidget();
            break;
    }
}

void LC_ActionDrawCross::updateMouseCursor(){
    switch (getStatus())    {
        case SetCircle:
            graphicView->setMouseCursor(RS2::SelectCursor);
            break;
        default:
            graphicView->setMouseCursor(RS2::CadCursor);
    }
}

void LC_ActionDrawCross::createOptionsWidget(){
    m_optionWidget = std::make_unique<LC_CrossOptions>(nullptr);
}
