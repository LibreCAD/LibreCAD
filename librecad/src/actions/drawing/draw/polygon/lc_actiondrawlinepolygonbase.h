#ifndef LC_ACTIONDRAWLINEPOLYGONBASE_H
#define LC_ACTIONDRAWLINEPOLYGONBASE_H

#include "rs_previewactioninterface.h"

class LC_ActionDrawLinePolygonBase:public RS_PreviewActionInterface{
    Q_OBJECT

public:

    LC_ActionDrawLinePolygonBase(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView, RS2::ActionType actionType);
    ~LC_ActionDrawLinePolygonBase() override;

    void mouseMoveEvent(QMouseEvent *e) override;
    QStringList getAvailableCommands() override;

    int getNumber() const{return number;}
    void setNumber(int n) {number = n;}
    bool isPolyline() const {return createPolyline;};
    void setPolyline(bool value){ createPolyline = value;};
    bool isCornersRounded() const {return roundedCorners;};
    void setCornersRounded(bool value){roundedCorners = value;};
    double getRoundingRadius(){return roundingRadius;}
    void setRoundingRadius(double val){roundingRadius = val;}
    void updateMouseButtonHints() override;
protected:
    /** Number of edges. */
    int number = 0;

    enum Status {
        SetPoint1,
        SetPoint2,
        SetNumber,
        SetRadius
    };

    struct Points {
        RS_Vector point1;
        RS_Vector point2;
    };

    struct PolygonInfo{
        RS_Vector centerPoint{false};
        double startingAngle = 0.0;
        double vertexRadius = 0.0;
        double innerRadius = 0.0;
    };

    std::unique_ptr<Points> pPoints;

/** Last status before entering text. */
    Status lastStatus = SetPoint1;

    bool createPolyline = false;
    bool roundedCorners = false;
    double roundingRadius = 0.0;

    bool completeActionOnTrigger = false;

    LC_ActionOptionsWidget* createOptionsWidget() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool doProcessCommand(int status, const QString &command) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;

    virtual QString getPoint1Hint() const;
    virtual QString getPoint2Hint() const = 0;

    void createPolygonPreview(const RS_Vector& mouse);
    virtual void previewAdditionalReferences([[maybe_unused]]const RS_Vector &mouse) {};
    virtual void preparePolygonInfo([[maybe_unused]]PolygonInfo &polygonInfo, [[maybe_unused]]const RS_Vector &snap){};
    RS_Polyline *createShapePolyline(PolygonInfo &polygonInfo, bool preview);
    void doTrigger() override;
};

#endif // LC_ACTIONDRAWLINEPOLYGONBASE_H
