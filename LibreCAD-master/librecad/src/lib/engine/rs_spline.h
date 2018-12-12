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


#ifndef RS_SPLINE_H
#define RS_SPLINE_H

#include <vector>
#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a line.
 */
struct RS_SplineData {
	/**
	 * Default constructor. Leaves the data object uninitialized.
	 */
	RS_SplineData() = default;

	RS_SplineData(int degree, bool closed);


	/** Degree of the spline (1, 2, 3) */
	size_t degree;
	/** Closed flag. */
	bool closed;
	/** Control points of the spline. */
	std::vector<RS_Vector> controlPoints;
	std::vector<double> knotslist;
};

std::ostream& operator << (std::ostream& os, const RS_SplineData& ld);

/**
 * Class for a spline entity.
 *
 * @author Andrew Mustun
 */
class RS_Spline : public RS_EntityContainer {
public:
    RS_Spline(RS_EntityContainer* parent,
            const RS_SplineData& d);

	RS_Entity* clone() const override;


    /**	@return RS2::EntitySpline */
	RS2::EntityType rtti() const override{
        return RS2::EntitySpline;
    }
    /** @return false */
	bool isEdge() const override{
        return false;
    }

	/** @return Copy of data that defines the spline. */
	const RS_SplineData& getData() const {
		return data;
	}

	/** Sets the splines degree (1-3). */
	void setDegree(size_t deg);

	/** @return Degree of this spline curve (1-3).*/
	size_t getDegree() const;

	/** @return 0. */
	int getNumberOfKnots() {
		return 0;
	}

	/** @return Number of control points. */
	size_t getNumberOfControlPoints() const;

	/**
		 * @retval true if the spline is closed.
		 * @retval false otherwise.
		 */
	bool isClosed() const;

	/**
		 * Sets the closed flag of this spline.
		 */
	void setClosed(bool c);

	RS_VectorSolutions getRefPoints() const override;
	RS_Vector getNearestRef( const RS_Vector& coord, double* dist = nullptr) const override;
	RS_Vector getNearestSelectedRef( const RS_Vector& coord, double* dist = nullptr) const override;

    /** @return Start point of the entity */
	RS_Vector getStartpoint() const override;
    /** @return End point of the entity */
	RS_Vector getEndpoint() const override;
    /** Sets the startpoint */
    /** Sets the endpoint */
	void update() override;

	RS_Vector getNearestEndpoint(const RS_Vector& coord,
										 double* dist = nullptr)const override;
	//RS_Vector getNearestPointOnEntity(const RS_Vector& coord,
	//        bool onEntity=true, double* dist = nullptr, RS_Entity** entity=nullptr);
	RS_Vector getNearestCenter(const RS_Vector& coord,
									   double* dist = nullptr)const override;
	RS_Vector getNearestMiddle(const RS_Vector& coord,
									   double* dist = nullptr,
									   int middlePoints = 1)const override;
	RS_Vector getNearestDist(double distance,
                                     const RS_Vector& coord,
									 double* dist = nullptr)const override;

		void addControlPoint(const RS_Vector& v);
		void removeLastControlPoint();

		void move(const RS_Vector& offset) override;
		void rotate(const RS_Vector& center, const double& angle) override;
		void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
		void scale(const RS_Vector& center, const RS_Vector& factor) override;
		void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

		void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
		void revertDirection() override;

		void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;
		const std::vector<RS_Vector>& getControlPoints() const;

        friend std::ostream& operator << (std::ostream& os, const RS_Spline& l);

		void calculateBorders() override;

private:
		std::vector<double> knot(size_t num, size_t order) const;
		void rbspline(size_t npts, size_t k, size_t p1,
		              const std::vector<RS_Vector>& b,
		              const std::vector<double>& h,
		              std::vector<RS_Vector>& p) const;

		std::vector<double> knotu(size_t num, size_t order) const;
		void rbsplinu(size_t npts, size_t k, size_t p1,
		              const std::vector<RS_Vector>& b,
		              const std::vector<double>& h,
		              std::vector<RS_Vector>& p) const;

protected:
		RS_SplineData data;
}
;

#endif
