#ifndef LC_ACTIONDRAWELLIPSE1POINT_H
#define LC_ACTIONDRAWELLIPSE1POINT_H

#include "lc_actiondrawcirclebase.h"

class LC_ActionDrawEllipse1Point:public LC_ActionDrawCircleBase{
Q_OBJECT
public:
    LC_ActionDrawEllipse1Point(
        RS_EntityContainer &container, RS_GraphicView &graphicView, bool isArc);

    ~LC_ActionDrawEllipse1Point() override;

    double getMajorRadius();
    double getMinorRadius();
    double getAngle();
    bool hasAngle();
    bool isAngleFree();
    void setMajorRadius(double val);
    void setMinorRadius(double val);
    void setAngle(double val);
    void setHasAngle(bool val);
    void setAngleFree(bool val);

    void mouseMoveEvent(QMouseEvent *event) override;

    void trigger() override;

protected:
    struct Points;

    enum Status{
        SetPoint,
        SetMajorAngle,
        SetAngle1,
        SetAngle2
    };

    std::unique_ptr<Points> pPoints;

    LC_ActionOptionsWidget *createOptionsWidget() override;
    void updateMouseButtonHints() override;
    bool doProcessCommand(int status, const QString &command) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
};

#endif // LC_ACTIONDRAWELLIPSE1POINT_H
