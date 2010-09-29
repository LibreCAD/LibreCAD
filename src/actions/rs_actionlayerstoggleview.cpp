/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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

#include "rs_actionlayerstoggleview.h"

#include "rs_graphic.h"



RS_ActionLayersToggleView::RS_ActionLayersToggleView(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView)
        :RS_ActionInterface("Toggle Layer Visibility",
                    container, graphicView) {}


QAction* RS_ActionLayersToggleView::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Toggle Layer Visibility")
    QAction* action = new QAction(tr("&Toggle Layer"), NULL);
    //action->zetStatusTip(tr("Toggle Layer"));
	action->setIcon(QIcon(":/ui/layertoggle.png"));
    return action;
}

void RS_ActionLayersToggleView::trigger() {
    RS_DEBUG->print("toggle layer");
    if (graphic!=NULL) {
        RS_Layer* layer = graphic->getActiveLayer();
        graphic->toggleLayer(layer);
    }
    finish();
}



void RS_ActionLayersToggleView::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
