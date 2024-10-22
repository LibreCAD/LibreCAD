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

#include <QDialog>
#include "rs_actionoptionsdrawing.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_debug.h"

RS_ActionOptionsDrawing::RS_ActionOptionsDrawing(RS_EntityContainer& container,
        RS_GraphicView& graphicView, int tabIndex)
        :RS_ActionInterface("Drawing Options",
                    container, graphicView) {
	actionType=RS2::ActionOptionsDrawing;
 tabToShow = tabIndex;
}

void RS_ActionOptionsDrawing::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionOptionsDrawing::trigger() {
    if (graphic != nullptr) {
        graphicView->setForcedActionKillAllowed(false);
        int dialogResult = RS_DIALOGFACTORY->requestOptionsDrawingDialog(*graphic,tabToShow);
        if (dialogResult == QDialog::Accepted){
            updateCoordinateWidgetFormat();
            if (graphicView != nullptr) {
                graphicView->loadSettings();
                graphicView->redraw(RS2::RedrawAll);
                graphicView->repaint();
            }
            else{
            }
        }
        graphicView->setForcedActionKillAllowed(true);
    }
    finish(false);
}
