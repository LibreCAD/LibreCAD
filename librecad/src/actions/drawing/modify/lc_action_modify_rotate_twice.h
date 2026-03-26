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

#ifndef RS_ACTIONMODIFYROTATE2_H
#define RS_ACTIONMODIFYROTATE2_H


#include "lc_action_modify_base.h"

struct RS_Rotate2Data;
/**
 * This action class can handle user events to rotate entities around
 * two entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionModifyRotateTwice : public LC_ActionModifyBase {
    Q_OBJECT
public:
    explicit LC_ActionModifyRotateTwice(LC_ActionContext *actionContext);
    ~LC_ActionModifyRotateTwice() override;
    void init(int status) override;
    void setAngle1(double angleRad) const;
    double getAngle1() const;
    void setAngle2(double angleRad) const;
    double getAngle2() const;
    void setUseSameAngle2ForCopies(bool b) const;
    bool isUseSameAngle2ForCopies() const;
    bool isMirrorAngles() const;
    void setMirrorAngles(bool b) const;
protected:
    /**
     * Action States.
     */
    enum Status {
        SetReferencePoint1,    /**< Setting the reference point. */
        SetReferencePoint2,    /**< Setting the target point. */
        ShowDialog             /**< Showing the options dialog. */
    };

    std::unique_ptr<RS_Rotate2Data> m_actionData;
    void previewRefPointsForMultipleCopies(const RS_Vector& mouse) const;
    void doPerformTrigger();
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* e) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    bool isAllowTriggerOnEmptySelection() override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    bool doUpdateAngleByInteractiveInput(const QString& tag, double angleRad) override;
    void doTriggerCompletion(bool success) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& ctx) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
