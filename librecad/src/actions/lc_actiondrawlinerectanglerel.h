//
// Created by sand1 on 15/02/2024.
//

#ifndef LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H
#define LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"
#include "rs_polyline.h"

class LC_ActionDrawLineRectangleRel :public RS_PreviewActionInterface {
    Q_OBJECT
public:
    /**
        * Action States.
        */
    enum Status {
        SetWidth,
        SetHeight,
        SetStart,
        SetAngle
    };

    enum{
        SNAP_CORNER1,
        SNAP_CORNER2,
        SNAP_CORNER3,
        SNAP_CORNER4
    };

    enum{
        DRAW_STAIGHT,
        DRAW_RADIUS,
        DRAW_BEVEL
    };

    LC_ActionDrawLineRectangleRel(RS_EntityContainer& container,
    RS_GraphicView& graphicView);
    ~LC_ActionDrawLineRectangleRel() override;

    void init(int status) override;

    void trigger() override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;
    void setStartState();

    int getEndZeroPointCorner(){return endRelativeZeroPointCorner;};
    void setEndZeroPointCorner(int value){endRelativeZeroPointCorner = value;};

    void setUsePolyline(bool value){usePolyline = value;};

    void setRadius(double radius);
    double getRadius(){return radius;};

    void setLengthX(double value);
    double getLengthX(){return bevelX;};

    void setLengthY(double value);
    double getLengthY(){return bevelY;};

    bool isUsePolyline(){return usePolyline;};

    void setCornersMode(int value);
    int getCornersMode(){return cornersDrawMode;};




protected:
    struct Points;
    int endRelativeZeroPointCorner {3};
    std::unique_ptr<Points> pPoints;
    bool widthIsSet{false};

    double angle;
    bool usePolyline;
    int radius;
    int cornersDrawMode;
    double bevelX;
    double bevelY;

    QString evaluateFraction(QString input, QRegExp rx, int index, int tailI);
    QString updateForFraction(QString input);
    void resetPoints();
    void doResetPoints(const RS_Vector &zero);
    RS_Vector calculatePossibleEndpointForAngle(const RS_Vector &snap, const RS_Vector lineStartPoint, double angle);
    RS_Polyline *createPolyline() const;
    RS_Vector calculateAngleEndpoint(const RS_Vector &startPoint, double angle, double length);
    void setStatusAfterAngleValue();
    void calculateCorner4();
    void toHeightExpectedState();
    void toWidthExpectedState();
};

#endif //LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H
