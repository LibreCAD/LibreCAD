/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/
#ifndef LC_ACTIONDRAWSTAR_H
#define LC_ACTIONDRAWSTAR_H

#include "lc_abstractactionwithpreview.h"

#define STAR_MIN_RAYS 3
#define STAR_MAX_RAYS 99

// fixme - add center point on preview
class LC_ActionDrawStar:public LC_AbstractActionWithPreview{
   Q_OBJECT
public:
    LC_ActionDrawStar(LC_ActionContext *actionContext);
    QStringList getAvailableCommands() override;
    double getRadiusInner() const {return m_innerRadius;};
    void setRadiusInner(double d);
    double getRadiusOuter() const {return m_outerRadius;};
    void setRadiusOuter(double d);
    int getRaysNumber() const {return m_raysNumber;};
    void setRaysNumber(int i);
    bool isOuterRounded() const {return m_outerRadiusRounded;};
    void setOuterRounded(bool value);
    bool isInnerRounded() const {return m_innerRadiusRounded;};
    void setInnerRounded(bool value);
    bool isPolyline() const {return m_createPolyline;};
    void setPolyline(bool value);
    bool isSymmetric() const {return m_symmetric;};
    void setSymmetric(bool value);
protected:
    /**
 * Action statuses
 */
    enum{
        SetCenter,
        SetOuterPoint,
        SetInnerPoint,
        SetRadiuses,
        SetRays
    };
    /**
     * outer point of rays
     */
    RS_Vector m_outerPoint = RS_Vector(false);
    /**
     * inner point of rays
     */
    RS_Vector m_innerPoint  = RS_Vector(false);
    /**
     * center point of star
     */
    RS_Vector m_centerPoint  = RS_Vector(false);
    /**
     * is star symmetric or not
     */
    bool m_symmetric = true;
    /**
     * determines whether inner ray junction should be rounded
     */
    bool m_innerRadiusRounded = false;
    /**
     * determines whether outer ray junction should be rounded
     */
    bool m_outerRadiusRounded = true;
    /**
     * inner rays rounding radius
     */
    double m_innerRadius = 4;
    /**
     * outer rays rounding radius
     */
    double m_outerRadius = 10;
    /**
     * determines whether entire start should be drawn using polyline or via individual entities
     */
    bool m_createPolyline = false;
    /**
     * amount of rays for star
     */
    int m_raysNumber = 5;

    void addPolylineToEntitiesList(RS_Polyline *pPolyline, QList<RS_Entity *> &list, bool b);
    RS_Polyline *createShapePolyline(RS_Vector &snap, QList<RS_Entity *> &list, int status, bool preview);
    void doOnLeftMouseButtonRelease(LC_MouseEvent *e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayDrawPreview(LC_MouseEvent *event, int status) override;
    void doPreparePreviewEntities(LC_MouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    int doGetStatusForInitialSnapToRelativeZero() override;
    void doInitialSnapToRelativeZero(RS_Vector vector) override;
    bool doProcessCommand(int status, const QString &c) override;
    void doBack(LC_MouseEvent *pEvent, int status) override;
    RS_Vector doGetMouseSnapPoint(LC_MouseEvent *e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
};
#endif // LC_ACTIONDRAWSTAR_H
