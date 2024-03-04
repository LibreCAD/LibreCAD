#ifndef LC_ACTIONDRAWLINERECTANGLEFIXED_H
#define LC_ACTIONDRAWLINERECTANGLEFIXED_H

#include "rs_previewactioninterface.h"
#include "rs_polyline.h"

class LC_ActionDrawLineRectangleFixed :public RS_PreviewActionInterface {
    Q_OBJECT


public:
    /**
        * Action States.
        */
    enum Status {
        SetPosition,
        SetReferencePoint,
        SetHeight,
        SetWidth,
        SetSize,
        SetAngle,
        SetCorners,
        SetBevels,
        SetRadius,
        SetUsePolyline
    };

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

    enum{
        DRAW_STAIGHT,
        DRAW_RADIUS,
        DRAW_BEVEL
    };

    static const std::vector<RS_Vector> snapPoints;


    LC_ActionDrawLineRectangleFixed(RS_EntityContainer& container,
    RS_GraphicView& graphicView);
    ~LC_ActionDrawLineRectangleFixed() override;

    void init(int status) override;

    void trigger() override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
    void updateMouseButtonHints() override;
    QStringList getAvailableCommands() override;

    bool isUsePolyline(){return usePolyline;};

    void setCornersMode(int value);
    int getCornersMode(){return cornersDrawMode;};

    void setSnapPointMode(int value);
    int getSnapPointMode(){return snapMode;};
    void setUsePolyline(bool value){usePolyline = value;};
    void setSnapToCornerArcCenter(bool b);
    bool isSnapToCornerArcCenter() {return snapToCornerArcCenter;};

    void setAngle(double angle);
    double getAngle(){return angle;}

    void setWidth(double value);
    double getWidth(){return width;};
    void setHeight(double value);
    double getHeight(){return height;};

    void setRadius(double radius);
    double getRadius(){return radius;};

    void setLengthX(double value);
    double getLengthX(){return bevelX;};
    void setLengthY(double value);
    double getLengthY(){return bevelY;};


protected:
    bool usePolyline;
    int snapMode;
    int cornersDrawMode;
    bool snapToCornerArcCenter;
    double width;
    double radius;
    double bevelX;
    double bevelY;
    double height;
    double angle;

    void createOptionsWidget() override;

    RS_Polyline *createPolyline(RS_Vector &snapPoint) const;
    RS_Polyline *resultingPolyline;


    RS_Vector lastSnapPoint;
    void drawPreviewForPoint(RS_Vector &lastSnapPoint);
    void drawPreviewForLastPoint();
};
#endif // LC_ACTIONDRAWLINERECTANGLEFIXED_H
