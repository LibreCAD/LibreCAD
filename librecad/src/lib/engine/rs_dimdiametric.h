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


#ifndef RS_DIMDIAMETER_H
#define RS_DIMDIAMETER_H

#include "rs_dimension.h"

/**
 * Holds the data that defines a diametric dimension entity.
 */
struct RS_DimDiametricData {
    /**
	 * Default constructor
     */
	RS_DimDiametricData();

    /**
     * Constructor with initialisation.
     *
     * @param definitionPoint Definition point of the diametric dimension.
     * @param leader Leader length.
     */
    RS_DimDiametricData(const RS_Vector& definitionPoint,
					 double leader);


    /** Definition point. */
    RS_Vector definitionPoint;
    /** Leader length. */
    double leader;
};

std::ostream& operator << (std::ostream& os, const RS_DimDiametricData& dd);

/**
 * Class for diametric dimension entities.
 *
 * @author Andrew Mustun
 */
class RS_DimDiametric : public RS_Dimension {
public:
    RS_DimDiametric(RS_EntityContainer* parent,
                 const RS_DimensionData& d,
                 const RS_DimDiametricData& ed);

	RS_Entity* clone() const override;

    /**	@return RS2::EntityDimDiametric */
	RS2::EntityType rtti() const override{
        return RS2::EntityDimDiametric;
    }

    /**
     * @return Copy of data that defines the diametric dimension.
     * @see getData()
     */
    RS_DimDiametricData getEData() const {
        return edata;
    }

	RS_VectorSolutions getRefPoints() const override;

	QString getMeasuredLabel() override;

	void updateDim(bool autoText=false) override;

    RS_Vector getDefinitionPoint() {
        return edata.definitionPoint;
    }
    double getLeader() {
        return edata.leader;
    }
	void move(const RS_Vector& offset) override;
	void rotate(const RS_Vector& center, const double& angle) override;
	void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
	void scale(const RS_Vector& center, const RS_Vector& factor) override;
	void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;

	void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_DimDiametric& d);

protected:
    /** Extended data. */
    RS_DimDiametricData edata;
};

#endif
