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

#ifndef RS_ACTIONMODIFYMIRROR_H
#define RS_ACTIONMODIFYMIRROR_H

#include "lc_action_modify_base.h"

struct RS_MirrorData;

/**
 * This action class can handle user events to mirror entities.
 *
 * @author Andrew Mustun
 */
class LC_ActionModifyMirror:public LC_ActionModifyBase {
    Q_OBJECT
public:
    explicit LC_ActionModifyMirror(LC_ActionContext *actionContext);
    ~LC_ActionModifyMirror() override;
    bool isMirrorToExistingLine() const {return m_mirrorToExistingLine;}
    void setMirrorToExistingLine(bool value);
protected:
    /**
    * Action States.
    */
    enum Status {
        SetAxisPoint1,    /**< Setting the 1st point of the axis. */
        SetAxisPoint2,    /**< Setting the 2nd point of the axis. */
        ShowDialog,        /**< Showing the options dialog. */
    };
    struct MirrorActionData;
    std::unique_ptr<MirrorActionData> m_actionData;
    bool m_mirrorToExistingLine;
    void previewMirror(const RS_Vector &mirrorLinePoint1, const RS_Vector &mirrorLinePoint2) const;
    void showOptionsAndTrigger();
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void onMouseLeftButtonReleaseSelected(int status, const LC_MouseEvent* e) override;
    void onMouseRightButtonReleaseSelected(int status, const LC_MouseEvent* event) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    LC_ModifyOperationFlags *getModifyOperationFlags() override;
    void doPerformTrigger();
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &coord) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void obtainFlipLineCoordinates(RS_Vector *start, RS_Vector *end, bool verticalLine) const;
    void onMouseMoveEventSelected(int status, const LC_MouseEvent* e) override;
    void doTriggerSelectionUpdate(bool keepSelected, const LC_DocumentModificationBatch& ctx) override;
    bool doTriggerModifications(LC_DocumentModificationBatch& ctx) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};
#endif
