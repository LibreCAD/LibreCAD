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


#pragma once
#ifndef RS_Polyline_INCLUDE_H

#include <utility>

#include "rs_entitycontainer.h"

class RS_Arc;
class RS_Ellipse;

/**
 * Holds the data that defines a polyline.
 */
struct RS_PolylineData : public RS_Flags {
    RS_PolylineData() = default;
	RS_PolylineData(const RS_Vector& startpoint,
                    const RS_Vector& endpoint,
					bool closed);

    RS_Vector startpoint;
    RS_Vector endpoint;
};

std::ostream& operator << (std::ostream& os, const RS_PolylineData& pd);

/**
 * Class for a poly line entity (lots of connected lines and arcs).
 *
 * @author Andrew Mustun
 */
class RS_Polyline:public RS_EntityContainer {
public:
    RS_Polyline(RS_EntityContainer *parent = nullptr);
    RS_Polyline(RS_EntityContainer *parent, const RS_PolylineData &d);
    RS_Entity *clone() const override;

    /**	@return RS2::EntityPolyline */
    RS2::EntityType rtti() const override{
        return RS2::EntityPolyline;
    }

    /** @return Copy of data that defines the polyline. */
    RS_PolylineData getData() const{
        return data;
    }

    RS_PolylineData& getData() {
        return data;
    }

    /** sets a new start point of the polyline */
    void setStartpoint(RS_Vector const &v);
    /** @return Start point of the entity */
    RS_Vector getStartpoint() const override;
    /** sets a new end point of the polyline */
    void setEndpoint(RS_Vector const &v);
    // set layer for polyline and sub-entities
    void setLayer(const QString &name);
    void setLayer(RS_Layer *l);
    /** @return End point of the entity */
    RS_Vector getEndpoint() const override;
    double getClosingBulge() const;
    void updateEndpoints();
    /** @return true if the polyline is closed. false otherwise */
    bool isClosed() const;
    void setClosed(bool cl);
    void setClosed(bool cl, double bulge);//RLZ: rewrite this:

    RS_VectorSolutions getRefPoints() const override;

    RS_Vector getMiddlePoint(void) const override{
        return RS_Vector(false);
    }

    RS_Vector getNearestRef(
        const RS_Vector &coord,
        double *dist = nullptr) const override;
    RS_Vector getNearestSelectedRef(
        const RS_Vector &coord,
        double *dist = nullptr) const override;
    RS_Entity *addVertex(
        const RS_Vector &v,
        double bulge = 0.0, bool prepend = false);
    void appendVertexs(const std::vector<std::pair<RS_Vector, double> > &vl);

    void setNextBulge(double bulge){
        m_nextBulge = bulge;
    }

    void addEntity(RS_Entity *entity) override;
//void addSegment(RS_Entity* entity) override;
    void removeLastVertex();
    void endPolyline();

//void reorder() override;

    bool offset(const RS_Vector &coord, const double &distance) override;
    void move(const RS_Vector &offset) override;
    void rotate(const RS_Vector &center, double angle) override;
    void rotate(const RS_Vector &center, const RS_Vector &angleVector) override;
    void scale(const RS_Vector &center, const RS_Vector &factor) override;
    void mirror(const RS_Vector &axisPoint1, const RS_Vector &axisPoint2) override;
    void stretch(
        const RS_Vector &firstCorner,
        const RS_Vector &secondCorner,
        const RS_Vector &offset) override;
    void moveRef(const RS_Vector &ref, const RS_Vector &offset) override;
    void revertDirection() override;

    /**
 * @brief containsArc whether the polyline contains an arc segment
 * @return true - if the polyline contains any circular arc
 */
    bool containsArc() const;
    void draw(RS_Painter *painter) override;
    void drawAsChild(RS_Painter *painter) override;
    friend std::ostream &operator<<(std::ostream &os, const RS_Polyline &l);
    RS_Vector getRefPointAdjacentDirection(bool previousSegment, RS_Vector& refPoint);
    static RS_Ellipse* convertToEllipse(const std::pair<RS_Arc*, double>& arcPair);
    static std::pair<RS_Arc*, double> convertToArcPair(const RS_Ellipse* ellipse);
    static RS_Arc* arcFromBulge(const RS_Vector& start, const RS_Vector& end, double bulge);

protected:
    std::unique_ptr<RS_Entity> createVertex(
        const RS_Vector &v,
        double bulge = 0.0, bool prepend = false);

private:
    /**
     * @brief Converts all circular arc (RS_Arc) segments to equivalent elliptic arcs (RS_Ellipse)
     *        to correctly handle non-uniform scaling (factor.x != factor.y).
     *        Computes scaleRatio = |factor.y / factor.x| and replaces each arc with an
     *        axis-aligned ellipse that matches the arc's geometry when stretched.
     *        Preserves entity order/index in the container. No-op if uniform scaling.
     * @param factor Scaling factor vector used to determine y/x scale ratio.
     * @note Called from scale() before applying the transformation.
     * @note Relies on static convertToEllipse() for per-arc conversion.
     */
    void convertArcsToElliptic(const RS_Vector &factor);

    RS_PolylineData data;
    RS_Entity *m_closingEntity = nullptr;
    double m_nextBulge = 0.;
};

#endif // RS_Polyline_INCLUDE_H
