/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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
class RS_DimDiametricData {
public:
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_DimDiametricData() {}

    /**
     * Constructor with initialisation.
     *
     * @param definitionPoint Definition point of the diametric dimension. 
     * @param leader Leader length.
     */
    RS_DimDiametricData(const RS_Vector& definitionPoint,
                     double leader) {
        this->definitionPoint = definitionPoint;
        this->leader = leader;
    }

    friend class RS_DimDiametric;
    //friend class RS_ActionDimDiametric;

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_DimDiametricData& dd) {
        os << "(" << dd.definitionPoint << "/" << dd.leader << ")";
        return os;
    }

public:
    /** Definition point. */
    RS_Vector definitionPoint;
    /** Leader length. */
    double leader;
};



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
    virtual ~RS_DimDiametric() {}

    virtual RS_Entity* clone() {
        RS_DimDiametric* d = new RS_DimDiametric(*this);
		d->entities.setAutoDelete(entities.autoDelete());
        d->initId();
        d->detach();
        return d;
    }

    /**	@return RS2::EntityDimDiametric */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityDimDiametric;
    }

    /**
     * @return Copy of data that defines the diametric dimension. 
     * @see getData()
     */
    RS_DimDiametricData getEData() const {
        return edata;
    }
    
	virtual RS_VectorSolutions getRefPoints();

    virtual RS_String getMeasuredLabel();

    virtual void update(bool autoText=false);

    RS_Vector getDefinitionPoint() {
        return edata.definitionPoint;
    }
    double getLeader() {
        return edata.leader;
    }

    virtual void move(RS_Vector offset);
    virtual void rotate(RS_Vector center, double angle);
    virtual void scale(RS_Vector center, RS_Vector factor);
    virtual void mirror(RS_Vector axisPoint1, RS_Vector axisPoint2);
	virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_DimDiametric& d);

protected:
    /** Extended data. */
    RS_DimDiametricData edata;
};

#endif
