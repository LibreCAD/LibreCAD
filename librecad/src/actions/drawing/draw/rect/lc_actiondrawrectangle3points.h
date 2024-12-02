/****************************************************************************
**
* Abstract base class for actions that draws a rectangle (or quadrangle)
* based on 3 points

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H
#define LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H

#include "rs_vector.h"
#include "rs_polyline.h"
#include "lc_abstractactiondrawrectangle.h"


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

    int getEndZeroPointCorner() const {return endRelativeZeroPointCorner;};
    void setEndZeroPointCorner(int value){endRelativeZeroPointCorner = value;};

    void setCreateQuadrangle(bool value) {createQuadrangle = value;};
    bool isCreateQuadrangle() const {return createQuadrangle;};

    double getFixedInnerAngle() const {return innerAngle;};
    void setFixedInnerAngle(double value){innerAngle = value;};

    bool isInnerAngleFixed() const{return innerAngleIsFixed;};
    void setInnerAngleFixed(bool value){innerAngleIsFixed = value;};

protected:
    /**
     * information about corners
     */
    struct Points;
    int endRelativeZeroPointCorner {3};
    std::unique_ptr<Points> pPoints;

    /**
     * inner angle of quadrangle
     */
    double innerAngle = 0.0;
    /**
     * flag that indicates that inner angle of quadrangle is fixed
     */
    bool innerAngleIsFixed = false;

    /**
     * if true, quadrangle will be created, if false - rectangle
     */
    bool createQuadrangle = false;

    void resetPoints();
    void doResetPoints(const RS_Vector &zero);
    RS_Vector calculatePossibleEndpointForAngle(const RS_Vector &snap, const RS_Vector lineStartPoint, double angle) const;
    void calculateCorner2(const RS_Vector &snapPoint, double angleRad, bool cornerSet) const;
    void calculateCorner4() const;
    RS_Polyline *createPolyline(const RS_Vector &snapPoint) override;
    void processCommandValue(double value, bool &toMainStatus) override;
    bool processCustomCommand(int status, const QString &command, bool &toMainStatus) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    RS_Vector doGetMouseSnapPoint(QMouseEvent *e) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doAfterTrigger() override;
    void doBack(QMouseEvent *pEvent, int status) override;
    void doProcessCoordinateEvent(const RS_Vector &vector, bool zero, int status) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void doFinish(bool updateTB) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    int doGetStatusForInitialSnapToRelativeZero() override;
    void doInitialSnapToRelativeZero(RS_Vector vector) override;
    void doUpdateMouseButtonHints(int status) override;
    void calculateCornersBySize(RS_Vector size);
    double getActualInnerAngle() const;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
};

#endif //LIBRECAD_MASTER_LC_ACTIONDRAWLINERECTANGLEREL_H
