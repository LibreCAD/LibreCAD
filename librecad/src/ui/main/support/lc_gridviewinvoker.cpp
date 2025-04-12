/*******************************************************************************
*
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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
 ******************************************************************************/
#include "lc_gridviewinvoker.h"

#include "lc_graphicviewport.h"
#include "qc_applicationwindow.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"

LC_GridViewInvoker::LC_GridViewInvoker(QC_ApplicationWindow* mainWin):LC_AppWindowAware{mainWin} {
}

void LC_GridViewInvoker::setGridView(bool toggle, bool isometric, RS2::IsoGridViewType isoGridType) const {
    if (toggle) {
        RS_GraphicView *view = m_appWin->getCurrentGraphicView();
        if (view != nullptr) {
            if (!view->isPrintPreview()) {
                RS_Graphic *graphic = view->getGraphic();
                graphic->setIsometricGrid(isometric);
                if (isometric) {
                    graphic->setIsoView(isoGridType);
                }
                LC_GraphicViewport* viewport = view->getViewPort();
                viewport->loadGridSettings();
                updateGridViewActions(isometric, isoGridType);
                view->redraw();
                view->update();
            }
        }
    }
}

void LC_GridViewInvoker::updateGridViewActions(bool isometric, RS2::IsoGridViewType type) const{
    bool viewOrtho = false, viewIsoLeft = false, viewIsoRight = false, viewIsoTop = false;

    if (isometric){
        switch (type){
            case RS2::IsoLeft:{
                viewIsoLeft = true;
                break;
            }
            case RS2::IsoTop:{
                viewIsoTop = true;
                break;
            }
            case RS2::IsoRight:{
                viewIsoRight = true;
            }
            default:
                break;
        }
    }
    else{
        viewOrtho = true;
    }

    m_appWin->getAction("ViewGridOrtho")->setChecked(viewOrtho);
    m_appWin->getAction("ViewGridIsoLeft")->setChecked(viewIsoLeft);
    m_appWin->getAction("ViewGridIsoTop")->setChecked(viewIsoTop);
    m_appWin->getAction("ViewGridIsoRight")->setChecked(viewIsoRight);
}
