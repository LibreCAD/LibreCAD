/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef RS_ENTITY_H
#define RS_ENTITY_H

#include <QString>

#include "lc_drawable.h"
#include "lc_secondmoment.h"
#include "rs_undoable.h"
#include "rs_vector.h"

class RS_Pen;
class RS_Line;
class RS_Layer;
class RS_Document;
class RS_Insert;
class RS_Block;
class RS_Graphic;
class RS_EntityContainer;
class LC_Quadratic;

/**
 * Base class for an entity (line, arc, circle, ...)
 *
 * @author Andrew Mustun
 */
class RS_Entity : public RS_Undoable, public LC_Drawable {
public:
    explicit RS_Entity(RS_EntityContainer* parent = nullptr);
    // RS_Entity(RS_EntityContainer *parent, bool setPenToActive = false);
    RS_Entity(const RS_Entity& other);
    RS_Entity& operator =(const RS_Entity& other);
    RS_Entity(RS_Entity&& other) noexcept;
    RS_Entity& operator =(RS_Entity&& other) noexcept;
    ~RS_Entity() override;

    virtual RS_Entity* clone() const = 0;
    virtual RS_Entity* cloneProxy() const;

    virtual void reparent(RS_EntityContainer* parent) {
        m_parent = parent;
    }

    void resetBorders();
    void moveBorders(const RS_Vector& offset);
    void scaleBorders(const RS_Vector& center, const RS_Vector& factor);

    /**
     * Identify all entities as undoable entities.
     * @return RS2::UndoableEntity
     */
    RS2::UndoableType undoRtti() const override {
        return RS2::UndoableEntity;
    }

    /**
     * @return Unique Id of this entity.
     */
    unsigned long long getId() const;

    /**
     * This method must be overwritten in subclasses and return the
     * number of <b>atomic</b> entities in this entity.
     */
    virtual unsigned int count() const = 0;
    /**
     * This method must be overwritten in subclasses and return the
     * number of <b>atomic</b> entities in this entity including sub containers.
     */
    virtual unsigned int countDeep() const = 0;

    /**
         * Implementations must return the total length of the entity
         * or a negative number if the entity has no length (e.g. a text or hatch).
         */
    virtual double getLength() const {
        return -1.0;
    }

    /**
     * @return Parent of this entity or nullptr if this is a root entity.
     */
    RS_EntityContainer* getParent() const {
        return m_parent;
    }

    /**
     * Reparents this entity.
     */
    void setParent(RS_EntityContainer* p) {
        m_parent = p;
    }

    /** @return The center point (x) of this arc */
    //get center for entities arc, circle and ellipse
    virtual RS_Vector getCenter() const;
    virtual double getRadius() const;
    virtual void setRadius(double r);
    virtual RS_Graphic* getGraphic() const;
    RS_Block* getBlock() const;
    RS_Insert* getInsert() const;
    RS_Entity* getBlockOrInsert() const;
    RS_Document* getDocument() const;
    void setLayer(const QString& name);
    void setLayer(RS_Layer* l);
    void setLayerToActive();
    void setPenAndLayerToActive();
    RS_Layer* getLayer(bool resolve = true) const;
    RS_Layer* getLayerResolved() const;

    /**
     * Sets the explicit pen for this entity or a pen with special
     * attributes such as BY_LAYER, ..
     */
    void setPen(const RS_Pen& pen) const;

    void setPenToActive() const;
    RS_Pen getPen(bool resolve = true) const;
    RS_Pen getPenResolved() const;
    /**
     * Must be overwritten to return true if an entity type
     * is a container for other entities (e.g. polyline, group, ...).
     */
    virtual bool isContainer() const = 0;
    /**
     * Must be overwritten to return true if an entity type
     * is an atomic entity.
     */
    virtual bool isAtomic() const = 0;

    /**
     * Must be overwritten to return true if an entity type
     * is a potential edge entity of a contour. By default,
    * this returns false.
     */
    virtual bool isEdge() const {
        return false;
    }

    /**
     * @return true for all document entities (e.g. Graphics or Blocks).
     * false otherwise.
     */
    virtual bool isDocument() const {
        return false;
    }

