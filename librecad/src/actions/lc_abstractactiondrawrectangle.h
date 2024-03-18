#ifndef LC_ABSTRACTACTIONDRAWRECTANGLE_H
#define LC_ABSTRACTACTIONDRAWRECTANGLE_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"
#include "rs_polyline.h"
#include "lc_abstractactionwithpreview.h"

class LC_AbstractActionDrawRectangle:public LC_AbstractActionWithPreview {
Q_OBJECT

public:
    LC_AbstractActionDrawRectangle(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_AbstractActionDrawRectangle() override;

    /**
     * defines modes for drawing rect corners
     */
    enum {
        DRAW_STRAIGHT, // plain rect
        DRAW_RADIUS,  // rounded corners
        DRAW_BEVEL // bevels
    };

    /**
      * Action States.
      */
    enum Status {
        SetPoint1,
        SetPoint2,
        SetPoint3,
        SetHeight,
        SetWidth,
        SetPoint1Snap,
        SetPoint2Snap,
        SetSize,
        SetAngle,
        SetCorners,
        SetBevels,
        SetRadius,
        LAST_BASE_STATUS
    };


    void updateMouseButtonHints() override;

    bool isUsePolyline(){return usePolyline;};

    void setUsePolyline(bool value){usePolyline = value;};
    void setRadius(double radius);

    double getRadius(){return radius;};

    void setLengthX(double value);

    double getLengthX(){return bevelX;};

    void setLengthY(double value);

    double getLengthY(){return bevelY;};
    void setAngle(double angle);

    double getAngle(){return angle;}

    void setCornersMode(int value);

    int getCornersMode(){return cornersDrawMode;};

    void setInsertionPointSnapMode(int value);
    int getInsertionPointSnapMode(){return insertionPointSnapMode;};

    void setSnapToCornerArcCenter(bool b);
    bool isSnapToCornerArcCenter() {return snapToCornerArcCenter;};

protected:
    /**
     * should resulting rect be polyline or not
     */
    bool usePolyline;
    /**
     * flag that controls how to draw corners
     */
    int cornersDrawMode;
    /**
     * radius for rounded corners
     */
    double radius;
    /**
     * x value of bevel
     */
    double bevelX;
    /**
     * y value of bevel
     */
    double bevelY;
    /**
     * angle of rect's rotation
     */
    double angle;
    /**
     * flag that controls how to position rect relative to insertion point - may have different meanings in different actions
     */
    int insertionPointSnapMode;
    /**
     * flag that indicates that snap should be performed taking into consideration rounded corners (if true, snap is not for, say, corner,
     * but to center of nearest rounded corner arc
     */
    bool snapToCornerArcCenter;

    struct ShapeData{
       /**
     * built polyline for shape
     */
        RS_Polyline *resultingPolyline;
        RS_Vector snapPoint;
    };

    ShapeData* shapeData;

    void prepareCornersDrawMode(double &radiusX, double &radiusY, bool &drawComplex, bool &drawBulge) const;
    RS_Polyline* createPolylineByVertexes( RS_Vector bottomLeftCorner, RS_Vector bottomRightCorner,
                                           RS_Vector topRightCorner, RS_Vector topLeftCorner,
                                           bool drawBulge, bool drawComplex,
                                           double radiusX, double radiusY) const;

    virtual RS_Polyline *createPolyline(const RS_Vector &snapPoint) const = 0;

    void createShapeData(const RS_Vector &snapPoint);

    virtual void setMainStatus() = 0;
    virtual void processCommandValue(double value) = 0;
    virtual bool processCustomCommand(RS_CommandEvent *e, const QString &command, bool &toMainStatus) = 0;
    virtual void doProcessCoordinateEvent(const RS_Vector &vector, bool zero, int status);
    virtual void doUpdateMouseButtonHints();

    void onOnCoordinateEvent(const RS_Vector &coord, bool isZero, int status) override;
    bool doProcessCommand(RS_CommandEvent *e, const QString &c) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void normalizeCorners(RS_Vector &bottomLeftCorner, RS_Vector &bottomRightCorner, RS_Vector &topRightCorner, RS_Vector &topLeftCorner) const;
    void stateUpdated(bool toMainStatus);
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    bool doCheckMayTrigger() override;
    void doAfterTrigger() override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doBack(QMouseEvent *pEvent, int status) override;
};

#endif // LC_ABSTRACTACTIONDRAWRECTANGLE_H
