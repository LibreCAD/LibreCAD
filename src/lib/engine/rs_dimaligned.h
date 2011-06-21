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


#ifndef RS_DIMALIGNED_H
#define RS_DIMALIGNED_H

#include "rs_dimension.h"

/**
 * Holds the data that defines an aligned dimension entity.
 */
class RS_DimAlignedData {
public:
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_DimAlignedData() {}

    /**
     * Constructor with initialisation.
     *
        * @para extensionPoint1 Definition point. Startpoint of the 
     *         first extension line.
        * @para extensionPoint2 Definition point. Startpoint of the 
     *         second extension line.
     */
    RS_DimAlignedData(const RS_Vector& extensionPoint1,
                      const RS_Vector& extensionPoint2) {
        this->extensionPoint1 = extensionPoint1;
        this->extensionPoint2 = extensionPoint2;
    }

    friend class RS_DimAligned;
    friend class RS_ActionDimAligned;

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_DimAlignedData& dd) {
        os << "(" << dd.extensionPoint1 << "/" << dd.extensionPoint1 << ")";
        return os;
    }

private:
    /** Definition point. Startpoint of the first extension line. */
    RS_Vector extensionPoint1;
    /** Definition point. Startpoint of the second extension line. */
    RS_Vector extensionPoint2;
};



/**
 * Class for aligned dimension entities.
 *
 * @author Andrew Mustun
 */
class RS_DimAligned : public RS_Dimension {
public:
    RS_DimAligned(RS_EntityContainer* parent,
                  const RS_DimensionData& d,
                  const RS_DimAlignedData& ed);
    virtual ~RS_DimAligned() {}

    virtual RS_Entity* clone() {
        RS_DimAligned* d = new RS_DimAligned(*this);
        d->setOwner(isOwner());
        d->initId();
        d->detach();
        return d;
    }

    /**	@return RS2::EntityDimAligned */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityDimAligned;
    }

    /**
     * @return Copy of data that defines the aligned dimension. 
     * @see getData()
     */
    RS_DimAlignedData getEData() const {
        return edata;
    }

    virtual RS_VectorSolutions getRefPoints();

    virtual RS_String getMeasuredLabel();

    virtual void update(bool autoText=false);

    RS_Vector getExtensionPoint1() {
        return edata.extensionPoint1;
    }

    RS_Vector getExtensionPoint2() {
        return edata.extensionPoint2;
    }
	
	virtual bool hasEndpointsWithinWindow(RS_Vector v1, RS_Vector v2);

    virtual void move(RS_Vector offset);
    virtual void rotate(RS_Vector center, double angle);
    virtual void scale(RS_Vector center, RS_Vector factor);
    virtual void mirror(RS_Vector axisPoint1, RS_Vector axisPoint2);
    virtual void stretch(RS_Vector firstCorner,
                         RS_Vector secondCorner,
                         RS_Vector offset);
	virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_DimAligned& d);

protected:
    /** Extended data. */
    RS_DimAlignedData edata;
};

#endif