    virtual void setSelectionFlag(bool select);
    void clearSelectionFlag();
    bool isSelected() const;
    bool isParentSelected() const;
    virtual bool isProcessed() const;
    virtual void setProcessed(bool on);
    bool isInWindow(const RS_Vector& v1, const RS_Vector& v2) const;

    virtual bool hasEndpointsWithinWindow(const RS_Vector& /*v1*/, const RS_Vector& /*v2*/) const {
        return false;
    }

    virtual bool isVisible() const;

    bool isInvisible() const {
        return !isVisible();
    }

    virtual void setVisible(bool v);
    virtual void setHighlighted(bool on);
    virtual bool isHighlighted() const;
    bool isTransparent() const;
    void setTransparent(bool on);
    bool isLocked() const;
    bool isEditable() const;
    void deletedStateChanged(bool undone) override;
    bool isDeleted() const override;

    /**
     * Can be implemented by child classes to update the entities
     * temporary subentities. update() is called if the entity's
     * parameters or undo state changed.
     */
    virtual void update() {
    }

    virtual void setUpdateEnabled(const bool on) {
        m_updateEnabled = on;
    }

    /**
     * This method doesn't do any calculations.
     * @return minimum coordinate of the entity.
     * @see calculateBorders()
     */
    RS_Vector getMin() const {
        return m_minV;
    }

    /**
     * This method doesn't do any calculations.
     * @return minimum coordinate of the entity.
     * @see calculateBorders()
     */
    RS_Vector getMax() const {
        return m_maxV;
    }

    /**
     * This method returns the difference of max and min returned
     * by the above functions.
     * @return size of the entity.
     * @see calculateBorders()
     * @see getMin()
     * @see getMax()
     */
    RS_Vector getSize() const;
    void addGraphicVariable(const QString& key, double val, int code) const;
    void addGraphicVariable(const QString& key, int val, int code) const;
    void addGraphicVariable(const QString& key, const QString& val, int code) const;
    double getGraphicVariableDouble(const QString& key, double def) const;
    int getGraphicVariableInt(const QString& key, int def) const;
    QString getGraphicVariableString(const QString& key, const QString& def) const;
    virtual RS_Vector getStartpoint() const;
    virtual RS_Vector getEndpoint() const;

    //find the local direction at end points, derived entities
    // must implement this if direction is supported by the entity type
    virtual double getDirection1() const {
        return 0.;
    }

    virtual double getDirection2() const {
        return 0.;
    }

    //find the tangential points seeing from given point
    virtual RS_VectorSolutions getTangentPoint(const RS_Vector& /*point*/) const;
    virtual RS_Vector getTangentDirection(const RS_Vector& /*point*/) const;
    RS2::Unit getGraphicUnit() const;
    /**
     * Must be overwritten to get all reference points of the entity.
     */
    virtual RS_VectorSolutions getRefPoints() const;

    RS_Vector getNearestEndpoint(const RS_Vector& coord, RS_Entity** entity = nullptr, double* dist = nullptr) const {
        return doGetNearestEndpoint(coord, dist, entity);
    }

    RS_Vector getNearestPointOnEntity(const RS_Vector& coord, const bool onEntity = true, double* dist = nullptr,
                                      RS_Entity** entity = nullptr) const {
        return doGetNearestPointOnEntity(coord, onEntity, dist, entity);
    }

    RS_Vector getNearestCenter(const RS_Vector& coord, double* dist = nullptr, RS_Entity** centerEntity = nullptr) const {
        return doGetNearestCenter(coord, dist, centerEntity);
    }


    virtual RS_Vector getMiddlePoint() const {
        return RS_Vector(false);
    }

    /**
     * Must be overwritten to get the (nearest) middle point to the
     * given coordinate for this entity.
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest middle point. The passed
     * pointer can also be nullptr in which case the distance will be
     * lost.
     * @param middlePoints
     *
     * @return The closest middle point.
     */
    RS_Vector getNearestMiddle(const RS_Vector& coord, double* dist = nullptr, const int middlePoints = 1) const {
        return doGetNearestMiddle(coord, dist, middlePoints);
    }

