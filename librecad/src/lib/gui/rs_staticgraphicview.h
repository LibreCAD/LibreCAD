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

#ifndef RS_STATICGRAPHICVIEW_H
#define RS_STATICGRAPHICVIEW_H

#include "rs_graphicview.h"
//#include "rs_layerlistlistener.h"

/**
 * This is an implementation of a graphic viewer with a fixed size
 * for drawing onto fixed devices (such as bitmaps).
 */
class RS_StaticGraphicView: public RS_GraphicView {
public:
	RS_StaticGraphicView(int w, int h, RS_Painter* p, QSize const* pb = nullptr);

	int getWidth() const override;
	int getHeight() const override;
	void redraw(RS2::RedrawMethod) override{}
	void adjustOffsetControls() override{}
	void adjustZoomControls() override{}
	void setMouseCursor(RS2::CursorType ) override{}

	void updateGridStatusWidget(const QString& ) override{}
	RS_Vector getMousePosition() const override;

    void paint();

private:
    //! Width
    int width;

    //! Height
    int height;
};

#endif
