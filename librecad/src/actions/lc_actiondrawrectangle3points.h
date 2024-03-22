//
// Created by sand1 on 15/02/2024.
//

#ifndef LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H
#define LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H

#include "rs_previewactioninterface.h"
#include "rs_vector.h"
#include "rs_polyline.h"
#include "lc_abstractactiondrawrectangle.h"

// fixme - rename to rect3 points
class LC_ActionDrawRectangle3Points :public LC_AbstractActionDrawRectangle {
    Q_OBJECT



public:

    enum{
        SNAP_CORNER1,
        SNAP_CORNER2,
        SNAP_CORNER3,
        SNAP_CORNER4
    };

    LC_ActionDrawRectangle3Points(RS_EntityContainer& container,
                                  RS_GraphicView& graphicView);
    ~LC_ActionDrawRectangle3Points() override;

    void init(int status) override;

    void commandEvent(RS_CommandEvent* e) override;

    void updateMouseButtonHints() override;
    void updateMouseCursor() override;
    void setStartState();

    int getEndZeroPointCorner(){return endRelativeZeroPointCorner;};
    void setEndZeroPointCorner(int value){endRelativeZeroPointCorner = value;};

    void setCreateQuadrangle(bool value) {createQuadrangle = value;};
    bool isCreateQuadrangle() {return createQuadrangle;};

    double getFixedInnerAngle() {return innerAngle;};
    double setFixedInnerAngle(double value){innerAngle = value;};

    bool isInnerAngleFixed(){return innerAngleIsFixed;};
    void setInnerAngleFixed(bool value){innerAngleIsFixed = value;};

protected:
    struct Points;
    int endRelativeZeroPointCorner {3};
    std::unique_ptr<Points> pPoints;
    bool widthIsSet{false};

    double angle;
    double innerAngle;
    bool innerAngleIsFixed;
    bool createQuadrangle;
    bool squareDrawRequested;


    void resetPoints();
    void doResetPoints(const RS_Vector &zero);
    RS_Vector calculatePossibleEndpointForAngle(const RS_Vector &snap, const RS_Vector lineStartPoint, double angle) const;
    RS_Polyline *createPolyline() const;
    RS_Vector calculateAngleEndpoint(const RS_Vector &startPoint, double angle, double length);
    void setStatusAfterAngleValue();
    void calculateCorner4() const;
    void toHeightExpectedState();
    void toWidthExpectedState();
    RS_Polyline *createPolyline(const RS_Vector &snapPoint) const override;
    void setMainStatus() override;
    void processCommandValue(double value) override;
    bool processCustomCommand(RS_CommandEvent *e, const QString &command, bool &toMainStatus) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    RS_Vector doGetMouseSnapPoint(QMouseEvent *e, bool shiftPressed) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doAfterTrigger() override;
    void doBack(QMouseEvent *pEvent, int status) override;
    void doProcessCoordinateEvent(const RS_Vector &vector, bool zero, int status) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint, bool shiftPressed) override;
    void doFinish(bool updateTB) override;
    void createOptionsWidget() override;
    int doRelZeroInitialSnapState() override;
    void doRelZeroInitialSnap(RS_Vector vector) override;
};

#endif //LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H
