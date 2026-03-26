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

#ifndef RS_ENTITYCONTAINER_H
#define RS_ENTITYCONTAINER_H

#include <QList>

#include "rs_entity.h"

/**
 * Class representing a tree of entities.
 * Typical entity containers are graphics, polylines, groups, texts, ...)
 *
 * @author Andrew Mustun
 */
class RS_EntityContainer : public RS_Entity {
public:
    using value_type = RS_Entity*;

    struct RefInfo {
        RS_Vector ref{false};
        RS_Entity* entity = nullptr;
    };

    explicit RS_EntityContainer(RS_EntityContainer* parent = nullptr, bool owner = true);
    RS_EntityContainer(const RS_EntityContainer& other);
    RS_EntityContainer(const RS_EntityContainer& other, bool copyChildren);
    RS_EntityContainer& operator =(const RS_EntityContainer& other);
    RS_EntityContainer(RS_EntityContainer&& other) noexcept;
    RS_EntityContainer& operator =(RS_EntityContainer&& other) noexcept;
    //RS_EntityContainer(const RS_EntityContainer& ec);

    ~RS_EntityContainer() override;

    RS_Entity* clone() const override;
    virtual void detach();

    /** @return RS2::EntityContainer */
    RS2::EntityType rtti() const override {
        return RS2::EntityContainer;
    }

    void reparent(RS_EntityContainer* newParent) override;

    /**
     * @return true: because entities made from this class
  *         and subclasses are containers for other entities.
     */
    bool isContainer() const override {
        return true;
    }

    /**
     * @return false: because entities made from this class
     *         and subclasses are containers for other entities.
     */
    bool isAtomic() const override {
        return false;
    }

    double getLength() const override;
    void setVisible(bool v) override;
    void setHighlighted(bool on) override;
    void setSelectionFlag(bool select) override;
    bool doSelectInDocument(bool select, RS_Document* doc) override;
    /*virtual void selectWindow(RS_Vector v1, RS_Vector v2,
   bool select=true, bool cross=false);*/
    // virtual void selectWindow(enum RS2::EntityType typeToSelect, RS_Vector v1, RS_Vector v2,
    // bool select=true, bool cross=false);
    // virtual void selectWindow(const QList<RS2::EntityType> &typesToSelect, RS_Vector v1, RS_Vector v2,
    // bool select=true, bool cross=false);
    virtual void addEntity(const RS_Entity* entity);
    virtual void appendEntity(RS_Entity* entity);
    virtual void prependEntity(RS_Entity* entity);
    virtual void moveEntity(int index, QList<RS_Entity*>& entList);
    void adjustBordersIfNeeded(const RS_Entity* entity);
    virtual void insertEntity(int index, RS_Entity* entity);
    virtual bool removeEntity(RS_Entity* entity);

    //!
    //! \brief addRectangle add four lines to form a rectangle by
    //! the diagonal vertices v0,v1
    //! \param v0,v1 diagonal vertices of the rectangle
    //!
    void addRectangle(const RS_Vector& v0, const RS_Vector& v1);
    void addRectangle(const RS_Vector& v0, const RS_Vector& v1, const RS_Vector& v2, const RS_Vector& v3);

    virtual RS_Entity* firstEntity(RS2::ResolveLevel level = RS2::ResolveNone) const;
    virtual RS_Entity* lastEntity(RS2::ResolveLevel level = RS2::ResolveNone) const;
    virtual RS_Entity* nextEntity(RS2::ResolveLevel level = RS2::ResolveNone) const;
    virtual RS_Entity* prevEntity(RS2::ResolveLevel level = RS2::ResolveNone) const;
    virtual RS_Entity* entityAt(int index) const;
    virtual void setEntityAt(int index, RS_Entity* en);
    virtual int findEntity(const RS_Entity* entity);
    int findEntityIndex(const RS_Entity* entity) const;
    bool areNeighborsEntities(const RS_Entity* e1, const RS_Entity* e2) const;
    virtual void clear();

    //virtual unsigned long int count() {
    // return count(false);
    //}
    virtual bool isEmpty() const {
        return count() == 0;
    }

    bool empty() const {
        return isEmpty();
    }

    unsigned count() const override;
    unsigned countDeep() const override;

    size_t size() const {
        return m_entities.size();
    }

    /**
     * Enables / disables automatic update of borders on entity removals
     * and additions. By default this is turned on.
     */
    void setAutoUpdateBorders(const bool enable) {
        m_autoUpdateBorders = enable;
    }

    bool getAutoUpdateBorders() const {
        return m_autoUpdateBorders;
    }

