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
#include "lc_graphicviewport.h"

/**
 * Default constructor.
 *
 * @param direction In for zooming in, Out for zooming out.
 * @param axis Axis that are affected by the zoom (OnlyX, OnlyY or Both)
 */
RS_ActionZoomIn::RS_ActionZoomIn(LC_ActionContext *actionContext,
                                 RS2::ZoomDirection direction,
                                 RS2::Axis axis,
								 RS_Vector const* pCenter,
                                 double factor)
            :RS_ActionInterface("Zoom in", actionContext, RS2::ActionZoomIn)
        ,m_zoomFactor(factor)
        ,m_direction(direction)
        ,m_axis(axis)
		,m_centerPoint(pCenter?new RS_Vector{*pCenter}:new RS_Vector{}){;
}

RS_ActionZoomIn::~RS_ActionZoomIn() = default;

void RS_ActionZoomIn::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

void RS_ActionZoomIn::trigger() {
    switch (m_axis) {
        // fixme - sand - review and remove this if not needed...
        case RS2::OnlyX:
    /*        if (direction==RS2::In) {
                viewport->zoomInX();
            } else {
                viewport->zoomOutX();
            }*/
            break;

        case RS2::OnlyY:
           /* if (direction==RS2::In) {
                viewport->zoomInY();
            } else {
                viewport->zoomOutY();
            }*/
            break;

        case RS2::Both:
            if (m_direction==RS2::In) {
                m_viewport->zoomIn(m_zoomFactor, *m_centerPoint);
            } else {
                m_viewport->zoomOut(m_zoomFactor, *m_centerPoint);
            }
            break;
    }
    finish(false);
}