    RS_Vector getNearestDist(const double distance, const RS_Vector& coord, double* dist = nullptr) const {
        return doGetNearestDist(distance, coord, dist);
    }

    /**
     * Must be overwritten to get the point with a given
     * distance to the start- or endpoint to the given coordinate for this entity.
     *
     * @param distance Distance to endpoint.
     * @param startp true = measured from Startpoint, false = measured from Endpoint
     *
     * @return The point with the given distance to the start- or endpoint.
     */
    virtual RS_Vector getNearestDistToEndpoint([[maybe_unused]]double distance, [[maybe_unused]]bool startp) const {
        return RS_Vector(false);
    }

    /**
     * @brief dualLineTangentPoint find the tangent point for a line in line coordinates
     * @param line a tangent line in line coordinates
     * @return the tangent point for the line
     */
    virtual RS_Vector dualLineTangentPoint([[maybe_unused]] const RS_Vector& line) const {
        return RS_Vector{false};
    }

    RS_Vector getNearestRef(const RS_Vector& coord, double* dist = nullptr) const {
        return doGetNearestRef(coord, dist);
    }

    RS_Vector getNearestSelectedRef(const RS_Vector& coord, double* dist = nullptr) const {
        return doGetNearestSelectedRef(coord, dist);
    }

    virtual RS_Vector getNearestOrthTan(const RS_Vector& coord, const RS_Line& normal, bool onEntity) const;

    /**
    * Must be overwritten to get the shortest distance between this
    * entity and a coordinate.
    *
    * @param coord Coordinate (typically a mouse coordinate)
    * @param entity Pointer which will contain the (sub-)entity which is
    *               closest to the given point or nullptr if the caller is not
    *               interested in this information.
    * @param level The resolve level.
    * @param solidDist
    *
    * @sa RS2::ResolveLevel
    *
    * @return The measured distance between \p coord and the entity.
    */
    double getDistanceToPoint(const RS_Vector& coord, RS_Entity** entity = nullptr, const RS2::ResolveLevel level = RS2::ResolveNone,
                              const double solidDist = RS_MAXDOUBLE) const {
        return doGetDistanceToPoint(coord, entity, level, solidDist);
    }

    bool isPointOnEntity(const RS_Vector& coord, const double tolerance = 20. * RS_TOLERANCE) const {
        return doIsPointOnEntity(coord, tolerance);
    }

    /**
     * Implementations must offset the entity by the given direction and distance.
     */
    virtual bool offset(const RS_Vector& /*coord*/, double) {
        return false;
    }

    /**
     * Produce offset copies for entities whose offset cannot be expressed as
     * an in-place mutation of the same type (e.g. ellipses → splines).
     * Default returns empty so callers fall back to the in-place offset() path.
     * Caller takes ownership of returned entities.
     */
    virtual std::vector<RS_Entity *> createOffset(const RS_Vector & /*coord*/,
                                                  const double & /*distance*/) const {
        return std::vector<RS_Entity *>();
    }

    /**
     * Implementations must offset the entity by the distance at both directions
     * used to generate tangential circles
     */
    virtual std::vector<RS_Entity*> offsetTwoSides(double) const {
        return std::vector<RS_Entity*>();
    }

    /**
          * implementations must revert the direction of an atomic entity
          */
    virtual void revertDirection() {
    }

    /**
     * Implementations must move the entity by the given vector.
     */
    virtual void move(const RS_Vector& offset) = 0;
    /**
     * Implementations must rotate the entity by the given angle around
     * the given center.
     */
    virtual void rotate(const RS_Vector& center, double angle) = 0;
    virtual void rotate(const RS_Vector& center, const RS_Vector& angleVector) = 0;
    /**
     * Implementations must scale the entity by the given factors.
     */
    virtual void scale(const RS_Vector& center, const RS_Vector& factor) = 0;

    /**
     * Acts like scale(RS_Vector) but with equal factors.
     * Equal to scale(center, RS_Vector(factor, factor)).
     */
    virtual void scale(const RS_Vector& center, const double factor) {
        scale(center, RS_Vector(factor, factor));
    }

