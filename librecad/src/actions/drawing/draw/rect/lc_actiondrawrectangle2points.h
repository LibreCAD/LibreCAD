/****************************************************************************
**
* Action that creates a rectangle defined by 2 points
* in one point

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
#ifndef LC_ACTIONDRAWRECTANGLE2POINTS_H
#define LC_ACTIONDRAWRECTANGLE2POINTS_H

#include "lc_abstractactiondrawrectangle.h"

class LC_ActionDrawRectangle2Points :public LC_AbstractActionDrawRectangle {
    Q_OBJECT

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
    int getSecondPointSnapMode() const{return secondPointSnapMode;};
    void setSecondPointSnapMode(int value);
protected:
    /**
     * position of corner 1
     */
    RS_Vector corner1 = RS_Vector(false);
    /*
     * flag that indicates that corner 1 is already set
     */
    bool corner1Set = false;

    /**
     * mode that indicates how snap second point
     */
    int secondPointSnapMode = SNAP_CORNER;

    RS_Polyline *createPolyline(const RS_Vector &snapPoint) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void processCommandValue(double value, bool &toMainStatus) override;
    bool processCustomCommand(int status, const QString &command, bool &toMainStatus) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    bool doCheckMayDrawPreview(QMouseEvent *pEvent, int status) override;
    void doAfterTrigger() override;
    void doUpdateMouseButtonHints(int status) override;
    RS_Vector createSecondCornerSnapForGivenRectSize(RS_Vector size);
    void doProcessCoordinateEvent(const RS_Vector &vector, bool zero, int status) override;
    int doGetStatusForInitialSnapToRelativeZero() override;
    void doInitialSnapToRelativeZero(RS_Vector vector) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
};

#endif // LC_ACTIONDRAWRECTANGLE2POINTS_H
