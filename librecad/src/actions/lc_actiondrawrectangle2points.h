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

    RS_Polyline *createPolyline(RS_Vector &snapPoint) const override;
    void proceedMouseLeftButtonReleasedEvent(QMouseEvent *e) override;
    void processCommandValue(double value) override;
    bool processCustomCommand(RS_CommandEvent *e, const QString &command, bool &toMainStatus) override;
    void processCoordinateEvent(RS_CoordinateEvent *pEvent, RS_Vector vector, bool zero) override;
    void createOptionsWidget() override;
    bool mayDrawPreview(QMouseEvent *pEvent) override;
    void doAfterTrigger() override;
    void processMouseEvent(QMouseEvent *e) override;
    void doUpdateMouseButtonHints() override;
    RS_Vector createSecondCornerSnapForGivenRectSize(RS_Vector size);
};

#endif // LC_ACTIONDRAWRECTANGLE2POINTS_H
