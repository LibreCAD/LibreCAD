//
// Created by sand1 on 17/02/2024.
//

#ifndef LC_ACTIONDRAWCROSS_H
#define LC_ACTIONDRAWCROSS_H

#include <QMouseEvent>
#include "rs_coordinateevent.h"
#include "rs_entity.h"
#include "rs_snapper.h"
#include "rs_previewactioninterface.h"
#include "rs_line.h"
#include "lc_abstractactionwithpreview.h"

/**
 * Structure that contains information about cross lines
 */
struct LC_CrossData {
    LC_CrossData() :
        horizontal(),
        vertical(),
        centerPoint()
    {}

    LC_CrossData(const RS_Vector& horPoint1,
                 const RS_Vector& horPoint2,
                 const RS_Vector& vertPoint1,
                 const RS_Vector& vertPoint2,
                 const RS_Vector& center) :
        horizontal( horPoint1, horPoint2),
        vertical( vertPoint1, vertPoint2),
        centerPoint(center)
    {}

    // horizontal line
    RS_LineData horizontal;
    // vertical line
    RS_LineData vertical;
    // center point
    RS_Vector centerPoint;
};

/**
 * Action that draws cross positioned in center of selected circle, arc or ellipse
 */
class LC_ActionDrawCross:public LC_AbstractActionWithPreview {
Q_OBJECT

public:
    enum Status {
        SetEntity      /**< Choose the circle / arc. */
    };

    enum{
        CROSS_SIZE_EXTEND, // cross is outside shape for specified length
        CROSS_SIZE_LENGTH, // absolute length of cross lines
        CROSS_SIZE_PERCENT // cross length is calculated as percentage of shape radius
    };

    LC_ActionDrawCross(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~LC_ActionDrawCross() override;

    void coordinateEvent(RS_CoordinateEvent *e) override;
    void updateMouseButtonHints() override;

    double getLenX() {return lenX;};
    double getLenY() {return lenY;};
    double getCrossAngle(){return angle;};
    int getCrossMode(){return crossSizeMode;};

    void setXLength(double d);
    void setYLength(double d);
    void setCrossAngle(double d);
    void setCrossMode(int i);

protected:
    void createOptionsWidget() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    bool doCheckMayTrigger() override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doAfterTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
private:
    /** Chosen entity */
    RS_Entity *entity = nullptr;
    //list of entity types supported by current action
    const EntityTypeList circleType = EntityTypeList{RS2::EntityArc,
                                                     RS2::EntityCircle,
                                                     RS2::EntityEllipse/*,
                                                     RS2::EntitySplinePoints*/};

    /**
     * Mode that controls how the circle should be drawn
     */
    int crossSizeMode;
    /*
     * length value for axis x
     */
    double lenX;
    /*
     * length value for axis x
     */
    double lenY;
    /**
     * Angle between axis x and horizontal cross line
     */
    double angle;

    LC_CrossData createCrossData();
};

#endif //LC_ACTIONDRAWCROSS_H
