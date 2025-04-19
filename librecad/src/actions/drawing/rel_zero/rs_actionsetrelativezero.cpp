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

#include "rs_actionsetrelativezero.h"

#include "lc_graphicviewport.h"
#include "rs_document.h"

RS_ActionSetRelativeZero::RS_ActionSetRelativeZero(LC_ActionContext *actionContext)
    : RS_PreviewActionInterface("Set the relative Zero",actionContext, RS2::ActionSetRelativeZero)
    , m_position(std::make_unique<RS_Vector>()){
}

RS_ActionSetRelativeZero::~RS_ActionSetRelativeZero() = default;

void RS_ActionSetRelativeZero::trigger(){
    bool wasLocked = m_viewport->isRelativeZeroLocked();
    if (m_position->valid) {
        m_viewport->lockRelativeZero(false);
        moveRelativeZero(*m_position);
        undoCycleStart();
        RS_Undoable *relativeZeroUndoable = m_viewport->getRelativeZeroUndoable();
        if (relativeZeroUndoable != nullptr) {
            m_document->addUndoable(relativeZeroUndoable);
        }
        undoCycleEnd();
        m_viewport->lockRelativeZero(wasLocked);
    }
    finish(false);
}

void RS_ActionSetRelativeZero::onMouseLeftButtonRelease([[maybe_unused]]int status, LC_MouseEvent *e) {
    fireCoordinateEventForSnap(e);
}

void RS_ActionSetRelativeZero::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    initPrevious(status);
}

void RS_ActionSetRelativeZero::onCoordinateEvent( [[maybe_unused]]int status, [[maybe_unused]]bool isZero, const RS_Vector &pos) {
    *m_position = pos;
    trigger();
    updateMouseButtonHints();
}

void RS_ActionSetRelativeZero::updateMouseButtonHints(){
    switch (getStatus()) {
        case 0:
            updateMouseWidgetTRCancel(tr("Set relative Zero"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

RS2::CursorType RS_ActionSetRelativeZero::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::CadCursor;
}
