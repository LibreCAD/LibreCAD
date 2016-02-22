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
    RS_Information(RS_EntityContainer& entityContainer);

	static bool isDimension(RS2::EntityType type);
	static bool isTrimmable(RS_Entity* e);
	static bool isTrimmable(RS_Entity* e1, RS_Entity* e2);

    RS_Vector getNearestEndpoint(const RS_Vector& point,
								 double* dist = nullptr) const;
    RS_Vector getNearestPointOnEntity(const RS_Vector& point,
	                                  bool onEntity=true, 
									  double* dist = nullptr,
									  RS_Entity** entity=nullptr) const;
    RS_Entity* getNearestEntity(const RS_Vector& point,
								double* dist = nullptr,
                                RS2::ResolveLevel level=RS2::ResolveAll) const;


	static RS_VectorSolutions getIntersection(RS_Entity const* e1,
			RS_Entity const* e2,
            bool onEntities = false);

    static RS_VectorSolutions getIntersectionLineLine(RS_Line* e1,
            RS_Line* e2);

    static RS_VectorSolutions getIntersectionLineArc(RS_Line* line,
            RS_Arc* arc);

	static RS_VectorSolutions getIntersectionArcArc(RS_Entity const* e1,
			RS_Entity const* e2);

	static RS_VectorSolutions getIntersectionEllipseEllipse(
			RS_Ellipse const* e1,
			RS_Ellipse const* e2);
    static RS_VectorSolutions getIntersectionArcEllipse(RS_Arc* e1,
            RS_Ellipse* e2);
    static RS_VectorSolutions getIntersectionCircleEllipse(RS_Circle* e1,
            RS_Ellipse* e2);
    
	static RS_VectorSolutions getIntersectionEllipseLine(RS_Line* line,
            RS_Ellipse* ellipse);
	/**
	 * @brief createQuadrilateral form quadrilateral from 4 straight lines
	 * @param container contains 4 straight lines
	 * @return ordered vertices of quadrilateral formed by the 4 lines
	 */
	static RS_VectorSolutions createQuadrilateral(const RS_EntityContainer& container);

    static bool isPointInsideContour(const RS_Vector& point,
                                     RS_EntityContainer* contour,
									 bool* onContour=nullptr);
	
private:
    RS_EntityContainer* container;
};



#endif
