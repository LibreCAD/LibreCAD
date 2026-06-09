/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef RS_ACTIONDRAWARCTANGENTIAL_H
#define RS_ACTIONDRAWARCTANGENTIAL_H

#include "lc_undoabledocumentmodificationaction.h"

class RS_AtomicEntity;
struct RS_ArcData;

/**
 * This action class can handle user events to draw 
 * arcs with three points given.
 *
 * @author Andrew Mustun
 */
class LC_ActionDrawArcTangential:public LC_SingleEntityCreationAction {
    Q_OBJECT
public:
    explicit LC_ActionDrawArcTangential(LC_ActionContext *actionContext);
    ~LC_ActionDrawArcTangential() override;
    void reset();
    void preparePreview();
    void setRadius(double r);
    double getRadius() const;
    void setAngle(double angle);
    double getAngle() const;
    void setByRadius(bool byRadius = true);
    bool isByRadius() const;
protected:
    /**
 * Action States.
 */
    enum Status {
        SetBaseEntity = InitialActionStatus,   /**< Setting base entity. */
        SetEndAngle                            /**< Setting end angle. */
    };

    /**
     * Base entity.
     */
    RS_AtomicEntity *m_baseEntity = nullptr;
    /**
  * Start point of base entity clicked?
  */
    bool m_isStartPoint = false;
    /**
     * Point that determines end angle.
     */
    RS_Vector m_point;

    RS_Vector m_arcStartPoint;
    /**
  * Arc data calculated.
  */
    std::unique_ptr<RS_ArcData> m_arcData;

    double m_angleLength = 0.;
    bool m_byRadius = false;
    bool m_alternateArc = false;
    double m_radius = 1.0;
    double m_angle = 90.0;
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPosition) override;
    RS_Vector forecastArcCenter() const;
    void setBaseEntity(RS_Entity* entity, const RS_Vector& coord);
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void updateActionPrompt() override;
    void doTriggerCompletion(bool success) override;
    RS_Entity* doTriggerCreateEntity() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angle) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
