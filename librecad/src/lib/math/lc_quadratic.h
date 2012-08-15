/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011-2012 Dongxu Li (dongxuli2011@gmail.com)

Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)

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


#ifndef LC_QUADRATIC_H
#define LC_QUADRATIC_H


#include "rs_vector.h"
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

class RS_VectorSolutions;
class RS_AtomicEntity;

/**
 * Class for generic linear and quadratic equation
 * supports translation and rotation of an equation
 *
 * @author Dongxu Li
 */
class LC_Quadratic {
public:
    explicit LC_Quadratic();
    /** construct a ellipse or hyperbola as the path of center of tangent circles
      passing the point */
    LC_Quadratic(const RS_AtomicEntity* circle, const RS_Vector& point);
    /** construct a ellipse or hyperbola as the path of center of common tangent circles
      of this two given entities*/
    LC_Quadratic(const RS_AtomicEntity* circle0,const RS_AtomicEntity* circle1);
    LC_Quadratic(std::vector<double> ce);
    std::vector<double> getCoefficients() const;
    LC_Quadratic move(const RS_Vector& v);
    LC_Quadratic rotate(const double& a);
    LC_Quadratic rotate(const RS_Vector& center, const double& a);
    /** whether it's quadratic or linear
      @return true, if quadratic;
      return false, if linear
 */
    bool isQuadratic() const {
        return m_bIsQuadratic;
    }
    bool isValid() const
    {
            return m_bValid;
    }
    void setValid(bool value)
    {
        m_bValid=value;
    }

    /** switch x,y coordinates */
    LC_Quadratic flipXY(void) const;
    /** the matrix of rotation by angle **/
    static boost::numeric::ublas::matrix<double> rotationMatrix(const double& angle);

    static RS_VectorSolutions getIntersection(const LC_Quadratic& l1, const LC_Quadratic& l2);

    friend std::ostream& operator << (std::ostream& os, const LC_Quadratic& l);

//    private:
    // the equation form: {x, y}.m_mQuad.{{x},{y}} + m_vLinear.{{x},{y}}+m_dConst=0
    boost::numeric::ublas::matrix<double> m_mQuad;
    boost::numeric::ublas::vector<double> m_vLinear;
    double m_dConst;
    bool m_bIsQuadratic;
    /** whether this quadratic form is valid */
    bool m_bValid;
};



#endif
//EOF
