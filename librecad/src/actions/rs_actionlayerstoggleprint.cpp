/****************************************************************************
**
 * Toggle whether a layer should appear on print (a construction layer doesn't appear on
 printout, and have straight lines of infinite length)

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/


#include "rs_actionlayerstoggleprint.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_debug.h"

/**
 * whether a layer should appear on print (a construction layer doesn't appear on
 printout, and have straight lines of infinite length)
 *
 * @author Dongxu Li
 */
RS_ActionLayersTogglePrint::RS_ActionLayersTogglePrint(
    RS_EntityContainer& container,
    RS_GraphicView& graphicView,
    RS_Layer* layer)
        : RS_ActionInterface("Toggle Layer Printing", container, graphicView)
        , a_layer(layer)
{}

void RS_ActionLayersTogglePrint::trigger() {
    RS_DEBUG->print("toggle layer printing");
    if (graphic) {
        if (a_layer) {
            graphic->toggleLayerPrint(a_layer);

			// deselect entities on locked layer:
			for(auto e: *container){
                if (e && e->isVisible() && e->getLayer()==a_layer) {

                    if (graphicView) {
                        graphicView->deleteEntity(e);
                    }

                    if (graphicView) {
                        graphicView->drawEntity(e);
                    }
                }
            }
        }

    }
    finish(false);
}



void RS_ActionLayersTogglePrint::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
