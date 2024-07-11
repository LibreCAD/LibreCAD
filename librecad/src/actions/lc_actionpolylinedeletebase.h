#ifndef LC_ACTIONPOLYLINEDELETEBASE_H
#define LC_ACTIONPOLYLINEDELETEBASE_H

#include "rs_previewactioninterface.h"

class LC_ActionPolylineDeleteBase:public RS_PreviewActionInterface {
    Q_OBJECT

public:
    LC_ActionPolylineDeleteBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionPolylineDeleteBase() override = default;
protected:
    /**
   * Action States.
   */
    enum Status {
        SetPolyline,		/**< Choosing segment of existing polyline to delete between two nodes. */
        SetVertex1,    /**< Setting the node's point1. */
        SetVertex2     /**< Setting the node's point2. */
    };

    RS_Polyline *polylineToModify = nullptr;
    RS_Vector vertexToDelete = RS_Vector(false);

    void getSelectedPolylineVertex(QMouseEvent *e, RS_Vector &vertex, RS_Entity *&segment);
    void finish(bool updateTB) override;
    void clean();
    RS2::CursorType doGetMouseCursor(int status) override;
    void mouseLeftButtonReleaseEvent(int status, QMouseEvent *e) override;
    void mouseRightButtonReleaseEvent(int status, QMouseEvent *e) override;
};

#endif // LC_ACTIONPOLYLINEDELETEBASE_H
