#ifndef LC_ACTIONDRAWCIRCLEBASE_H
#define LC_ACTIONDRAWCIRCLEBASE_H

#include <QMouseEvent>
#include "rs_previewactioninterface.h"

class LC_ActionDrawCircleBase:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    LC_ActionDrawCircleBase(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionDrawCircleBase() override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void updateMouseCursor() override;
    void init(int status = 0) override;
protected:
    virtual void reset();
    bool drawCreationPointsOnPreview = true; // fixme - move to options?
    bool moveRelPointAtCenterAfterTrigger = true; // fixme - move to options
};

#endif // LC_ACTIONDRAWCIRCLEBASE_H
