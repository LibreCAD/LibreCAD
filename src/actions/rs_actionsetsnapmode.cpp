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

#include "rs_actionsetsnapmode.h"

#include "rs_graphicview.h"


/**
 * Constructor.
 *
 * @param snapMode The new snap mode used from now on.
 */
RS_ActionSetSnapMode::RS_ActionSetSnapMode(RS_EntityContainer& container,
        RS_GraphicView& graphicView,
        RS2::SnapMode snapMode)
        :RS_ActionInterface("Set Snap Mode", container, graphicView) {

    this->snapMode = snapMode;
}



void RS_ActionSetSnapMode::trigger() {
    RS_SnapMode s = graphicView->getDefaultSnapMode();

    switch (snapMode) {

        case RS2::SnapFree:
            s.clear();
            break;

        case RS2::SnapCenter:
            s.snapCenter = !s.snapCenter;
            break;

        case RS2::SnapDist:
            s.snapDistance = !s.snapDistance;
            break;

        case RS2::SnapEndpoint:
            s.snapEndpoint = !s.snapEndpoint;
            break;

        case RS2::SnapGrid:
            s.snapGrid = !s.snapGrid;
            break;

        case RS2::SnapOnEntity:
            s.snapOnEntity = !s.snapOnEntity;
            break;

        case RS2::SnapMiddle:
            s.snapMiddle = !s.snapMiddle;
            break;

        case RS2::SnapIntersection:
            s.snapIntersection = !s.snapIntersection;
            break;

        default:
            break;
    }

    graphicView->setDefaultSnapMode(s);

    finish(false);
}



void RS_ActionSetSnapMode::init(int status) {
    RS_ActionInterface::init(status);
    trigger();
}

// EOF
