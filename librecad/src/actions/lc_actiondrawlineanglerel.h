#ifndef LC_ACTIONDRAWLINEANGLEREL_H
#define LC_ACTIONDRAWLINEANGLEREL_H

#include "rs_previewactioninterface.h"
#include "rs_line.h"
#include "lc_abstractactionwithpreview.h"

class LC_ActionDrawLineAngleRel :public LC_AbstractActionWithPreview {
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

    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void updateMouseButtonHints() override;

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
    RS_Vector detectNearestPointOnLine(const RS_Vector &coord, bool infiniteLine);
protected:
    void createOptionsWidget() override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doBack(QMouseEvent *pEvent, int status) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed) override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doAfterTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;

};

#endif // LC_ACTIONDRAWLINEANGLEREL_H
