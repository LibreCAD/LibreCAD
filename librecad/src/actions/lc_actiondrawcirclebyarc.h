#ifndef LC_ACTIONDRAWCIRCLEBYARC_H
#define LC_ACTIONDRAWCIRCLEBYARC_H

#include "lc_abstractactiondrawrectangle.h"
#include "rs_circle.h"

/**
 * Action draws circle with the same center and radius as selected arc.
 * Based on setting, original arc may remains in drawing (so both arc and circle will be present) or just be replaced by circle
 */
class LC_ActionDrawCircleByArc:public RS_PreviewActionInterface {
    Q_OBJECT

public:
    LC_ActionDrawCircleByArc(RS_EntityContainer& container,RS_GraphicView& graphicView);
    ~LC_ActionDrawCircleByArc() override;

    enum{
        SetCircle
    };

    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void coordinateEvent(RS_CoordinateEvent *e) override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    bool isReplaceArcByCircle(){return replaceArcByCircle;};
    void setReplaceArcByCircle(bool value);
protected:

    void createOptionsWidget() override;

private:
    /** Chosen entity */
    RS_Arc *arc = nullptr;

    //list of entity types supported by current action
    const EntityTypeList circleType = EntityTypeList{RS2::EntityArc};

    RS_CircleData createCircleData();
    /**
     * controls whether original arc should be deleted or not
     */
    bool replaceArcByCircle;
    void deleteOriginalEntity(RS_Entity *entity);
};

#endif // LC_ACTIONDRAWCIRCLEBYARC_H
