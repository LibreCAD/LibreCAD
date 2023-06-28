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


#ifndef RS_DIMLINEAR_H
#define RS_DIMLINEAR_H

#include "rs_dimension.h"

/**
 * Holds the data that defines a linear dimension entity.
 */
struct RS_DimLinearData {
    /**
	 * Default constructor
     */
	RS_DimLinearData();

    /**
     * Constructor with initialisation.
     *
     * @para extensionPoint1 Startpoint of the first extension line.
     * @para extensionPoint2 Startpoint of the second extension line.
     * @param angle Rotation angle in rad.
     * @param oblique Oblique angle in rad.
     */
    RS_DimLinearData(const RS_Vector& extensionPoint1,
                     const RS_Vector& extensionPoint2,
					 double angle, double oblique);

    /** Definition point. Startpoint of the first definition line. */
    RS_Vector extensionPoint1;
    /** Definition point. Startpoint of the second definition line. */
    RS_Vector extensionPoint2;
    /** Rotation angle in rad. */
    double angle;
    /** Oblique angle in rad. */
    double oblique;
};

std::ostream& operator << (std::ostream& os,
									  const RS_DimLinearData& dd);

/**
 * Class for aligned dimension entities.
 *
 * @author Andrew Mustun
 */
class RS_DimLinear : public RS_Dimension {
public:
    RS_DimLinear(RS_EntityContainer* parent,
                 const RS_DimensionData& d,
                 const RS_DimLinearData& ed);
	virtual ~RS_DimLinear() = default;

    RS_Entity* clone() const override;

    /**	@return RS2::EntityDimLinear */
    RS2::EntityType rtti() const override {
        return RS2::EntityDimLinear;
    }

    /**
     * @return Copy of data that defines the linear dimension.
     * @see getData()
     */
    RS_DimLinearData getEData() const {
        return edata;
    }

    RS_VectorSolutions getRefPoints() const override;

    QString getMeasuredLabel() override;

    void updateDim(bool autoText=false) override;

	RS_Vector getExtensionPoint1() const{
        return edata.extensionPoint1;
    }

	RS_Vector getExtensionPoint2() const{
        return edata.extensionPoint2;
    }

	double getAngle() const{
        return edata.angle;
    }

	void setAngle(double a);

	double getOblique() const{
        return edata.oblique;
    }

    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, const double& angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    bool hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2) override;
    void stretch(const RS_Vector& firstCorner,
                         const RS_Vector& secondCorner,
                         const RS_Vector& offset) override;
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_DimLinear& d);

protected:
    /** Extended data. */
    RS_DimLinearData edata;
};

#endif
