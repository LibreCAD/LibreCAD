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


#ifndef RS_DIMRADIAL_H
#define RS_DIMRADIAL_H

#include "rs_dimension.h"

/**
 * Holds the data that defines a radial dimension entity.
 */
class RS_DimRadialData {
public:
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_DimRadialData() {}

    /**
     * Constructor with initialisation.
     *
     * @param definitionPoint Definition point of the radial dimension.
     * @param leader Leader length.
     */
    RS_DimRadialData(const RS_Vector& definitionPoint,
                     double leader) {
        this->definitionPoint = definitionPoint;
        this->leader = leader;
    }

    friend class RS_DimRadial;
    //friend class RS_ActionDimRadial;

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_DimRadialData& dd) {
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
 * Class for radial dimension entities.
 *
 * @author Andrew Mustun
 */
class RS_DimRadial : public RS_Dimension {
public:
    RS_DimRadial(RS_EntityContainer* parent,
                 const RS_DimensionData& d,
                 const RS_DimRadialData& ed);
    virtual ~RS_DimRadial() {}

    virtual RS_Entity* clone() {
        RS_DimRadial* d = new RS_DimRadial(*this);
        d->setOwner(isOwner());
        d->initId();
        d->detach();
        return d;
    }

    /**	@return RS2::EntityDimRadial */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityDimRadial;
    }

    /**
     * @return Copy of data that defines the radial dimension.
     * @see getData()
     */
    RS_DimRadialData getEData() const {
        return edata;
    }

    virtual RS_VectorSolutions getRefPoints();

    virtual QString getMeasuredLabel();

    virtual void update(bool autoText=false);

    RS_Vector getDefinitionPoint() {
        return edata.definitionPoint;
    }
    double getLeader() {
        return edata.leader;
    }

    virtual void move(const RS_Vector& offset);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);
    virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_DimRadial& d);

protected:
    /** Extended data. */
    RS_DimRadialData edata;
};

#endif
