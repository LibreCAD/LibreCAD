#ifndef LC_ACTIONMODIFYDUPLICATE_H
#define LC_ACTIONMODIFYDUPLICATE_H

#include "lc_abstractactionwithpreview.h"

class LC_ActionModifyDuplicate:public LC_AbstractActionWithPreview{
    Q_OBJECT
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    void updateMouseButtonHints() override;

public:
    enum{
        SelectEntity
    };

    LC_ActionModifyDuplicate(RS_EntityContainer& container,RS_GraphicView& graphicView);
    ~LC_ActionModifyDuplicate() override;
    void init(int status) override;

    double getOffsetX() {return offsetX;};
    double getOffsetY() {return offsetY;};

    void setOffsetX(double value) {offsetX = value;};
    void setOffsetY(double value){offsetY = value;};

protected:
    RS2::CursorType doGetMouseCursor(int status) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doAfterTrigger() override;
    void createOptionsWidget() override;
private:

    QList<RS_Entity *> selectedEntities;
    int offsetX;
    int offsetY;
    RS_Vector getOffset() const;
};

#endif // LC_ACTIONMODIFYDUPLICATE_H
