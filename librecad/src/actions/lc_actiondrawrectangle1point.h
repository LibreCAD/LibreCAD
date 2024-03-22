#ifndef LC_ACTIONDRAWRECTANGLE1POINT_H
#define LC_ACTIONDRAWRECTANGLE1POINT_H

#include "rs_previewactioninterface.h"
#include "rs_polyline.h"
#include "lc_abstractactiondrawrectangle.h"

class LC_ActionDrawRectangle1Point :public LC_AbstractActionDrawRectangle {
    Q_OBJECT


public:

    /**
     * points of rectangle to which snap should be performed on rect insertion
     */
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

    QStringList getAvailableCommands() override;

    void setWidth(double value);
    double getWidth(){return width;};
    void setHeight(double value);
    double getHeight(){return height;};

    void setSizeInner(bool value);
    bool isSizeInner(){return sizeIsInner;};

protected:
    // width of rect
    double width;
    // height of rect
    double height;
    // flag that indicates that width and rect are applied to external area or excluding corner radius
    bool sizeIsInner;

    static const std::vector<RS_Vector> snapPoints;

    void createOptionsWidget() override;

    RS_Polyline *createPolyline(const RS_Vector &snapPoint) const override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed) override;
    void processCommandValue(double value) override;
    void setMainStatus() override;
    bool processCustomCommand(RS_CommandEvent *e, const QString &command,bool &toMainStatus) override;
    void doUpdateMouseButtonHints() override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    void doProcessCoordinateEvent(const RS_Vector &vector, bool zero, int status) override;
};
#endif // LC_ACTIONDRAWRECTANGLE1POINT_H
