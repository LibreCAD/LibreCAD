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

#ifndef RS_INFOAREA_H
#define RS_INFOAREA_H

#include <vector>
#include "rs_vector.h"

class QPolygon;
/**
 * Class for getting information about an area.
 *
 * @author Andrew Mustun
 */
class RS_InfoArea {
public:
    RS_InfoArea();
    void reset();
    void push_back(const RS_Vector &p);
    //whether the point p is already in contour
    bool duplicated(const RS_Vector &p);
    void pop_back();
    double getArea() const;
    double getCircumference();
    int size();
    RS_Vector& front() {
        return m_points.front();
    }
    const RS_Vector& front() const {
        return m_points.front();
    }
    RS_Vector& back();
    const RS_Vector& back() const;
    RS_Vector &at(int i);
    const RS_Vector &at(int i) const;
    /**
     * @brief getArea of polygon
     * @param polygon
     * @return area
     */
    static double getArea(const QPolygon &polygon);

private:
    void calculate();
    double calcSubArea(const RS_Vector &p1, const RS_Vector &p2);
    std::vector<RS_Vector> m_points;
    double baseY = 0.;
    double area = 0.;
    double circumference = 0.;
    bool calculationNeeded = false;

public:
    auto begin() -> decltype(m_points.begin()) {
        return m_points.begin();
    }
    auto cbegin() const -> decltype(m_points.cbegin()) {
        return m_points.cbegin();
    }
    auto end() -> decltype(m_points.end()) {
        return m_points.end();
    }
    auto cend() const -> decltype(m_points.cend()) {
        return m_points.cend();
    }

};

#endif
