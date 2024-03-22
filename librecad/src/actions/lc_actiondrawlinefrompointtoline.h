#ifndef LC_ACTIONDRAWLINEFROMPOINTTOLINE_H
#define LC_ACTIONDRAWLINEFROMPOINTTOLINE_H

#include "lc_abstractactionwithpreview.h"
/**
 * Actions draws line from given point to selected target line. Angle between target line and created line is either user defined
 * in range (-90..90 degrees) or fixed to 90 degrees (so line orthogonal to target line is created).
 *
 * The lengths of created line may be either fixed, or be from start point to the point of intersection of created line and target line.
 */
class LC_ActionDrawLineFromPointToLine:public LC_AbstractActionWithPreview
{
public:

    // action state
    enum{
        SetPoint,
        SelectLine
    };

    /**
     * How to determine the length of created line
     */
    enum{
        SIZE_INTERSECTION,
        SIZE_FIXED_LENGTH
    };

    /**
     * Positioning of created line relating to start point (if fixed length of line is used)
     */
    enum {
        SNAP_START,
        SNAP_MIDDLE,
        SNAP_END
    };


    LC_ActionDrawLineFromPointToLine(RS_EntityContainer &container, RS_GraphicView &graphicView);

    ~LC_ActionDrawLineFromPointToLine() override = default;
    void setLineSnapMode(int val) {lineSnapMode = val;};
    int getLineSnapMode(){return lineSnapMode;};
    void setOrthogonal(bool value){orthogonalMode = value;};
    bool getOrthogonal(){return orthogonalMode;};
    void setAngle(double ang){angle = ang;};
    double getAngle(){return angle;};
    int getSizeMode(){return sizeMode;};
    void setSizeMode(int mode){sizeMode = mode;};
    void setLength(double len){length = len;};
    double getLength(){return length;}

    void updateMouseButtonHints() override;

protected:
    void createOptionsWidget() override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    RS2::CursorType doGetMouseCursor(int status) override;

    /**
     * Target line to which line will be created
     */
    RS_Line *targetLine;
    /**
     * start point from which line will be created
     */
    RS_Vector startPoint;
    /**
     * desired angle between target line and line that will be built. angle 0..90 - line will be closer to left corner of target line,
     * -90..0 - will be closer to right corner of target line (considering that start point is located above the target line).
     */
    double angle;
    /**
     * controls the mode for line length calculation
     */
    int sizeMode;
    /**
     * mode for snapping fixed size line relative to start point
     */
    int lineSnapMode;
    /**
     * flag determines whether angle is fixed to 90 degrees
     */
    bool orthogonalMode;
    /**
     * Length of the line if it is fixed
     */
    double length;

    bool alternateAngle;


    RS_Line *createOrtLine(RS_Line *line);
    int doRelZeroInitialSnapState() override;
    void doRelZeroInitialSnap(RS_Vector vector) override;
    void onCoordinateEvent(const RS_Vector &coord, bool isZero, int status) override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    void doAfterTrigger() override;
    void doMouseMoveEnd(int status, QMouseEvent *e) override;
    void doMouseMoveStart(int status, QMouseEvent *pEvent, bool shiftPressed) override;
};

#endif // LC_ACTIONDRAWLINEFROMPOINTTOLINE_H
