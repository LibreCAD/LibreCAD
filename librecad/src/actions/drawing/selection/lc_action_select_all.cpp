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

#include "lc_action_select_all.h"

#include "rs_selection.h"

LC_ActionSelectAll::LC_ActionSelectAll(LC_ActionContext* actionContext, const bool select)
    : RS_ActionInterface("ActionSelectAll", actionContext, RS2::ActionSelectAll), m_select(select) {
    m_actionType = RS2::ActionSelectAll;
}

void LC_ActionSelectAll::init(const int status) {
    RS_ActionInterface::init(status);
    trigger();
    finish();
}

void LC_ActionSelectAll::trigger() {
    const RS_Selection s(m_document, m_viewport);
    s.selectAll(m_select);
}