    virtual void adjustBorders(const RS_Entity* entity);
    void calculateBorders() override;
    void forcedCalculateBorders();
    int updateDimensions(bool autoText = true);
    int updateVisibleDimensions(bool autoText = true);
    virtual void updateInserts();
    virtual void updateSplines();
    void update() override;
    virtual void renameInserts(const QString& oldName, const QString& newName);
    RS_Entity* getNearestEntity(const RS_Vector& coord, double* dist = nullptr, RS2::ResolveLevel level = RS2::ResolveAll) const;
    RS_Vector getNearestIntersection(const RS_Vector& coord, double* dist = nullptr, RS_Entity** entity = nullptr, RS_Entity** otherEntity=nullptr) const;
    RS_Vector getNearestVirtualIntersection(const RS_Vector& coord, double angle, double* dist) const;
    virtual RefInfo getNearestSelectedRefInfo(const RS_Vector& coord, double* dist = nullptr) const;
    virtual bool optimizeContours();
    bool hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2) const override;
    void move(const RS_Vector& offset) override;
    void rotate(const RS_Vector& center, double angle) override;
    void rotate(const RS_Vector& center, const RS_Vector& angleVector) override;
    void scale(const RS_Vector& center, const RS_Vector& factor) override;
    void mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) override;
    RS_Entity& shear(double k) override;

    void stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) override;
    void calculateBordersIfNeeded();
    void moveRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset) override;
    void revertDirection() override;

    void draw(RS_Painter* painter) override;

    friend std::ostream& operator <<(std::ostream& os, RS_EntityContainer& ec);

    bool isOwner() const {
        return m_autoDelete;
    }

    void setOwner(const bool owner) {
        m_autoDelete = owner;
    }

    /**
     * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
     * Contour Area =\oint x dy
     * @return line integral \oint x dy along the entity
     * returns absolute value
     */
    double areaLineIntegral() const override;
    /**
  * @brief ignoreForModification ignore this entity for entity catch for certain actions
     * like catching circles to create tangent circles
     * @return, true, indicate this entity container should be ignored
     */
    bool ignoredOnModification() const;

    void push_back(RS_Entity* entity) {
        m_entities.push_back(entity);
    }

    void pop_back() {
        if (!isEmpty()) {
            m_entities.pop_back();
        }
    }

    /**
     * @brief begin/end to support range based loop
     * @return iterator
     */
    QList<RS_Entity*>::const_iterator begin() const;
    QList<RS_Entity*>::const_iterator end() const;
    QList<RS_Entity*>::const_iterator cbegin() const;
    QList<RS_Entity*>::const_iterator cend() const;
    QList<RS_Entity*>::iterator begin();
    QList<RS_Entity*>::iterator end();
    //! \{
    //! first and last without resolving into children, assume the container is
    //! not empty
    RS_Entity* last() const;
    RS_Entity* first() const;
    //! \}

    const QList<RS_Entity*>& getEntityList() const;

    RS_Entity* unsafeEntityAt(const int index) const {
        return m_entities.at(index);
    }

    void drawAsChild(RS_Painter* painter) override;
    RS_Entity* cloneProxy() const override;

protected:
    /**
     * @brief getLoops for hatch, split closed loops into single simple loops. All returned containers are owned by
     * the returned object.
     * @return std::vector<std::unique_ptr<RS_EntityContainer>> - each container contains edge entities of a single
     * closed loop. Each loop is assumed to be simply closed, and loops never cross each other.
     */
    virtual std::vector<std::unique_ptr<RS_EntityContainer>> getLoops() const;

    bool setSelected(bool select) override;

    RS_Vector doGetNearestPointOnEntity(const RS_Vector& coord, bool onEntity, double* dist, RS_Entity** entity) const override;
    double doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, RS2::ResolveLevel level, double solidDist) const override;
    RS_Vector doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const override;
    RS_Vector doGetNearestRef(const RS_Vector& coord, double* dist) const override;
    RS_Vector doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const override;
    RS_Vector doGetNearestMiddle(const RS_Vector& coord, double* dist, int middlePoints) const override;
    RS_Vector doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const override;
    RS_Vector obtainNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** pEntity) const;
    RS_Vector doGetNearestDist(double distance, const RS_Vector& coord, double* dist) const override;
    /** sub container used only temporarily for iteration. */
    mutable RS_EntityContainer* m_subContainer = nullptr;

private:
    /**
     * @brief ignoredSnap whether snapping is ignored
     * @return true when entity of this container won't be considered for snapping points
     */
    bool ignoredSnap() const;

    void debugEntityAlreadyPresentExists(const RS_Entity* entity) const;

    /** m_entities in the container */
    QList<RS_Entity*> m_entities;
    /**
     * Automatically update the borders of the container when entities
     * are added or removed.
     */
    bool m_autoUpdateBorders = true;
    mutable int m_entIdx = 0;
    bool m_autoDelete = false;
};

#endif