    virtual void scale(const RS_Vector& factor) {
        scale(RS_Vector(0., 0.), factor);
    }

    /**
     * Implementations must mirror the entity by the given axis.
     */
    virtual void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) = 0;
    virtual void stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset);
    /**
     * @description:    Implementation of the Shear/Skew the entity
     *                  The shear transform is
     *                  1  k  0
     *                  0  1  0
     *                        1
     * @author          Dongxu Li
     * @param k the skew/shear parameter
     */
    virtual RS_Entity& shear(double k) = 0;

    /**
     * @description:    Implementation of the Shear/Skew the entity
     *                  The shear transform is
     *                  1  k  0
     *                  0  1  0
     *                        1
     * @author          Dongxu Li
     * @param origin origin the point to be used as the origin
     * @param xAxis the x-axis direction
     * @param k the skew/shear parameter
     */
    virtual RS_Entity& shear(const RS_Vector& origin, const RS_Vector& xAxis, const double k) {
        rotate(origin, -xAxis.angle());
        move(-origin);
        shear(k);
        move(origin);
        rotate(origin, xAxis.angle());
        return *this;
    }

    /**
         * Implementations must drag the reference point(s) of all
         * (sub-)entities that are very close to ref by offset.
         */
    virtual void moveRef(const RS_Vector& /*ref*/, const RS_Vector& /*offset*/) {
    }

    /**
         * Implementations must drag the reference point(s) of selected
         * (sub-)entities that are very close to ref by offset.
         */
    virtual void moveSelectedRef(const RS_Vector& /*ref*/, const RS_Vector& /*offset*/) {
    }

    virtual void drawAsChild(RS_Painter* painter) {
        draw(painter);
    }

    virtual void drawDraft(RS_Painter* painter) {
        draw(painter);
    }

    //    double getStyleFactor(RS_GraphicView *view);
    QString getUserDefVar(const QString& key) const;
    std::vector<QString> getAllKeys() const;
    void setUserDefVar(QString key, QString val) const;
    void delUserDefVar(const QString& key) const;
    friend std::ostream& operator<<(std::ostream& os, RS_Entity& e);
    /** Recalculates the borders of this entity. */
    virtual void calculateBorders() = 0;
    /** whether the entity is on a constructionLayer */
    //! constructionLayer contains entities of infinite length, constructionLayer doesn't show up in print
    bool isConstruction(bool typeCheck = false) const; // ignore certain entity types for constructionLayer check
    //! whether printing is enabled or disabled for the entity's layer
    bool isPrint() const;
    /** return the equation of the entity
for quadratic,

return a vector contains:
m0 x^2 + m1 xy + m2 y^2 + m3 x + m4 y + m5 =0

for linear:
m0 x + m1 y + m2 =0
**/
    virtual LC_Quadratic getQuadratic() const;
    /**
     * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
     * Contour Area =\oint x dy
     * @return line integral \oint x dy along the entity
     */
    virtual double areaLineIntegral() const;

    /**
     * @brief firstMomentLineIntegral - computes the first-order moments of area
     *        via Green's theorem contour integrals.
     *
     * Returns:
     *   mx = ∬ x dA   (first moment with respect to y-axis)
     *   my = ∬ y dA   (first moment with respect to x-axis)
     *
     * These values are used to compute the centroid: cx = mx / A, cy = my / A.
     *
     * @return LC_FirstMoment containing mx and my.
     */
    virtual LC_FirstMoment firstMomentLineIntegral() const;

    /**
     * @brief secondMomentLineIntegral - Green's theorem line integrals for the
     * three second moments of area (∬ x² dA, ∬ y² dA, ∬ x·y dA).
     *
     * For a closed contour traversed counter-clockwise, summing the contributions
     * of all boundary entities gives:
     *   ixx = ∮ (x³/3) dy       →  ∬ x² dA
     *   iyy = -∮ (y³/3) dx      →  ∬ y² dA
     *   ixy = ∮ (x²·y/2) dy     →  ∬ x·y dA
     *
     * @return LC_SecondMoment containing the three line-integral contributions.
     */
    virtual LC_SecondMoment secondMomentLineIntegral() const;
    /**
     * @brief trimmable, whether the entity type can be trimmed
     * @return true, for trimmable entity types
     * currently, trimmable types are: RS_Line, RS_Circle, RS_Arc, RS_Ellipse
     */
    bool trimmable() const;
    /**
     * @brief isArc is the entity of type Arc, Circle, or Ellipse
     * @return true for Arc, Circle, or Ellipse
     */
    virtual bool isArc() const;
    /**
     * @brief isArcLine determine the entity is either Arc, Circle, or Line
     * @return true if entity is Arc, Circle, or Line
     */
    virtual bool isArcCircleLine() const;
    bool isParentIgnoredOnModifications() const;

