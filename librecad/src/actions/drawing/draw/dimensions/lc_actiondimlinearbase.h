/****************************************************************************
**
* Abstract base class for linear dimensions

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

#ifndef LC_ACTIONDIMLINEARBASE_H
#define LC_ACTIONDIMLINEARBASE_H

#include "rs_actiondimension.h"

class LC_ActionDimLinearBase : public RS_ActionDimension {
    Q_OBJECT
protected:
    /**
   * Action States.
   */
    enum Status {
        SetExtPoint1 = InitialActionStatus, /**< Setting the 1st ext point.  */
        SetExtPoint2,                       /**< Setting the 2nd ext point. */
        SetDefPoint,                        /**< Setting the common def point. */
        SetText,                            /**< Setting the text label in the command line. */
        SetAngle                            /**< Setting the angle in the command line. */
    };

    enum ActionMode {
        NORMAL,
        BASELINE,
        CONTINUE
    };

    ActionMode m_actionMode = NORMAL;

    bool m_alternateDimDirection = false;

    LC_ActionDimLinearBase(const char* name, LC_ActionContext* actionContext, RS2::EntityType dimType,
                               RS2::ActionType actionType = RS2::ActionNone);
    ~LC_ActionDimLinearBase() override;
    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    virtual RS_Vector getExtensionPoint1() = 0;
    virtual void setExtensionPoint1(const RS_Vector& p) = 0;
    virtual void setExtensionPoint2(const RS_Vector& p) = 0;
    virtual RS_Vector getExtensionPoint2() = 0;
    virtual void preparePreview(bool alternateMode) = 0;
    virtual double getDimAngle(bool alternateMode) = 0;
    RS_Vector getAdjacentDimDimSnapPoint(const RS_Vector& ownDimPointToCheck, double snapRange) const;
    RS_Vector adjustDefPointByAdjacentDims(const RS_Vector& mouse, const RS_Vector& extPoint1,
                                           const RS_Vector& extPoint2, double ownDimLineAngle, bool forPreview);
    RS_Vector adjustByAdjacentDim(const RS_Vector& mouse, bool forPreview, bool altDirection);
    virtual void updateMouseButtonHintForExtPoint2();
    virtual void updateMouseButtonHintForDefPoint();
    virtual RS_Entity* createDim(RS_EntityContainer* parent) = 0;
    void onMouseLeftButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonRelease(int status, const LC_MouseEvent* e) override;
    void onMouseMoveEvent(int status, const LC_MouseEvent* e) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector& pos) override;
    void updateActionPrompt() override;
    RS_Entity* doTriggerCreateEntity() override;
};
#endif
