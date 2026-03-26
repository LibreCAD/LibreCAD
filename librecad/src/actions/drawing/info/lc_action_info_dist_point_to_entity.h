/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#ifndef RS_ACTIONINFODIST2_H
#define RS_ACTIONINFODIST2_H

#include "rs_previewactioninterface.h"

/**
 * This action class can handle user events to measure distances between
 * entities and points.
 *
 * @author Andrew Mustun
 */
class LC_ActionInfoDistPointToEntity:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    explicit LC_ActionInfoDistPointToEntity(LC_ActionContext *actionContext,bool fromPointToEntity = false);
    ~LC_ActionInfoDistPointToEntity() override;
    void init(int status) override;
    bool isUseNearestPointOnEntity() const {return m_nearestPointShouldBeOnEntity;}
    void setUseNearestPointOnEntity(const bool value){m_nearestPointShouldBeOnEntity = value;}
    void finish() override;
protected:
    /**
   * Action States.
   */
    enum Status {
        SetEntity = InitialActionStatus,    /**< Setting the entity. */
        SetPoint      /**< Setting the point of the distance. */
    };

    enum Mode{
        FIRST_IS_ENTITY,
        FIRST_IS_POINT
    };
    RS_Entity *m_entity = nullptr;
    RS_Vector m_point = RS_Vector(false);
    int m_selectionMode = FIRST_IS_ENTITY;
    bool m_nearestPointShouldBeOnEntity = true;
    RS_Vector m_savedRelZero = RS_Vector{false};
    RS_Vector m_entityNearestPoint = RS_Vector{false};

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;

    RS_Entity *doCatchEntity(const LC_MouseEvent *e, bool preview) const;
    void restoreRelZero();
    RS_Vector obtainNearestPointOnEntity(const RS_Vector &snap) const;
    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void updateActionPrompt() override;
    void updateInfoCursor(const RS_Vector &mouse, const RS_Vector &startPoint) const;
    void doTrigger() override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};

#endif
