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
#include "rs_actiontoolregeneratedimensions.h"

#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_information.h"

// fixme - review
RS_ActionToolRegenerateDimensions::RS_ActionToolRegenerateDimensions(LC_ActionContext *actionContext)
        :RS_ActionInterface("Tool Regen Dim", actionContext, RS2::ActionToolRegenerateDimensions) {}

void RS_ActionToolRegenerateDimensions::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionToolRegenerateDimensions::trigger() {
    RS_DEBUG->print("RS_ActionToolRegenerateDimensions::trigger()");

    int num = 0;
    for(auto e: *m_container){ // fixme - iteration over all entities in container

        if (RS_Information::isDimension(e->rtti()) && e->isVisible()) {
            num++;
            if (((RS_Dimension*)e)->getLabel()==";;") {
                ((RS_Dimension*)e)->setLabel("");
            }
            ((RS_Dimension*)e)->updateDim(true);
        }
    }

    if (num>0) {
        redrawDrawing();
        commandMessage(tr("Regenerated %1 dimension entities").arg(num));
    } else {
        commandMessage(tr("No dimension entities found"));
    }
    finish(false);
}
