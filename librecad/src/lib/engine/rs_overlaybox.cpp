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

#include<iostream>
#include "rs_overlaybox.h"

#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_graphic.h"
#include <QBrush>

/**
 * Constructor.
 */
RS_OverlayBox::RS_OverlayBox(RS_EntityContainer* parent,
                             const RS_OverlayBoxData& d)
    :RS_AtomicEntity(parent), data(d) {
}

RS_Entity* RS_OverlayBox::clone() const{
    auto* l = new RS_OverlayBox(*this);
    l->initId();
    return l;
}

void RS_OverlayBox::draw(RS_Painter* painter, RS_GraphicView* view, double& /*patternOffset*/) {

    RS_Vector v1=view->toGui(getCorner1());
    RS_Vector v2=view->toGui(getCorner2());

    QRectF selectRect(
        v1.x,
        v1.y,
        v2.x - v1.x,
        v2.y - v1.y);

    if (v1.x > v2.x) {
        RS_Pen p(view->getOverlayBoxLineInvertedColor(), RS2::Width00, RS2::DashLine);
        painter->setPen(p);
        const RS_Color &fillColor = view->getOverlayBoxFillInvertedColor();
        painter->fillRect(selectRect, fillColor);
    }
    else {
        painter->setPen(view->getOverlayBoxLineColor());
        const RS_Color &fillColor = view->getOverlayBoxFillColor();
        painter->fillRect(selectRect, fillColor);
    }

    painter->drawRect(v1, v2);
}

/**
 * Dumps the point's data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_OverlayBox& l) {
    os << " Line: " << l.getData() << "\n";
    return os;
}

std::ostream& operator << (std::ostream& os, const RS_OverlayBoxData& ld) {
    os << "(" << ld.corner1 <<
       "/" << ld.corner2 <<
       ")";
    return os;
}
