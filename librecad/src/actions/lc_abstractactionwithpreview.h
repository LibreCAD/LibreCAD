#ifndef LC_ABSTRACTACTIONWITHPREVIEW_H
#define LC_ABSTRACTACTIONWITHPREVIEW_H

#include "rs_line.h"
#include "rs_polyline.h"
#include "rs_vector.h"
#include "rs_previewactioninterface.h"

class LC_AbstractActionWithPreview :public RS_PreviewActionInterface{
   Q_OBJECT

public:

    LC_AbstractActionWithPreview(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView);
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void commandEvent(RS_CommandEvent *event) override;
    void finish(bool updateTB) override;
    void coordinateEvent(RS_CoordinateEvent *event) override;
    void trigger() override;
    void updateMouseCursor() override;
protected:
    /**
     * Entity that is highlighted as part of mouse selection operation (if any)
     */
    RS_Entity* highlightedEntity;
    /**
     * Last point of snap during mouse move
     */
    RS_Vector lastSnapPoint;

    // functions starting with "do" prefix are the most probable candidates for overriding in inherited actions

    virtual bool doCheckMayDrawPreview(QMouseEvent *event, int status);
    virtual bool doProcessCommand(RS_CommandEvent *e, const QString &c);
    virtual void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed);
    virtual RS_Vector doGetMouseSnapPoint(QMouseEvent *e, bool shiftPressed);
    virtual void doFinish(bool updateTB);
    virtual void doBack(QMouseEvent *pEvent, int status);
    virtual bool onMouseMove(QMouseEvent *e, RS_Vector snap, int status);
    virtual void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status);
    virtual void onCoordinateEvent(const RS_Vector &coord, bool isZero, int status);
    virtual bool doCheckMayTrigger();
    virtual void doAfterTrigger();
    virtual void doPrepareTriggerEntities(QList<RS_Entity *> &list);
    virtual RS2::CursorType doGetMouseCursor(int status);
    virtual RS_Vector doGetRelativeZeroAfterTrigger();
    virtual int doRelZeroInitialSnapState();
    virtual void doRelZeroInitialSnap(RS_Vector vector);
    virtual void doMouseMoveEnd(int status, QMouseEvent *e);
    virtual void doMouseMoveStart(int status, QMouseEvent *pEvent, bool shiftPressed);

    // additional setup methods, that may be overridden if necessary
    virtual bool isSetActivePenAndLayerOnTrigger();
    virtual void onRightMouseButtonRelease(QMouseEvent *e, int status);
    virtual bool isUndoableTrigger();
    virtual void performTrigger();
    virtual void performTriggerInsertions();
    virtual void performTriggerDeletions();
    virtual void finishAction();
    // utility functions
    virtual void drawPreviewForPoint(QMouseEvent *e, RS_Vector& snap);
    void unHighlightEntity();
    void highlightEntity(RS_Entity *en);
    void drawPreviewForLastPoint();
    void deleteEntityUndoable(RS_Entity *entity);
    void updateMouseWidgetTR(const char* left,const char* right);
    void commandMessageTR(const char*);
    void updateMouseWidget(const QString& = QString(),
                           const QString& = QString());
    void commandMessage(QString msg) const;
    void highlightEntityExplicit(RS_Entity *en, bool highlight);
    void checkPreSnapToRelativeZero(int status, QMouseEvent *pEvent, bool shiftPressed);

};

#endif // LC_ABSTRACTACTIONWITHPREVIEW_H
