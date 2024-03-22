#ifndef LC_ACTIONDRAWCIRCLEBYARC_H
#define LC_ACTIONDRAWCIRCLEBYARC_H

#include "lc_abstractactionwithpreview.h"
#include "rs_circle.h"

/**
 * Action draws circle with the same center and radius as selected arc.
 * Based on setting, original arc may remains in drawing (so both arc and circle will be present) or just be replaced by circle
 *
 */
 // fixme - add attributes selection
class LC_ActionDrawCircleByArc:public LC_AbstractActionWithPreview {
    Q_OBJECT

public:
    LC_ActionDrawCircleByArc(RS_EntityContainer& container,RS_GraphicView& graphicView);
    ~LC_ActionDrawCircleByArc() override;

    enum{
        SetArc
    };

    void coordinateEvent(RS_CoordinateEvent *e) override;
    void updateMouseButtonHints() override;

    bool isReplaceArcByCircle(){return replaceArcByCircle;};
    void setReplaceArcByCircle(bool value);
protected:

    void createOptionsWidget() override;
    bool doCheckMayTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doAfterTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    void performTriggerDeletions() override;

private:
    /** Chosen arc entity */
    RS_Arc *arc = nullptr;

    //list of entity types supported by current action
    const EntityTypeList circleType = EntityTypeList{RS2::EntityArc};

    RS_CircleData createCircleData();
    /**
     * controls whether original arc should be deleted or not
     */
    bool replaceArcByCircle;
};

#endif // LC_ACTIONDRAWCIRCLEBYARC_H
