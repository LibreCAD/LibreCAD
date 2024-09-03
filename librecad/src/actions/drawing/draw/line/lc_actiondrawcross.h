/****************************************************************************
**
* Actions that draws a cross in the center of selected circle, arc, ellipse
* or ellipse arc

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
#ifndef LC_ACTIONDRAWCROSS_H
#define LC_ACTIONDRAWCROSS_H

#include <QMouseEvent>
#include "rs_coordinateevent.h"
#include "rs_entity.h"
#include "rs_snapper.h"
#include "rs_previewactioninterface.h"
#include "rs_line.h"
#include "lc_abstractactionwithpreview.h"

/**
 * Structure that contains information about cross lines
 */
struct LC_CrossData {
    LC_CrossData() :
        horizontal(),
        vertical(),
        centerPoint()
    {}

    LC_CrossData(const RS_Vector& horPoint1,
                 const RS_Vector& horPoint2,
                 const RS_Vector& vertPoint1,
                 const RS_Vector& vertPoint2,
                 const RS_Vector& center) :
        horizontal( horPoint1, horPoint2),
        vertical( vertPoint1, vertPoint2),
        centerPoint(center){}

    // horizontal line
    RS_LineData horizontal;
    // vertical line
    RS_LineData vertical;
    // center point
    RS_Vector centerPoint;
};

/**
 * Action that draws cross positioned in center of selected circle, arc or ellipse
 */

class LC_ActionDrawCross:public LC_AbstractActionWithPreview {
Q_OBJECT
public:
    LC_ActionDrawCross(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~LC_ActionDrawCross() override;

    double getLenX() const {return lenX;};
    double getLenY() const {return lenY;};
    double getCrossAngle() const{return angle;};
    int getCrossMode() const{return crossSizeMode;};

    void setXLength(double d) {lenX = d;};
    void setYLength(double d) {lenY = d;};
    void setCrossAngle(double d) {angle = d;};
    void setCrossMode(int i) {crossSizeMode = i;};
protected:
    enum Status {
        SetEntity      /**< Choose the circle / arc. */
    };

    /**
     * Defines mode for calculation of cross lines size
     */
    enum{
        CROSS_SIZE_EXTEND, // cross is outside shape for specified length
        CROSS_SIZE_LENGTH, // absolute length of cross lines
        CROSS_SIZE_PERCENT // cross length is calculated as percentage of shape radius
    };

    /** Chosen entity */
    RS_Entity *entity = nullptr;
    //list of entity types supported by current action
    const EntityTypeList circleType = EntityTypeList{RS2::EntityArc,
                                                     RS2::EntityCircle,
                                                     RS2::EntityEllipse/*,
                                                     RS2::EntitySplinePoints*/};

    /**
     * Mode that controls how the circle should be drawn
     */
    int crossSizeMode = CROSS_SIZE_EXTEND;
    /*
     * length value for axis x
     */
    double lenX = 0.0;
    /*
     * length value for axis x
     */
    double lenY = 0.0;
    /**
     * Angle between axis x and horizontal cross line
     */
    double angle = 0.0;

    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    bool doCheckMayTrigger() override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doAfterTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    bool doCheckMayTriggerOnInit(int status) override;
    bool isAcceptSelectedEntityToTriggerOnInit(RS_Entity *pEntity) override;
    void doCreateEntitiesOnTrigger(RS_Entity *en, QList<RS_Entity *> &list) override;
    void addCrossDataEntities(QList<RS_Entity *> &list, const LC_CrossData &crossData) const;
    void updateMouseButtonHints() override;
    LC_CrossData createCrossDataForEntity(RS_Entity *ent) const;
    LC_ActionOptionsWidget* createOptionsWidget() override;
};

#endif //LC_ACTIONDRAWCROSS_H
