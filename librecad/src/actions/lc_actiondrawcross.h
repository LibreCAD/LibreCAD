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

struct LC_CrossData {
    LC_CrossData() :
        horizontal(),
        vertical()
    {}

    LC_CrossData(const RS_LineData& horLine,
                const RS_LineData& vertLine) :
        horizontal( horLine),
        vertical( vertLine)
    {}

    LC_CrossData(const RS_Vector& horPoint1,
                 const RS_Vector& horPoint2,
                 const RS_Vector& vertPoint1,
                 const RS_Vector& vertPoint2) :
        horizontal( horPoint1, horPoint2),
        vertical( vertPoint1, vertPoint2)
    {}

    RS_LineData horizontal;
    RS_LineData vertical;
};

class LC_ActionDrawCross:public RS_PreviewActionInterface {
Q_OBJECT

public:
    enum Status {
        SetCircle      /**< Choose the circle / arc. */
    };

    LC_ActionDrawCross(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~LC_ActionDrawCross() override;

    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void coordinateEvent(RS_CoordinateEvent *e) override;
    void updateMouseButtonHints() override;
    void updateMouseCursor() override;

    void showOptions() override;
    void hideOptions() override;

    double getLenX() {return lenX;};
    double getLenY() {return lenY;};
    double getCrossAngle(){return orientation;};
    int getCrossMode(){return crossMode;};

    void setXLength(double d);
    void setYLength(double d);
    void setCrossAngle(double d);
    void setCrossMode(int i);

private:
    /** Chosen startpoint */
    std::unique_ptr<RS_Vector> point;
    /** Chosen entity */
    RS_Entity *circle = nullptr;
    //list of entity types supported by current action
    const EntityTypeList circleType = EntityTypeList{RS2::EntityArc,
                                                     RS2::EntityCircle,
                                                     RS2::EntityEllipse/*,
                                                     RS2::EntitySplinePoints*/};

    int crossMode;
    double lenX;
    double lenY;
    double orientation;
    LC_CrossData createCrossData();
};

#endif //LC_ACTIONDRAWCROSS_H
