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

#include "rs_actionlayerstogglelock.h"

#include <QAction>
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_debug.h"


RS_ActionLayersToggleLock::RS_ActionLayersToggleLock(
        RS_EntityContainer& container,
        RS_GraphicView& graphicView,
        RS_Layer* layer)
    : RS_ActionInterface("Toggle Layer Visibility" ,container, graphicView)
    , a_layer(layer)
{}

void RS_ActionLayersToggleLock::trigger() {
    RS_DEBUG->print("toggle layer");
    if (graphic) {
        if (a_layer) {
            graphic->toggleLayerLock(a_layer);

            // deselect entities on locked layer:
            if (a_layer->isLocked()) {
				for(auto e: *container){
                    if (e && e->isVisible() && e->getLayer()==a_layer) {

                        if (graphicView) {
                            graphicView->deleteEntity(e);
                        }

                        e->setSelected(false);

                        if (graphicView) {
                            graphicView->drawEntity(e);
                        }
                    }
                }
            }
        }

    }
    finish(false);
}

void RS_ActionLayersToggleLock::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
