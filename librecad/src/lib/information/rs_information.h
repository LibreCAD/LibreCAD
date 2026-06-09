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

#ifndef RS_INFORMATION_H
#define RS_INFORMATION_H

#include "rs.h"

class RS_Ellipse;
class RS_Entity;
class RS_EntityContainer;
class RS_Vector;
class RS_VectorSolutions;
class RS_Arc;
class RS_Circle;
class RS_Line;

/**
 * Class for getting information about entities. This includes
 * also things like the end point of an element which is 
 * nearest to a given coordinate.
 * There's no interaction handled in this class.
 * This class is bound to an entity container.
 *
 * @author Andrew Mustun
 */
class RS_Information {
public:
    explicit RS_Information(RS_EntityContainer& container);

    static bool isDimension(RS2::EntityType type);
    static bool isTrimmable(const RS_Entity* e);

    RS_Vector getNearestEndpoint(const RS_Vector& coord, double* dist = nullptr) const;
    RS_Vector getNearestPointOnEntity(const RS_Vector& coord, bool onEntity = true, double* dist = nullptr,
                                      RS_Entity** entity = nullptr) const;
    RS_Entity* getNearestEntity(const RS_Vector& coord, double* dist = nullptr, RS2::ResolveLevel level = RS2::ResolveAll) const;

    static RS_VectorSolutions getIntersection(const RS_Entity* entity1, const RS_Entity* entity2, bool onEntities = false);

    static RS_VectorSolutions getIntersectionLineLine(const RS_Entity* line1, const RS_Entity* line2);

    static RS_VectorSolutions getIntersectionLineArc(const RS_Entity* line, const RS_Entity* arc);

    static RS_VectorSolutions getIntersectionArcArc(const RS_Entity* arc1, const RS_Entity* arc2);

    static RS_VectorSolutions getIntersectionEllipseEllipse(const RS_Ellipse* ellipse1, const RS_Ellipse* ellipse2);
    static RS_VectorSolutions getIntersectionArcEllipse(const RS_Arc* arc, const RS_Ellipse* ellipse);
    static RS_VectorSolutions getIntersectionCircleEllipse(const RS_Circle* circle, const RS_Ellipse* ellipse);

    static RS_VectorSolutions getIntersectionEllipseLine(const RS_Line* line, const RS_Ellipse* ellipse);
    /**
  * @brief createQuadrilateral form quadrilateral from 4 straight lines
  * @param container contains 4 straight lines
  * @return ordered vertices of quadrilateral formed by the 4 lines
  */
    static RS_VectorSolutions createQuadrilateral(const RS_EntityContainer& container);

    static bool isPointInsideContour(const RS_Vector& point, const RS_EntityContainer* contour, bool* onContour = nullptr);

private:
    RS_EntityContainer* m_container = nullptr;
};

#endif
