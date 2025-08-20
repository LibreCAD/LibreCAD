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

#ifndef RS_ACTIONEDITCOPY_H
#define RS_ACTIONEDITCOPY_H

#include "lc_actionpreselectionawarebase.h"

// fixme - add disabling of paste actions if clipboard is empty!
/**
 * This action class can handle user events for copying,cutting and pasting entities
 * to the clipboard.
 *
 * @author Andrew Mustun
 */
class RS_ActionEditCopyPaste :public LC_ActionPreSelectionAwareBase {
    Q_OBJECT
public:
    RS_ActionEditCopyPaste(LC_ActionContext *actionContext, RS2::ActionType actionType);
    ~RS_ActionEditCopyPaste() override;
    void init(int status) override;
protected:
    /**
  * Action States.
  */
    enum Status {
        SetReferencePoint = InitialActionStatus    /**< Setting the reference point. */
    };

    bool m_invokedWithControl = false;
    std::unique_ptr<RS_Vector> m_referencePoint;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void doTrigger(bool keepSelected) override;
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
    void onMouseMoveEventSelected(int status, LC_MouseEvent *e) override;
    void updateMouseButtonHintsForSelection() override;
    void updateMouseButtonHintsForSelected(int status) override;
    void onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *pEvent) override;
    void onMouseRightButtonReleaseSelected(int status, LC_MouseEvent *pEvent) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
};
#endif
