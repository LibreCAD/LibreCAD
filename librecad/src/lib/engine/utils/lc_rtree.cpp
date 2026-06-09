/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**

** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/


#include "lc_rtree.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/numeric/ublas/matrix.hpp>

#include "lc_rect.h"
#include "rs.h"
#include "rs_vector.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using BPoint = bg::model::point<double, 2, bg::cs::cartesian> ;
using BBox = bg::model::box<BPoint>;
using TreeValue = std::pair<BBox, unsigned>;

namespace lc {

namespace geo {

struct RTree::RTreeImpl: bgi::rtree< TreeValue, bgi::quadratic<16> >
{
    RTreeImpl(const double tolerance):
        m_tolerance{tolerance},
        m_halfBox{0.5 * m_tolerance, 0.5 * m_tolerance}
    {}

    BPoint toPoint(const RS_Vector& point) const
    {
        return {point.x, point.y};
    }

    static RS_Vector toPoint(const BBox& box)
    {
        const auto min = box.min_corner();
        const auto max = box.max_corner();
        const auto center = RS_Vector{min.get<0>()+max.get<0>(), min.get<1>()+max.get<1>() } * 0.5;
        return center;
    }

    BBox toBox(const RS_Vector& point) const
    {
        return {toPoint(point - m_halfBox), toPoint(point + m_halfBox)};
    }

    BBox toBox(const LC_Rect& rect) const
    {
        return {toPoint(rect.lowerLeftCorner()), toPoint(rect.upperRightCorner())};
    }

    RS_VectorSolutions Intersects(const BBox& box) const
    {
        std::vector<TreeValue> result_s;
        query(bgi::intersects(box), std::back_inserter(result_s));
        RS_VectorSolutions ret;
        for(const auto& [bbox, index]: result_s) {
            ret.push_back(toPoint(bbox));
        }
        return ret;
    }

    double m_tolerance = RS_TOLERANCE;
    RS_Vector m_halfBox{false};
};

/**
 * @brief Provide the R-Tree container
 *        R-Tree is an efficient container with spatial indexing
 *        The current implementation only uses a value pair of {Rect, index}
 *        with the Rect as a bounding box and index an unsigned number
 *        as the counting number of nodes.
 *        The implemented query methods:
 *          NearestNeighbors: return all nodes intersect a tolerance sized box
 *          PointsInBox: return all node box centers intersect a given box
 * @author Dongxu Li
 */
/**
     * @brief RTree Constructor of a RTree for efficient point look up
     * @param toleranceSize - the default box size to use to find nearest points,
     *                        i.e. the node box centers
     */
RTree::RTree(double toleranceSize):
    m_pRTree{std::make_unique<RTreeImpl>(toleranceSize)}
{}

/**
     * @brief RTree Constructor of a RTree for efficient point look up
     * @param points - boxes to construct boxes with the tolerance size
     * @param toleranceSize - the default box size to use to find nearest points,
     *                        i.e. the node box centers
     */
RTree::RTree(const RS_VectorSolutions& points, double toleranceSize):
    m_pRTree{std::make_unique<RTreeImpl>(toleranceSize)}{
    for(const RS_Vector& point: points) {
        Insert(point);
    }
}



/**
 * @brief Insert a point the R-Tree container
 * @return bool - true, if successful; false, if a coincident point is already in the the container
 * @author Dongxu Li
 */
bool RTree::Insert(const RS_Vector& point) const {
    // create a box
    BBox b = m_pRTree->toBox(point);
    // insert new value
    m_pRTree->insert({b, m_pRTree->size()});
    return true;
}
/**
     * @brief Insert insert a new box
     * @param area box
     * @return
     */
bool RTree::Insert(const Area& area) const {
    // create a box
    BBox b = m_pRTree->toBox(area);
    // insert new value
    m_pRTree->insert(std::make_pair(b, m_pRTree->size()));
    return true;
}

RS_VectorSolutions RTree::nearestNeighbors(const RS_Vector& point) const
{
    // convert the point to a box with side length by the default tolerance
    const BBox box = m_pRTree->toBox(point);
    // find values intersecting some area defined by a box
    return m_pRTree->Intersects(box);
}

RS_VectorSolutions RTree::pointsInBox(const Area& area) const
{
    // find values intersecting some area defined by a box
    const BBox box = m_pRTree->toBox(area);
    return m_pRTree->Intersects(box);
}

} // namespace geo
} // namespace lc
//EOF
