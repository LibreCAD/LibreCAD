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

#ifndef RS_ACTIONMODIFYROTATE_H
#define RS_ACTIONMODIFYROTATE_H

#include "lc_action_modify_base.h"

struct RS_RotateData;

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionModifyRotate: public LC_ActionModifyBase {
    Q_OBJECT
public:
    explicit LC_ActionModifyRotate(LC_ActionContext *actionContext);
    ~LC_ActionModifyRotate() override;
    void init(int status) override;
    double getAngle() const;
    void setAngle(double angleRad) const;
    void setFreeAngle(bool enable);
    void setRelativeAngle(bool enable);
    bool isFreeAngle() const {return m_freeAngle;}
    bool isRelativeAngle() const {return m_relativeAngle;}
    double getRefPointAngle() const;
    void setRefPointAngle(double angle) const;
    void setFreeRefPointAngle(bool value);
    bool isFreeRefPointAngle() const{return m_freeRefPointAngle;}
    bool isRefPointAngleAbsolute() const;
    void setRefPointAngleAbsolute(bool val) const;
    bool isRotateAlsoAroundReferencePoint() const;
    void setRotateAlsoAroundReferencePoint(bool value) const;
    double getCurrentAngleDegrees() const;
    double getCurrentAngle2Degrees() const;
    void keyPressEvent(QKeyEvent *e) override;
    void setCenterPointFirst(bool val);
    bool isCenterPointFirst()const {return !m_selectRefPointFirst;}
protected:
    /**
     * Action States.
     */
    enum Status {
        SetReferencePoint,    /**< Setting the reference point. */
        SetCenterPoint,    /**< Setting the rotation center */
        SetTargetPoint,    /**< Setting the target to rotation to*/
        SetTargetPoint2ndRotation,    /**< Setting the target to rotation around ref point*/
    };
    // support of old mode, most probably it should be removed and one selection mode should remain
    bool m_selectRefPointFirst = true;
    bool m_freeAngle = false;
    bool m_relativeAngle = false;
    bool m_freeRefPointAngle = false;
    std::unique_ptr<RS_RotateData> m_rotateData;
    double m_currentAngle = 0.0;
    double m_currentAngle2 = 0.0;

    void previewRotationCircleAndPoints(const RS_Vector &center,const RS_Vector &refPoint, double angle) const;
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void updateActionPromptForSelection() override;
    void updateActionPromptForSelected(int status) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
    void tryTrigger();
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* e) override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    void doTriggerCompletion(bool success) override;
    void previewRotatedEntities(const RS_RotateData& rotateData) const;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& ctx) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};

#endif
