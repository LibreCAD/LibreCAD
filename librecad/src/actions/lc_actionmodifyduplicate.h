#ifndef LC_ACTIONMODIFYDUPLICATE_H
#define LC_ACTIONMODIFYDUPLICATE_H

#include "lc_abstractactionwithpreview.h"

class LC_ActionModifyDuplicate:public LC_AbstractActionWithPreview{
    Q_OBJECT
public:
    enum{
        SelectEntity
    };

    enum {
        PEN_ACTIVE,
        PEN_ORIGINAL,
        PEN_ORIGINAL_RESOLVED
    };

    enum {
        LAYER_ACTIVE,
        LAYER_ORIGINAL
    };


    LC_ActionModifyDuplicate(RS_EntityContainer& container,RS_GraphicView& graphicView);
    ~LC_ActionModifyDuplicate() override;
    void init(int status) override;

    double getOffsetX() {return offsetX;};
    double getOffsetY() {return offsetY;};

    void setOffsetX(double value) {offsetX = value;};
    void setOffsetY(double value){offsetY = value;};

    bool isDuplicateInPlace(){return duplicateInplace;};
    void setDuplicateInPlace(bool value){duplicateInplace = value;};
    void updateMouseButtonHints() override;
    int getPenMode();
    void setPenMode(int value){penMode = value;};
    int getLayerMode(){return layerMode;};
    void setLayerMode(int value){layerMode = value;};
protected:
    RS2::CursorType doGetMouseCursor(int status) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doAfterTrigger() override;
    void createOptionsWidget() override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed) override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    bool isSetActivePenAndLayerOnTrigger() override;

private:

    QList<RS_Entity *> selectedEntities;
    int offsetX;
    int offsetY;
    bool duplicateInplace;
    RS_Vector getOffset() const;
    int penMode;
    int layerMode;
};

#endif // LC_ACTIONMODIFYDUPLICATE_H