protected:
    //! Entity's parent entity or nullptr is this entity has no parent.
    mutable RS_EntityContainer* m_parent = nullptr;
    //! minimum coordinates
    RS_Vector m_minV;
    //! maximum coordinates
    RS_Vector m_maxV;
    //! Pointer to layer
    RS_Layer* m_layer = nullptr;
    //! auto updating enabled?
    bool m_updateEnabled = false;

    void init(bool updatePenAndLayerToActive);
    void initId();

    void addToSelectionSet(bool select, const RS_Document* doc);
    virtual bool doSelectInDocument(bool select, RS_Document* doc);
    virtual bool setSelected(bool select);
    /**
     * Must be overwritten to get the closest coordinate to the
    * given coordinate which is on this entity.
     *
     * @param onEntity
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between \p coord and the point. The passed pointer can
     * also be \p nullptr in which case the distance will be lost.
     * @param entity
     *
     * @return The closest coordinate.
     */
    virtual RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const = 0;

    virtual bool doIsPointOnEntity(const RS_Vector& coord, double tolerance) const;
    virtual double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level, double solidDist) const;
    /**
     * Must be overwritten to get the closest endpoint to the
     * given coordinate for this entity.
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest endpoint. The passed
     * pointer can also be nullptr in which case the distance will be
     * lost.
     * @param entity
     * @param entity
     *
     * @return The closest endpoint.
     */
    virtual RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const = 0;

    /**
     * Must be overwritten to get the nearest reference point for this entity.
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest point. The passed
     * pointer can also be nullptr in which case the distance will be
     * lost.
     *
     * @return The closest point with the given distance to the endpoint.
     */
    virtual RS_Vector doGetNearestRef(const RS_Vector& coord, double* dist) const;

    /**
     * Must be overwritten to get the (nearest) center point to the
     * given coordinate for this entity.
     *
     * @param coord Coordinate (typically a mouse coordinate)
     * @param dist Pointer to a value which will contain the measured
     * distance between 'coord' and the closest center point. The passed
     * pointer can also be nullptr in which case the distance will be
     * lost.
     * @param entity
     * @param entity
     *
     * @return The closest center point.
     */
    virtual RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** entity) const = 0;
    virtual RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const = 0;
    /**
    * Gets the nearest reference point of this entity if it is selected.
        * Containers re-implement this method to return the nearest reference
        * point of a selected sub entity.
    *
    * @param coord Coordinate (typically a mouse coordinate)
    * @param dist Pointer to a value which will contain the measured
    * distance between 'coord' and the closest point. The passed
    * pointer can also be nullptr in which case the distance will be
    * lost.
    *
    * @return The closest point with the given distance to the endpoint.
    */
    virtual RS_Vector doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const;

    /**
 * Must be overwritten to get the nearest point with a given
 * distance to the endpoint to the given coordinate for this entity.
 *
 * @param distance Distance to endpoint.
 * @param coord Coordinate (typically a mouse coordinate)
 * @param dist Pointer to a value which will contain the measured
 * distance between 'coord' and the closest point. The passed
 * pointer can also be nullptr in which case the distance will be
 * lost.
 *
 * @return The closest point with the given distance to the endpoint.
 */
    virtual RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const = 0;

private:
    //! Entity m_id
    unsigned long long m_id = 0;
    // pImp to delay pulling in Qt headers
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;

    friend class RS_Document;
};
#endif
