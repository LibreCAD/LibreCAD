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

#include "rs_actionzoomin.h"

#include <QAction>
#include "rs_graphicview.h"

/**
 * Default constructor.
 *
 * @param direction In for zooming in, Out for zooming out.
 * @param axis Axis that are affected by the zoom (OnlyX, OnlyY or Both)
 */
RS_ActionZoomIn::RS_ActionZoomIn(RS_EntityContainer& container,
                                 RS_GraphicView& graphicView,
                                 RS2::ZoomDirection direction,
                                 RS2::Axis axis,
								 RS_Vector const* pCenter,
                                 double factor)
        :RS_ActionInterface("Zoom in", container, graphicView)
        ,zoom_factor(factor)
        ,direction(direction)
        ,axis(axis)
		,center(pCenter?new RS_Vector{*pCenter}:new RS_Vector{})
{
}

RS_ActionZoomIn::~RS_ActionZoomIn() = default;

void RS_ActionZoomIn::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionZoomIn::trigger() {
    switch (axis) {
    case RS2::OnlyX:
        if (direction==RS2::In) {
            graphicView->zoomInX();
        } else {
            graphicView->zoomOutX();
        }
        break;

    case RS2::OnlyY:
        if (direction==RS2::In) {
            graphicView->zoomInY();
        } else {
            graphicView->zoomOutY();
        }
        break;

    case RS2::Both:
        if (direction==RS2::In) {
			graphicView->zoomIn(zoom_factor, *center);
        } else {
			graphicView->zoomOut(zoom_factor, *center);
        }
        break;
    }
    finish(false);
}

// EOF
