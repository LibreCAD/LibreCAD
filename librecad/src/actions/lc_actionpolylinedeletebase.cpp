#include <QMouseEvent>
#include "rs_polyline.h"
#include "rs_modification.h"
#include "rs_graphicview.h"
#include "lc_actionpolylinedeletebase.h"

LC_ActionPolylineDeleteBase::LC_ActionPolylineDeleteBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView)
   :RS_PreviewActionInterface(name,container, graphicView){
}

void LC_ActionPolylineDeleteBase::getSelectedPolylineVertex(QMouseEvent *e, RS_Vector &vertex, RS_Entity *&segment){
    bool oldSnapOnEntity = snapMode.snapOnEntity;
    snapMode.snapOnEntity = true;
    RS_Vector snap = snapPoint(e);
    snapMode.snapOnEntity = oldSnapOnEntity;
    deletePreview();
    auto polyline = dynamic_cast<RS_Polyline *>(catchEntity(e, RS2::EntityPolyline));
    if (polyline == polylineToModify){
        RS_Vector coordinate = polyline->getNearestPointOnEntity(snap, true);
        segment = catchEntity(coordinate, RS2::ResolveAll);
        vertex  = segment->getNearestEndpoint(coordinate);
    }
    else{
        vertex = RS_Vector(false);
    }
}

void LC_ActionPolylineDeleteBase::mouseLeftButtonReleaseEvent([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
}

RS2::CursorType LC_ActionPolylineDeleteBase::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void LC_ActionPolylineDeleteBase::finish(bool updateTB){
    clean();
    RS_PreviewActionInterface::finish(updateTB);
}

void LC_ActionPolylineDeleteBase::clean(){
    if (polylineToModify){
        polylineToModify->setSelected(false);
    }
    deletePreview();
    graphicView->redraw();
}

void LC_ActionPolylineDeleteBase::mouseRightButtonReleaseEvent([[maybe_unused]]int status, [[maybe_unused]]QMouseEvent *e) {
    deleteSnapper();
    deletePreview();
    drawPreview();
    int newStatus = getStatus() - 1;
    if (newStatus == SetPolyline){
        clean();
    }
    setStatus(newStatus);
}
