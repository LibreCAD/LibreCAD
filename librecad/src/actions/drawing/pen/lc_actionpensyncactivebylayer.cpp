/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 sand1024

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
#include "rs_layer.h"
#include "qg_pentoolbar.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"
#include "lc_actionpensyncactivebylayer.h"

LC_ActionPenSyncActiveByLayer::LC_ActionPenSyncActiveByLayer(RS_EntityContainer &container, RS_GraphicView &graphicView)
   :RS_ActionInterface("PenSyncByLayer", container, graphicView){
}

void LC_ActionPenSyncActiveByLayer::init(int status){
    RS_ActionInterface::init(status);
    if (status >=0){
        QG_PenToolBar *penToolBar = QC_ApplicationWindow::getAppWindow()->getPenToolBar();
        if (penToolBar != nullptr){
            if (graphic != nullptr){
                RS_Layer *layer = graphic->getActiveLayer();
                if (layer != nullptr){
                    RS_Pen layerPen = layer->getPen();
                    penToolBar->setLayerLineType(layerPen.getLineType(), true);
                    penToolBar->setLayerWidth(layerPen.getWidth(), true);
                    penToolBar->setLayerColor(layerPen.getColor(), true);
                }
            }
        }
        RS_ActionInterface::finish(true);
        init(-1);
    }
}
