#ifndef LC_ACTIONDRAWRECTANGLE2POINTS_H
#define LC_ACTIONDRAWRECTANGLE2POINTS_H

#include "lc_abstractactiondrawrectangle.h"
#include "rs_previewactioninterface.h"

class LC_ActionDrawRectangle2Points :public LC_AbstractActionDrawRectangle {
    Q_OBJECT

    void setMainStatus() override;

public:

    enum
    {
        SNAP_CORNER, // corner of rectangle
        SNAP_EDGE_VERT, // middle of vertical edge
        SNAP_EDGE_HOR, // middle of horizontal edge
        SNAP_MIDDLE // middle of rectangle
    };

    LC_ActionDrawRectangle2Points(RS_EntityContainer& container, RS_GraphicView& graphicView);
    ~LC_ActionDrawRectangle2Points() override;

    QStringList getAvailableCommands() override;
    void init(int status) override;
    int getSecondPointSnapMode();
    void setSecondPointSnapMode(int value);

protected:
    RS_Vector corner1;
    bool corner1Set;
    bool squareDrawRequested;
    int secondPointSnapMode;

    RS_Polyline *createPolyline(const RS_Vector &snapPoint) const override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed) override;
    void processCommandValue(double value) override;
    bool processCustomCommand(RS_CommandEvent *e, const QString &command, bool &toMainStatus) override;
    void createOptionsWidget() override;
    bool doCheckMayDrawPreview(QMouseEvent *pEvent, int status) override;
    void doAfterTrigger() override;
    void doUpdateMouseButtonHints() override;
    RS_Vector createSecondCornerSnapForGivenRectSize(RS_Vector size);
    bool onMouseMove(QMouseEvent *e, RS_Vector snap, int status) override;
    void doProcessCoordinateEvent(const RS_Vector &vector, bool zero, int status) override;
    int doRelZeroInitialSnapState() override;
    void doRelZeroInitialSnap(RS_Vector vector) override;
};

#endif // LC_ACTIONDRAWRECTANGLE2POINTS_H
