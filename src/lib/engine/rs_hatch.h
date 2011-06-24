/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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


#ifndef RS_HATCH_H
#define RS_HATCH_H

#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_string.h"

/**
 * Holds the data that defines a hatch entity.
 */
class RS_HatchData {
public:
    /**
     * Default constructor. Leaves the data object uninitialized.
     */
    RS_HatchData() {}

	/**
	 * @param solid true: solid fill, false: pattern.
	 * @param scale Pattern scale or spacing.
	 * @param pattern Pattern name.
	 */
    RS_HatchData(bool solid,
	             double scale,
				 double angle,
	             const RS_String& pattern) {
		this->solid = solid;
		this->scale = scale;
		this->angle = angle;
		this->pattern = pattern;

		//std::cout << "RS_HatchData: " << pattern.latin1() << "\n";
	}

    friend std::ostream& operator << (std::ostream& os, const RS_HatchData& td) {
        os << "(" << td.pattern.latin1() << ")";
        return os;
    }

public:
	bool solid;
	double scale;
	double angle;
	RS_String pattern;
};



/**
 * Class for a hatch entity.
 *
 * @author Andrew Mustun
 */
class RS_Hatch : public RS_EntityContainer {
public:
    RS_Hatch(RS_EntityContainer* parent,
            const RS_HatchData& d);
    virtual ~RS_Hatch() {}

    virtual RS_Entity* clone();

    /**	@return RS2::EntityHatch */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityHatch;
    }
	
    /**
     * @return true: if this is a hatch with lines (hatch pattern),
     *         false: if this is filled with a solid color.
     */
    virtual bool isContainer() const {
		if (isSolid()) {
			return false;
		}
		else {
        	return true;
		}
    }

    /** @return Copy of data that defines the hatch. */
    RS_HatchData getData() const {
        return data;
    }

	bool validate();
	
	int countLoops();

	/** @return true if this is a solid fill. false if it is a pattern hatch. */
	bool isSolid() const {
		return data.solid;
	}
	void setSolid(bool solid) {
		data.solid = solid;
	}

	RS_String getPattern() {
		return data.pattern;
	}
	void setPattern(const RS_String& pattern) {
		data.pattern = pattern;
	}
	
	double getScale() {
		return data.scale;
	}
	void setScale(double scale) {
		data.scale = scale;
	}
	
	double getAngle() {
		return data.angle;
	}
	void setAngle(double angle) {
		data.angle = angle;
	}

    virtual void calculateBorders();
    void update();
	void activateContour(bool on);
	
    virtual void draw(RS_Painter* painter, RS_GraphicView* view, 
		double patternOffset=0.0);

	virtual double getLength() {
		return -1.0;
	}
	
    virtual double getDistanceToPoint(const RS_Vector& coord,
                                      RS_Entity** entity = NULL,
                                      RS2::ResolveLevel level = RS2::ResolveNone,
									  double solidDist = RS_MAXDOUBLE);

    virtual void move(RS_Vector offset);
    virtual void rotate(RS_Vector center, double angle);
    virtual void scale(RS_Vector center, RS_Vector factor);
    virtual void mirror(RS_Vector axisPoint1, RS_Vector axisPoint2);
    virtual void stretch(RS_Vector firstCorner,
                         RS_Vector secondCorner,
                         RS_Vector offset);

    friend std::ostream& operator << (std::ostream& os, const RS_Hatch& p);

protected:
    RS_HatchData data;
	RS_EntityContainer* hatch;
	bool updateRunning;
	bool needOptimization;
};

#endif
