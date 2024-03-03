#ifndef LC_ACTIONDRAWLINEANGLEREL_H
#define LC_ACTIONDRAWLINEANGLEREL_H

#include "rs_previewactioninterface.h"
#include "rs_line.h"

class LC_ActionDrawLineAngleRel :public RS_PreviewActionInterface {
    Q_OBJECT

public:

    enum{
        SNAP_START,
        SNAP_MIDDLE,
        SNAP_END
    };

    enum {
        SetLine,
        SetSnapDistance,
        SetLineSnap,
        SetTickAngle,
        SetTickLength,
        SetTickOffset,
        SetTickSnap
    };


    LC_ActionDrawLineAngleRel(RS_EntityContainer& container,
    RS_GraphicView& graphicView,
    double angle = 0.0,
    bool fixedAngle = false);

    ~LC_ActionDrawLineAngleRel() override;

    RS2::ActionType rtti() const override;

    void finish(bool updateTB) override;

    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void hideOptions() override;
    void showOptions() override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;


    void setLineSnapMode(int mode);
    int getLineSnapMode(){return lineSnapMode;};
    void setTickSnapMode(int mode);
    int getTickSnapMode(){return tickSnapMode;};

    void setTickAngle(double a);
    double getTickAngle(){return tickAngle;};
    void setTickLength(double len);
    double getTickLength(){return tickLength;};
    void setTickOffset(double o);
    double getTickOffset(){return tickOffset;};
    void setSnapDistance(double d);
    double getSnapDistance() {return snapDistance;};
    void setSegmentsCount(int c);

    bool hasFixedAngle(){return fixedAngle;};

    bool isAngleRelative(){return relativeAngle;};
    void setAngleIsRelative(bool value){relativeAngle = value;};

    bool isLengthFree(){return lengthIsFree;};
    void setLengthIsFree(bool value){lengthIsFree = value;};

    static RS_Vector getNearestPointOnLine(RS_Line* line, const RS_Vector &coord, bool infiniteLine);
    static RS_Vector getNearestPointOnInfiniteLine(const RS_Vector &coord, const RS_Vector &lineStartPoint, const RS_Vector &lineEndPoint);

private:
    int lineSnapMode {SNAP_START};
    int tickSnapMode {SNAP_END};
    double tickAngle;
    double tickLength {50};
    double tickOffset {0};

    double snapDistance;
    int segmentsCount;

    RS_Line* line;

    RS_LineData tickLineData;

    void prepareLineData();

    /** Chosen position */
    RS_Vector tickSnapPosition;
    RS_Vector tickEndPosition;



    bool fixedAngle;
    bool relativeAngle {false};
    bool lengthIsFree {false};
    int getSnapModeFromCommand(QString &command);

    void updateTickSnapPosition(double distanceOnLine);
    RS_Vector obtainLineSnapPointForMode() const;
    int getPointPosition(RS_Vector &startPos, RS_Vector &endPos, RS_Vector &point);
    RS_Vector detectNearestPointOnLine(const RS_Vector &coord, bool infiniteLine);


};

#endif // LC_ACTIONDRAWLINEANGLEREL_H
