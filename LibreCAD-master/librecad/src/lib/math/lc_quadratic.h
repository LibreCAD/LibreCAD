/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**

Copyright (C) 2012-2015 Dongxu Li (dongxuli2011@gmail.com)

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
    LC_Quadratic(const LC_Quadratic& lc0);
    LC_Quadratic& operator = (const LC_Quadratic& lc0);
	/** \brief construct a ellipse or hyperbola as the path of center of tangent circles
      passing the point */
    LC_Quadratic(const RS_AtomicEntity* circle, const RS_Vector& point);
	/** \brief construct a ellipse or hyperbola as the path of center of common tangent circles
      of this two given entities,
      mirror option allows to specify the mirror quadratic around the line
*/
    LC_Quadratic(const RS_AtomicEntity* circle0,const RS_AtomicEntity* circle1,
                 bool mirror = false);
    /**
     * @brief LC_Quadratic, construct a Perpendicular bisector line, which is the path of circles passing point0 and point1
     * @param point0
     * @param point1
     */
    LC_Quadratic(const RS_Vector& point0, const RS_Vector& point1);

    LC_Quadratic(std::vector<double> ce);
    std::vector<double> getCoefficients() const;
    LC_Quadratic move(const RS_Vector& v);
    LC_Quadratic rotate(const double& a);
    LC_Quadratic rotate(const RS_Vector& center, const double& a);
	/** \brief whether it's quadratic or linear
      @return true, if quadratic;
      return false, if linear
 */
	bool isQuadratic() const;

	//!
	//! \brief operator bool explicit and implicit conversion to bool
	//!
	explicit operator bool() const;
	bool isValid() const;
	void setValid(bool value);
	//!
	//! \brief operator == comparison of validity with bool
	//! \param valid boolean parameter
	//! \return true is the parameter valid is the same as validity
	//!
	bool operator == (bool valid) const;
	bool operator != (bool valid) const;

	boost::numeric::ublas::vector<double>& getLinear();
	 const boost::numeric::ublas::vector<double>& getLinear() const;
	 boost::numeric::ublas::matrix<double>& getQuad();
	 const boost::numeric::ublas::matrix<double>& getQuad() const;
	 double const& constTerm()const;
	 double& constTerm();

    /** switch x,y coordinates */
    LC_Quadratic flipXY(void) const;
    /** the matrix of rotation by angle **/
    static boost::numeric::ublas::matrix<double> rotationMatrix(const double& angle);

    static RS_VectorSolutions getIntersection(const LC_Quadratic& l1, const LC_Quadratic& l2);

    friend std::ostream& operator << (std::ostream& os, const LC_Quadratic& l);

private:
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
