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

#include "rs_infoarea.h"

#include "rs_math.h"
#include "rs_debug.h"


/**
 * Constructor.
 */
RS_InfoArea::RS_InfoArea() {
}



/**
 * Destructor.
 */
RS_InfoArea::~RS_InfoArea() {}



/**
 * Adds a point to the internal list
 *
 * @param p co-ordinate of the point
 */
void RS_InfoArea::addPoint(const RS_Vector& p) {
        if (thePoints.empty()) {
                baseY = p.y;
        }

        thePoints.append(p);
}

//remove the last point
void RS_InfoArea::pop_back() {
    thePoints.pop_back();
}

/**
 * Resets the points.
 */
void RS_InfoArea::reset() {
        thePoints.clear();
        area = 0.0;
        circumference = 0.0;
}



/**
 * Closes the polygon if it is not closed already.
 */
void RS_InfoArea::close(bool closePolygon ) {
    if( ! isClosed() && closePolygon){
        thePoints.append(thePoints.first());
        RS_DEBUG->print("RS_InfoArea::close(): closed");
        return;
    }
    if( isClosed() && !closePolygon) {
        thePoints.pop_back();
        RS_DEBUG->print("RS_InfoArea::close(): not closed");
    }
}



/**
 * @retval true If the area is closed (i.e. start point and end point are
 *   identical)
 * @retval false Otherwise.
 */
bool RS_InfoArea::isClosed() {
        return ((thePoints.first()-thePoints.last()).squared()<RS_TOLERANCE) ;
//	return (thePoints.first().distanceTo(thePoints.last())<1.0e-4);
}



/**
 * @retval true If the area is defined (i.e. there are at least 3 points)
 * @retval false If there are only two or less points.
 */
bool RS_InfoArea::isValid() {
        RS_DEBUG->print("RS_InfoArea::isValid: count: %d", thePoints.count());
        return (thePoints.count()>2);
}



/**
 * Calculates the area and the circumference of the area.
 */
void RS_InfoArea::calculate() {
        area = 0.0;
        circumference = 0.0;
        RS_Vector ptFirst;
        RS_Vector p1;
        RS_Vector p2;
        QVector<RS_Vector> pts(thePoints); //do not destroy the saved points

        // at least 3 points needed for an area
        if (isValid()) {
                ptFirst = pts.last();
                pts.pop_back();

                p1 = ptFirst;
                while (!pts.empty()) {
                        p2 = pts.last();
                        pts.pop_back();

                        area += calcSubArea(p1, p2);
                        circumference += p1.distanceTo(p2);
                        //if (p1 != ptFirst) {
                        //	delete p1;
                        //}
                        p1 = p2;
                }
                area += calcSubArea(p1, ptFirst);
                circumference += p1.distanceTo(ptFirst);
                //delete p1;
                //delete ptFirst;
        }

        //thePoints.clear();
        area = fabs(area);
}



/**
 * Calculates a sub area.
 *
 * @param p1 first point
 * @param p2 second point
 */
double RS_InfoArea::calcSubArea(const RS_Vector& p1, const RS_Vector& p2) {
        double width = p2.x - p1.x;
        double height = (p1.y - baseY) + (p2.y - baseY);

        return width * height / 2.0;
}



/*! Calculates a distance
    \param _p1 first point
    \param _p2 second point
*/
/*double
RS_InfoArea::calcDistance(RS_Vector *_p1, RS_Vector *_p2)
{
        return mtGetDistance(_p1->getX(), _p1->getY(), _p2->getX(), _p2->getY());
}*/


// EOF
