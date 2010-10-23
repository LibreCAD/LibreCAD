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

#include "rs_staticgraphicview.h"

#include "rs_graphic.h"


/**
 * Constructor.
 *
 * @param w Width
 * @param h Height
 */
RS_StaticGraphicView::RS_StaticGraphicView(int w, int h, RS_Painter* p) {
    setBackground(RS_Color(255,255,255));
    width = w;
    height = h;
    painter = p;
    setBorders(5, 5, 5, 5);
}


/**
 * Destructor
 */
RS_StaticGraphicView::~RS_StaticGraphicView() {}


/**
 * @return width of widget.
 */
int RS_StaticGraphicView::getWidth() {
    return width;
}


/**
 * @return height of widget.
 */
int RS_StaticGraphicView::getHeight() {
    return height;
}





/**
 * Handles paint events by redrawing the graphic in this view.
 */
void RS_StaticGraphicView::paint() {
    RS_DEBUG->print("RS_StaticGraphicView::paint begin");
    // drawIt();
    redraw(RS2::RedrawAll);
    RS_DEBUG->print("RS_StaticGraphicView::paint end");
}

