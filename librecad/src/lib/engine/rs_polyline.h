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
        p->setOwner(isOwner());
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
    virtual RS_Vector getStartpoint() const {
        return data.startpoint;
    }

    /** sets a new end point of the polyline */
    void setEndpoint(RS_Vector& v) {
        data.endpoint = v;
    }

    /** @return End point of the entity */
    virtual RS_Vector getEndpoint() const {
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

    void setClosed(bool cl, double bulge);//RLZ: rewrite this:

    virtual RS_VectorSolutions getRefPoints();
    virtual RS_Vector getMiddlePoint(void)const {
            return RS_Vector(false);
    }
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

    virtual bool offset(const RS_Vector& coord, const double& distance);
    virtual void move(const RS_Vector& offset);
    virtual void rotate(const RS_Vector& center, const double& angle);
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector);
    virtual void scale(const RS_Vector& center, const RS_Vector& factor);
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2);
    virtual void stretch(const RS_Vector& firstCorner,
                         const RS_Vector& secondCorner,
                         const RS_Vector& offset);

    virtual void moveRef(const RS_Vector& ref, const RS_Vector& offset);

    virtual void draw(RS_Painter* painter, RS_GraphicView* view,
                      double& patternOffset);

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
