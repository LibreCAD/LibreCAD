#ifndef LC_ACTIONDRAWRECTANGLE1POINT_H
#define LC_ACTIONDRAWRECTANGLE1POINT_H

#include "rs_previewactioninterface.h"
#include "rs_polyline.h"
#include "lc_abstractactiondrawrectangle.h"

class LC_ActionDrawRectangle1Point :public LC_AbstractActionDrawRectangle {
    Q_OBJECT


public:

    enum{
        SNAP_TOP_LEFT,
        SNAP_TOP,
        SNAP_TOP_RIGHT,
        SNAP_LEFT,
        SNAP_MIDDLE,
        SNAP_RIGHT,
        SNAP_BOTTOM_LEFT,
        SNAP_BOTTOM,
        SNAP_BOTTOM_RIGHT
    };

    LC_ActionDrawRectangle1Point(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
    ~LC_ActionDrawRectangle1Point() override;

    void init(int status) override;

    void updateMouseButtonHints() override;
    QStringList getAvailableCommands() override;

    void setWidth(double value);
    double getWidth(){return width;};
    void setHeight(double value);
    double getHeight(){return height;};

    void setSizeInner(bool value);
    bool isSizeInner(){return sizeIsInner;};

protected:

    double width;
    double height;

    bool sizeIsInner;

    static const std::vector<RS_Vector> snapPoints;

    void createOptionsWidget() override;

    RS_Polyline *createPolyline(RS_Vector &snapPoint) const override;
    void proceedMouseLeftButtonReleasedEvent(QMouseEvent *e) override;
    void processCommandValue(double value) override;
    void setMainStatus() override;
    bool processCustomCommand(RS_CommandEvent *e, const QString &command) override;
    void processCoordinateEvent(RS_CoordinateEvent *pEvent, RS_Vector vector, bool zero) override;
    void doAfterTrigger() override;
};
#endif // LC_ACTIONDRAWRECTANGLE1POINT_H
