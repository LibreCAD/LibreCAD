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

#ifndef RS_LEADER_H
#define RS_LEADER_H

#include "rs_dimension.h"
#include "rs_entitycontainer.h"

/**
 * Holds the data that defines a leader.
 */
struct RS_LeaderData {
    RS_LeaderData() = default;

    RS_LeaderData(const bool arrowHeadFlag, const QString& styleName) : arrowHead{arrowHeadFlag}, styleName{styleName} {
    }

    friend std::ostream& operator <<(std::ostream& os, const RS_LeaderData& /*ld*/);

    /** true: leader has an arrow head. false: no arrow. */
    bool arrowHead = false;
    QString styleName;
    bool isStraignt {true};            /*!< Leader path type, code 72, 0=Straight line segments; 1=Spline */
    int flag {3};                  /*!< Leader creation flag, code 73, default 3 */
                                   /*0 = Created with text annotation
                                     1 = Created with tolerance annotation
                                     2 = Created with block reference annotation
                                    3 = Created without any annotation*/
    int hookline {1};              /*!< Hook line direction flag, code 74, default 1 */
                                    /* Hookline direction flag:
                                        0 = Hookline (or end of tangent for a splined leader) is the opposite direction from the horizontal vector
                                        1 = Hookline (or end of tangent for a splined leader) is the same direction as horizontal vector (see code 75)
                                    */
    int hookflag {0};              /*!< Hook line flag, code 75 */
                                      /* Hookline flag:
                                        0 = No hookline
                                        1 = Has a hookline*/

    double textheight {0.0};         /*!< Text annotation height, code 40 */
    double textwidthX{0.0};          /*!< Text annotation width, code 41 */
    int vertnum {0};                 /*!< Number of vertices, code 76 */
    int coloruse {0};              /*!< Color to use if leader's DIMCLRD = BYBLOCK, code 77 */
    /* + add support of
      code  340 Hard reference to associated annotation (mtext, tolerance, or insert entity)
      */

    RS_Vector horizontalDirection; /*211 “Horizontal” direction for leader, DXF: X value; APP: 3D vector
                                    221, 231 DXF: Y and Z values of “horizontal” direction for leader*/
    RS_Vector offsetOfLastLeaderVertexFromAnnotationPlacementPoint; /*213 Offset of last leader vertex from annotation placement point
                                    DXF: X value; APP: 3D vector
                                    223, 233 DXF: Y and Z values of offset*/
    QList<RS_Vector> vertexes;
};

/**
 * Class for a leader entity (kind of a polyline arrow).
 *
 * @author Andrew Mustun
 */
// fixme - sand - this is just temporary implementation of LEADER. Support other features - hook line etc (even despite that it is obsolete).
// fixme - sand - add support of MLeader
class RS_Leader : public /*RS_EntityContainer*/ RS_Dimension {
public:
    explicit RS_Leader(RS_EntityContainer* parent = nullptr);
    RS_Leader(RS_EntityContainer* parent, const RS_LeaderData& d);

    RS_Entity* clone() const override;

    /**	@return RS2::EntityDimLeader */
    RS2::EntityType rtti() const override {
        return RS2::EntityDimLeader;
    }

    void update() override;

    /** @return Copy of data that defines the leader. */
    RS_LeaderData getData() const {
        return m_data;
    }

    /** @return true: if this leader has an arrow at the beginning. */
    bool hasArrowHead() const {
        return m_data.arrowHead;
    }

    void setHasArrwoHead(bool val) {
        m_data.arrowHead = val;
        update();
    }

    RS_Entity* addVertex(const RS_Vector& v);
    void addEntity(const RS_Entity* entity) override;

    //	double getLength() const {
    //		return -1.0;
    //	}

    QString getMeasuredLabel() override {
        return "";
    } // fixme - sand - review
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    void stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) override;

    friend std::ostream& operator <<(std::ostream& os, const RS_Leader& l);
    RS_VectorSolutions getRefPoints() const override;
    RS_Entity* doAddVertex(const RS_Vector& v);

    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset) override;
    int getVertexesCount() const {return m_data.vertexes.size();}
    RS_Vector getVertexAt(const int i) {return m_data.vertexes[i];}

protected:
    RS_LeaderData m_data;
    bool m_empty = true;
    void doUpdateDim() override;
    void addArrowHead(const RS_Vector& start, const RS_Vector& end);
};

#endif
