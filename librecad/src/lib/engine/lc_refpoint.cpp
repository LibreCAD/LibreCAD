/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "rs_painter.h"
#include "rs_graphicview.h"
#include "lc_refpoint.h"

LC_RefPoint::LC_RefPoint(RS_EntityContainer* parent,
                        const RS_Vector & d,
                        double size, int mode)
    :RS_Point(parent, RS_PointData(d)), pdmode{mode},pdsize{size}  {
    calculateBorders ();
}

RS_Entity* LC_RefPoint::clone() const {
    auto* p = new LC_RefPoint(*this);
    p->initId();
    return p;
}

RS2::EntityType LC_RefPoint::rtti() const{
    return RS2::EntityRefPoint;
}

void LC_RefPoint::draw(RS_Painter *painter, RS_GraphicView *view, [[maybe_unused]]double &patternOffset){
    if (painter == nullptr || view == nullptr){
        return;
    }

    int screenPDSize = determinePointSreenSize(painter, view, pdsize);

//		RS_DEBUG->print(RS_Debug::D_ERROR,"RS_Point::draw X = %f, Y = %f, PDMODE = %d, PDSIZE = %f, ScreenPDSize = %i",guiPos.x,guiPos.y,pdmode,pdsize,screenPDSize);
        RS_Vector guiPos = view->toGui(getPos());

     painter->drawPoint(guiPos, pdmode, screenPDSize);

}
