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

#include "rs_actionlayersfreezeall.h"

#include <QAction>
#include "rs_graphic.h"



RS_ActionLayersFreezeAll::RS_ActionLayersFreezeAll(bool freeze,
        RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_ActionInterface("Freeze all Layers",
                    container, graphicView) {

    this->freeze = freeze;
}

QAction* RS_ActionLayersFreezeAll::createGUIAction(RS2::ActionType type, QObject* /*parent*/) {
    QAction* action=NULL;
	
	if (type==RS2::ActionLayersFreezeAll) {
		// tr("Freeze all")
		action = new QAction(tr("&Freeze all"),  NULL);
    	//action->zetStatusTip(tr("Freeze all layers"));
		action->setIcon(QIcon(":/ui/hiddenblock.png"));
	}
	else if (type==RS2::ActionLayersDefreezeAll) {
		// tr("Defreeze all")
        action = new QAction(tr("&Defreeze all"),   NULL);
        //action->zetStatusTip(tr("Defreeze all layers"));
		action->setIcon(QIcon(":/ui/visibleblock.png"));
	}
    return action;
}


void RS_ActionLayersFreezeAll::trigger() {
    RS_DEBUG->print("RS_ActionLayersFreezeAll::trigger");
    if (graphic!=NULL) {
        //RS_Layer* layer = graphic->getActiveLayer();
        graphic->freezeAllLayers(freeze);
    }
    finish();
}



void RS_ActionLayersFreezeAll::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
