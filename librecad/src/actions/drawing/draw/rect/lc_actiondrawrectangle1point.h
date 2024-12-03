/****************************************************************************
**
* Action that creates a rectangle defined by fixed width and height and snapped
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

#ifndef LC_ACTIONDRAWRECTANGLE1POINT_H
#define LC_ACTIONDRAWRECTANGLE1POINT_H

#include "rs_polyline.h"
#include "lc_abstractactiondrawrectangle.h"

class LC_ActionDrawRectangle1Point :public LC_AbstractActionDrawRectangle {
    Q_OBJECT
public:

    /**
     * points of rectangle to which snap should be performed on rect insertion
     */
    enum{
        SNAP_TOP_LEFT, // top-left corner
        SNAP_TOP, // middle of top edge
        SNAP_TOP_RIGHT, // top-right corner
        SNAP_LEFT, // middle of left edge
        SNAP_MIDDLE, // center point
        SNAP_RIGHT, // middle of right edge
        SNAP_BOTTOM_LEFT, // bottom-left corner
        SNAP_BOTTOM, // middle of bottom edge
        SNAP_BOTTOM_RIGHT // bottom-right corner
    };

    LC_ActionDrawRectangle1Point(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView);
    ~LC_ActionDrawRectangle1Point() override;

    QStringList getAvailableCommands() override;
    void setWidth(double value);
    double getWidth()const {return width;};
    void setHeight(double value);
    double getHeight() const{return height;};
    void setSizeInner(bool value);
    bool isSizeInner() const{return sizeIsInner;};
    bool isBaseAngleFree() const {return angleIsFree;};
    void setBaseAngleFree(bool val);
protected:
    // width of rect
    double width = 0.0;
    // height of rect
    double height = 0.0;
    // flag that indicates that width and rect are applied to external area or excluding corner radius
    bool sizeIsInner = false;
    // flag that indicates that base angle is set by mouse instead of settings
    bool angleIsFree = false;
    // indicates that CTRL is pressed on trigger invocation by mouse
    bool controlPressedOnMouseRelease = false;

    RS_Vector insertionPoint = RS_Vector{false};

    static const std::vector<RS_Vector> snapPoints;

    LC_ActionOptionsWidget* createOptionsWidget() override;
    RS_Polyline *createPolyline(const RS_Vector &snapPoint) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void processCommandValue(double value, bool &toMainStatus) override;
    bool processCustomCommand(int status, const QString &command,bool &toMainStatus) override;
    void doUpdateMouseButtonHints(int status) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    void doProcessCoordinateEvent(const RS_Vector &vector, bool zero, int status) override;
    int doGetStatusForInitialSnapToRelativeZero() override;
    void doInitialSnapToRelativeZero(RS_Vector vector) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;

    RS_Vector doGetMouseSnapPoint(QMouseEvent *e) override;

    void doBack(QMouseEvent *pEvent, int status) override;

    RS_Vector doGetRelativeZeroAfterTrigger() override;

    void doAfterTrigger() override;
};
#endif // LC_ACTIONDRAWRECTANGLE1POINT_H
