#include "rs_ellipse.h"
#include <QMouseEvent>
#include "rs_graphicview.h"
#include "lc_actiondrawcirclebase.h"

LC_ActionDrawCircleBase::LC_ActionDrawCircleBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface(name,container, graphicView){}

LC_ActionDrawCircleBase::~LC_ActionDrawCircleBase() = default;

void LC_ActionDrawCircleBase::mouseLeftButtonReleaseEvent([[maybe_unused]]int status, QMouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void LC_ActionDrawCircleBase::mouseRightButtonReleaseEvent(int status, [[maybe_unused]]QMouseEvent *e) {
    deletePreview();
    init(status-1);
}

void LC_ActionDrawCircleBase::init(int status) {
    RS_PreviewActionInterface::init(status);
    reset(); // fixme - review implmenetation in inherited actions
    moveRelPointAtCenterAfterTrigger = true; // fixme - read from options
}

// fixme - resume method - re-read from options

 RS2::CursorType LC_ActionDrawCircleBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}

void LC_ActionDrawCircleBase::reset(){

}

void LC_ActionDrawCircleBase::previewEllipseReferencePoints(const RS_Ellipse *ellipse, bool drawAxises, RS_Vector mouse){
    RS_Vector center = ellipse->getCenter();
    RS_Vector majorP = ellipse->getMajorP();
    const RS_Vector &major1 = center - majorP;
    const RS_Vector &major2 = center + majorP;
    const RS_Vector &minor1 = ellipse->getMinorPoint();
    const RS_Vector &minor2 = center - RS_Vector(-majorP.y, majorP.x) * ellipse->getRatio();

    previewRefSelectablePoint(minor1);
    previewRefSelectablePoint(minor2);
    previewRefPoint(center);

    if (drawAxises){
        if (mouse.valid){
            RS_Vector minor;
            if (minor1.distanceTo(mouse) < minor2.distanceTo(mouse)){
                minor = minor1;
            }
            else{
                minor = minor2;
            }

            previewRefPoint(major1);
            previewRefPoint(major2);

            previewRefLine(center, minor);
        }
        else {
            previewRefLine(major1, major2);
            previewRefLine(minor1, minor2);
            previewRefSelectablePoint(major1);
            previewRefSelectablePoint(major2);
        }

    }
    else{
        previewRefSelectablePoint(major1);
        previewRefSelectablePoint(major2);
    }

}
