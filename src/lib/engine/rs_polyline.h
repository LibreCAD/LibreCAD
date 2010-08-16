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


#ifndef RS_POLYLINE_H
#define RS_POLYLINE_H

#include "rs_entity.h"
#include "rs_entitycontainer.h"



/**
 * Holds the data that defines a polyline.
 */
class RS_PolylineData : public RS_Flags {
public:
    RS_PolylineData() {
        startpoint = endpoint = RS_Vector(false);
    }
    RS_PolylineData(const RS_Vector& startpoint,
                    const RS_Vector& endpoint,
                    bool closed) {

        this->startpoint = startpoint;
        this->endpoint = endpoint;
        if (closed==true) {
            setFlag(RS2::FlagClosed);
        }
    }

    friend class RS_Polyline;

    friend std::ostream& operator << (std::ostream& os,
                                      const RS_PolylineData& pd) {
        os << "(" << pd.startpoint <<
        "/" << pd.endpoint <<
        ")";
        return os;
    }

private:
    RS_Vector startpoint;
    RS_Vector endpoint;
};



/**
 * Class for a poly line entity (lots of connected lines and arcs).
 *
 * @author Andrew Mustun
 */
class RS_Polyline : public RS_EntityContainer {
public:
    RS_Polyline(RS_EntityContainer* parent=NULL);
    RS_Polyline(RS_EntityContainer* parent,
                const RS_PolylineData& d);
    virtual ~RS_Polyline();

    virtual RS_Entity* clone() {
        RS_Polyline* p = new RS_Polyline(*this);
		p->entities.setAutoDelete(entities.autoDelete());
        p->initId();
        p->detach();
        return p;
    }

    /**	@return RS2::EntityPolyline */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityPolyline;
    }

    /** @return Copy of data that defines the polyline. */
    RS_PolylineData getData() const {
        return data;
    }

    /** sets a new start point of the polyline */
    void setStartpoint(RS_Vector& v) {
        data.startpoint = v;
        if (!data.endpoint.valid) {
            data.endpoint = v;
        }
    }

    /** @return Start point of the entity */
    RS_Vector getStartpoint() {
        return data.startpoint;
    }
	
    /** sets a new end point of the polyline */
    void setEndpoint(RS_Vector& v) {
        data.endpoint = v;
    }

    /** @return End point of the entity */
    RS_Vector getEndpoint() {
        return data.endpoint;
    }

	double getClosingBulge();

	void updateEndpoints();

    /** @return true if the polyline is closed. false otherwise */
    bool isClosed() const {
        return data.getFlag(RS2::FlagClosed);
    }

	void setClosed(bool cl) {
		if (cl) {
			data.setFlag(RS2::FlagClosed);
		}
		else {
			data.delFlag(RS2::FlagClosed);
		}
	}
    
	virtual RS_VectorSolutions getRefPoints();
    virtual RS_Vector getNearestRef(const RS_Vector& coord,
                                     double* dist = NULL);
    virtual RS_Vector getNearestSelectedRef(const RS_Vector& coord,
                                     double* dist = NULL);

    virtual RS_Entity* addVertex(const RS_Vector& v, 
		double bulge=0.0, bool prepend=false);
	virtual void setNextBulge(double bulge) {
		nextBulge = bulge;
	}
    virtual void addEntity(RS_Entity* entity);
    //virtual void addSegment(RS_Entity* entity);
    virtual void removeLastVertex();
    virtual void endPolyline();

	//virtual void reorder();

    virtual void move(RS_Vector offset);
    virtual void rotate(RS_Vector center, double angle);
    virtual void scale(RS_Vector center, RS_Vector factor);
    virtual void mirror(RS_Vector axisPoint1, RS_Vector axisPoint2);
	virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);
    virtual void stretch(RS_Vector firstCorner,
                         RS_Vector secondCorner,
                         RS_Vector offset);
	
    virtual void draw(RS_Painter* painter, RS_GraphicView* view, 
		double patternOffset=0.0);

    friend std::ostream& operator << (std::ostream& os, const RS_Polyline& l);

protected:
    virtual RS_Entity* createVertex(const RS_Vector& v, 
		double bulge=0.0, bool prepend=false);

protected:
    RS_PolylineData data;
    RS_Entity* closingEntity;
	double nextBulge;
};

#endif
