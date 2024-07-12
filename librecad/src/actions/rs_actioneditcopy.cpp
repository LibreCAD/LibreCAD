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


#include <QAction>
#include <QMouseEvent>

#include "rs_actioneditcopy.h"
#include "rs_coordinateevent.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"

/**
 * Constructor.
 *
 * @param undo true for undo and false for redo.
 */
RS_ActionEditCopy::RS_ActionEditCopy(bool copy,
                                     RS_EntityContainer& container,
                                     RS_GraphicView& graphicView)
        :RS_ActionInterface("Edit Copy",
					container, graphicView)
		, copy{copy}
		, referencePoint{new RS_Vector{}}{
}

RS_ActionEditCopy::~RS_ActionEditCopy() = default;

void RS_ActionEditCopy::init(int status) {
    RS_ActionInterface::init(status);
    //trigger();
}

void RS_ActionEditCopy::trigger() {

    RS_Modification m(*container, graphicView);
    m.copy(*referencePoint, !copy);

    //graphicView->redraw();
    finish(false);
    graphicView->killSelectActions();
    //init(getStatus()-1);
    updateSelectionWidget();
}

void RS_ActionEditCopy::mouseMoveEvent(QMouseEvent* e) {
	if (getStatus()==SetReferencePoint)
		(void) snapPoint(e);
	else
		deleteSnapper();
}

void RS_ActionEditCopy::mouseLeftButtonReleaseEvent([[maybe_unused]]int status, QMouseEvent *e) {
    RS_CoordinateEvent ce(snapPoint(e));
    coordinateEvent(&ce);
}

void RS_ActionEditCopy::mouseRightButtonReleaseEvent(int status, [[maybe_unused]]QMouseEvent *e) {
    init(status-1);
}

void RS_ActionEditCopy::coordinateEvent(RS_CoordinateEvent* e) {
    if (e == nullptr)
        return;

    *referencePoint = e->getCoordinate();
    trigger();
}

void RS_ActionEditCopy::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetReferencePoint:
        updateMouseWidgetTRCancel("Specify reference point");
        break;
    default:
        updateMouseWidget();
        break;
    }
}

RS2::CursorType RS_ActionEditCopy::doGetMouseCursor([[maybe_unused]]int status){
    return RS2::CadCursor;
}
// EOF
