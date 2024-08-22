#include "lc_actiondrawellipse1point.h"
#include "ellipse/lc_ellipse1pointoptions.h"

struct LC_ActionDrawEllipse1Point::Points {
/** Center of ellipse */
    RS_Vector center = RS_Vector(false);
    double  majorRadius = 1.0;
    double minorRadius = 1.0;
    bool hasAngle = false;
    bool freeAngle = false;
    double majorRadiusAngle = 0.0;
    double angle1 = 0.;
    double angle2 = 0.;
    bool isArc = false;
};

LC_ActionDrawEllipse1Point::LC_ActionDrawEllipse1Point(RS_EntityContainer &container, RS_GraphicView &graphicView,   bool isArc)
    :LC_ActionDrawCircleBase("Draw ellipse by 1 point", container, graphicView) {
    actionType = RS2::ActionDrawEllipse1Point; // fixme - support arc
}

LC_ActionDrawEllipse1Point::~LC_ActionDrawEllipse1Point() = default;

void LC_ActionDrawEllipse1Point::trigger() {

}

void LC_ActionDrawEllipse1Point::mouseMoveEvent(QMouseEvent *e) {
    int status = getStatus();
    RS_Vector mouse = snapPoint(e);
    switch (status){
        case SetPoint:{
            if (!trySnapToRelZeroCoordinateEvent(e)){
                RS_Vector majorP = pPoints->center;
                previewEllipse({pPoints->center, mouse - pPoints->center, 0.5, 0.0,
                                pPoints->isArc ? 2. * M_PI : 0., false});
            }
            break;
        }
        case SetMajorAngle: {
            mouse = getSnapAngleAwarePoint(e, pPoints->center, mouse, true);
            break;
        }
        case SetAngle1: {
            break;
        }
        case SetAngle2: {
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawEllipse1Point::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status){
        case SetPoint:{
            break;
        }
        case SetMajorAngle: {
            break;
        }
        case SetAngle1: {
            break;
        }
        case SetAngle2: {
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawEllipse1Point::onMouseRightButtonRelease(int status, QMouseEvent *e) {
    initPrevious(status);
}

void LC_ActionDrawEllipse1Point::onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) {
    switch (status){
        case SetPoint:{
            break;
        }
        case SetMajorAngle: {
            break;
        }
        case SetAngle1: {
            break;
        }
        case SetAngle2: {
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawEllipse1Point::updateMouseButtonHints() {
    int status = getStatus();
    switch (status){
        case SetPoint:{
            break;
        }
        case SetMajorAngle: {
            break;
        }
        case SetAngle1: {
            break;
        }
        case SetAngle2: {
            break;
        }
        default:
            break;
    }
}

bool LC_ActionDrawEllipse1Point::doProcessCommand(int status, const QString &command) {
    return false;
}

double LC_ActionDrawEllipse1Point::getMajorRadius() {
    return pPoints->majorRadius;
}

double LC_ActionDrawEllipse1Point::getMinorRadius() {
    return pPoints->minorRadius;
}

double LC_ActionDrawEllipse1Point::getAngle() {
    return pPoints->majorRadiusAngle;
}

bool LC_ActionDrawEllipse1Point::hasAngle() {
    return pPoints->hasAngle;
}

bool LC_ActionDrawEllipse1Point::isAngleFree() {
    return pPoints->freeAngle;
}

void LC_ActionDrawEllipse1Point::setMajorRadius(double val) {
    pPoints->majorRadius = val;
}

void LC_ActionDrawEllipse1Point::setMinorRadius(double val) {
    pPoints->minorRadius = val;
}

void LC_ActionDrawEllipse1Point::setAngle(double val) {
    pPoints->majorRadiusAngle = val;
}

void LC_ActionDrawEllipse1Point::setHasAngle(bool val) {
    pPoints->hasAngle = val;
}

void LC_ActionDrawEllipse1Point::setAngleFree(bool val) {
    pPoints->freeAngle = val;
}

LC_ActionOptionsWidget *LC_ActionDrawEllipse1Point::createOptionsWidget() {
    return new LC_Ellipse1PointOptions();
}
