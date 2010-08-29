/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
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


#ifndef RS_OVERLAYBOX_H
#define RS_OVERLAYBOX_H

#include "rs_atomicentity.h"

/**
 * Holds the data that defines a line.
 */
class RS_OverlayBoxData {
public:
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_OverlayBoxData() {}

    RS_OverlayBoxData(const RS_Vector& corner1, const RS_Vector& corner2) {

        this->corner1 = corner1;
        this->corner2 = corner2;
    }

    friend class RS_OverlayBox;

    friend std::ostream& operator << (std::ostream& os, const RS_OverlayBoxData& ld) {
        os << "(" << ld.corner1 <<
        "/" << ld.corner2 <<
        ")";
        return os;
    }

public:
    RS_Vector corner1;
    RS_Vector corner2;
};


/**
 * Class for a line entity.
 *
 * @author R. van Twisk
 */
class RS_OverlayBox : public RS_AtomicEntity {
public:
    RS_OverlayBox(RS_EntityContainer* parent, const RS_OverlayBoxData& d);
    virtual RS_Entity* clone();
    virtual ~RS_OverlayBox();

    /**	@return RS2::EntityLine */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityOverlayBox;
    }
    virtual void draw(RS_Painter* painter, RS_GraphicView* view, double patternOffset=0.0);

	/** @return Start point of the entity */
    virtual RS_Vector getCorner1() const {
        return data.corner1;
    }
    /** @return End point of the entity */
    virtual RS_Vector getCorner2() const {
        return data.corner2;
    }
	/** @return Copy of data that defines the line. */
    RS_OverlayBoxData getData() const {
        return data;
    }
	
	/** We should make a seperate drawing meganism for overlays and not use entities */
	virtual void move(RS_Vector offset){}
    virtual void rotate(RS_Vector center, double angle){}
    virtual void scale(RS_Vector center, RS_Vector factor){}
    virtual void mirror(RS_Vector axisPoint1, RS_Vector axisPoint2){}
	virtual void calculateBorders(){}
	virtual RS_Vector getNearestEndpoint(const RS_Vector&, double*){return RS_Vector();}
	virtual RS_Vector getNearestPointOnEntity(const RS_Vector&, bool, double*, RS_Entity**){return RS_Vector();}
	virtual RS_Vector getNearestCenter(const RS_Vector&, double*){return RS_Vector();}
	virtual RS_Vector getNearestMiddle(const RS_Vector&, double*){return RS_Vector();}
	virtual RS_Vector getNearestDist(double, const RS_Vector&, double*){return RS_Vector();}
	virtual double getDistanceToPoint(const RS_Vector&, RS_Entity**, RS2::ResolveLevel, double){return -1;}
	
protected:
    RS_OverlayBoxData data;
}
;

#endif
