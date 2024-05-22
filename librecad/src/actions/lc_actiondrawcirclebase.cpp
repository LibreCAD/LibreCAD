#include "rs_coordinateevent.h"
#include "rs_graphicview.h"
#include "lc_actiondrawcirclebase.h"

LC_ActionDrawCircleBase::LC_ActionDrawCircleBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView)
    :RS_PreviewActionInterface(name,container, graphicView){}

LC_ActionDrawCircleBase::~LC_ActionDrawCircleBase() = default;

void LC_ActionDrawCircleBase::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button()==Qt::LeftButton) {
        RS_CoordinateEvent ce(snapPoint(e));
        coordinateEvent(&ce);
    } else if (e->button()==Qt::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}

void LC_ActionDrawCircleBase::init(int status) {
    RS_PreviewActionInterface::init(status);
    reset(); // fixme - review implmenetation in inherited actions
    drawCirclePointsOnPreview = true; // fixme - read from options
    moveRelPointAtCenterAfterTrigger = true; // fixme - read from options
}

// fixme - resume method - re-read from options

void LC_ActionDrawCircleBase::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}

void LC_ActionDrawCircleBase::reset(){

}
