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

#ifndef RS_ACTIONMODIFYMOVEROTATE_H
#define RS_ACTIONMODIFYMOVEROTATE_H


#include "lc_action_modify_base.h"

struct RS_MoveRotateData;

/**
 * This action class can handle user events to move and at the same
 * time rotate entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionModifyMoveRotate : public LC_ActionModifyBase {
    Q_OBJECT
public:
    explicit LC_ActionModifyMoveRotate(LC_ActionContext *actionContext);
    ~LC_ActionModifyMoveRotate() override;
    QStringList getAvailableCommands() override;
    void setAngle(double angleRad) const;
    double getAngle() const;
    void setUseSameAngleForCopies(bool b) const;
    bool isUseSameAngleForCopies() const;
    void setAngleIsFree(bool b);
    bool isAngleFree() const {return !m_angleIsFixed;}
protected:
    /**
 * Action States.
 */
    enum Status {
        SetReferencePoint,    /**< Setting the reference point. */
        SetTargetPoint,       /**< Setting the target point. */
        SetAngle,              /**< Setting angle in command line. */
        ShowDialog           /**< Showing the options dialog. */
    };
    struct MoveRotateActionData;
    std::unique_ptr<MoveRotateActionData> m_actionData;
/** Last status before entering angle. */
    Status m_lastStatus = SetReferencePoint;
    bool m_angleIsFixed = true;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void previewRefPointsForMultipleCopies() const;
    void doPerformTrigger();
    bool doProcessCommand(int status, const QString &command) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* e) override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    void doTriggerCompletion(bool success) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& ctx) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doSaveOptions() override;
    void doLoadOptions() override;

    bool isInVisualSnapStatus(int status) override;
};
#endif
