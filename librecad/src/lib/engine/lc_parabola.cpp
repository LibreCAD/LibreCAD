/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2014 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2014 Pevel Krejcir (pavel@pamsoft.cz)

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

#include <QPainterPath>
#include <QPolygonF>
#include "lc_parabola.h"

#include "rs_circle.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_graphicview.h"
#include "rs_painter.h"
#include "rs_graphic.h"
#include "rs_painterqt.h"
#include "lc_quadratic.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_linetypepattern.h"

namespace {
LC_SplinePointsData convert2SplineData(const LC_ParabolaData& data)
{
    LC_SplinePointsData splineData{};
    RS_Line tangent0{nullptr, {data.startPoint, data.startPoint + data.startTangent}};
    RS_Line tangent1{nullptr, {data.endPoint, data.endPoint + data.endTangent}};
    RS_VectorSolutions intersection = RS_Information::getIntersectionLineLine(&tangent0, &tangent1);
    if (intersection.empty()) {
        assert(!"tangent direction cannot be parallel");
    }
    RS_Vector pointP1 = (data.startPoint + data.endPoint) * 0.25 + intersection.at(0) * 0.5;
    splineData.splinePoints = {{data.startPoint, pointP1, data.endPoint}};
    splineData.closed = false;
    return splineData;
}

}

LC_Parabola::LC_Parabola(RS_EntityContainer* parent, const LC_ParabolaData& d):
    LC_SplinePoints{parent, convert2SplineData(d)}
  , data{d}
{}

RS2::EntityType LC_Parabola::rtti() const
{
    return RS2::EntityParabola;
}



