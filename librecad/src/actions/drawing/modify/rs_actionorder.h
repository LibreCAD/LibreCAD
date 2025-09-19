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
#ifndef RS_ACTIONORDER_H
#define RS_ACTIONORDER_H

#include "lc_actionpreselectionawarebase.h"

/**
 * This action class can handle user events to move entities.
 *
 * @author Rallaz
 */
class RS_ActionOrder : public LC_ActionPreSelectionAwareBase {
    Q_OBJECT
public:
    RS_ActionOrder(LC_ActionContext *actionContext, RS2::ActionType type);
    void drawSnapper() override;
protected:
    RS_Entity* targetEntity = nullptr;
    void onMouseLeftButtonReleaseSelected(int status, LC_MouseEvent *pEvent) override;
    void onMouseRightButtonReleaseSelected(int status, LC_MouseEvent *pEvent) override;
    void updateMouseButtonHintsForSelected(int status) override;
    RS2::CursorType doGetMouseCursorSelected(int status) override;
    void updateMouseButtonHintsForSelection() override;
    void onSelectionCompleted(bool singleEntity, bool fromInit) override;
    void doTrigger(bool keepSelected) override;
    void onMouseMoveEventSelected(int status, LC_MouseEvent *e) override;
};

#endif
