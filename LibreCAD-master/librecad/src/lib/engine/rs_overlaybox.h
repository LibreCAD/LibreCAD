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
	RS_OverlayBoxData() = default;

    RS_OverlayBoxData(const RS_Vector& corner1, const RS_Vector& corner2)
                     : corner1(corner1), corner2(corner2) {}

    friend class RS_OverlayBox;

    friend std::ostream& operator << (std::ostream& os, const RS_OverlayBoxData& ld);

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
	RS_Entity* clone() const override;

    /**	@return RS2::EntityLine */
	RS2::EntityType rtti() const override{
        return RS2::EntityOverlayBox;
    }
	void draw(RS_Painter* painter, RS_GraphicView* view, double& patternOffset) override;

    /** @return Start point of the entity */
	RS_Vector getCorner1() const {
        return data.corner1;
    }
    /** @return End point of the entity */
	RS_Vector getCorner2() const {
        return data.corner2;
    }
    /** @return Copy of data that defines the line. */
    RS_OverlayBoxData getData() const {
        return data;
    }

    /** We should make a separate drawing mechanism for overlays and not use entities */
	void move(const RS_Vector& /*offset*/)override{}
	void rotate(const RS_Vector& /*center*/, const double& /*angle*/)override{}
	void rotate(const RS_Vector& /*center*/, const RS_Vector& /*angleVector*/)override{}
	void scale(const RS_Vector& /*center*/, const RS_Vector& /*factor*/)override{}
	void mirror(const RS_Vector& /*axisPoint1*/, const RS_Vector& /*axisPoint2*/)override{}
	void calculateBorders() override{}
	RS_Vector getNearestEndpoint(const RS_Vector&, double*)const override{return {};}
	RS_Vector getNearestPointOnEntity(const RS_Vector&, bool, double*, RS_Entity**)const override{return {};}
	RS_Vector getNearestCenter(const RS_Vector&, double*)const override{return {};}
	RS_Vector getNearestMiddle(const RS_Vector&, double*,int)const override{return {};}
	RS_Vector getNearestDist(double, const RS_Vector&, double*)const override{return {};}
	double getDistanceToPoint(const RS_Vector&, RS_Entity**, RS2::ResolveLevel, double)const override{return -1;}//is -1 right here

protected:
    RS_OverlayBoxData data;
}
;

#endif
