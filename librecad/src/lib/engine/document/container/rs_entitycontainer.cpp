/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 Dongxu Li (github.com/dxli)
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

#include "rs_entitycontainer.h"

#include <iostream>

#include "lc_containertraverser.h"
#include "lc_looputils.h"
#include "qg_dialogfactory.h"
#include "rs_constructionline.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dimension.h"
#include "rs_ellipse.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_painter.h"
#include "rs_solid.h"
#include "rs_vector.h"

class RS_Dimension;

namespace {
    // the tolerance used to check topology of contours in hatching
    constexpr double CONTOUR_TOLERANCE = 1e-8;  // fixme - sand - to options?

    // For validate hatch contours, whether an entity in the contour is a closed
    // loop itself
    bool isClosedLoop(RS_Entity& entity) {
        switch (entity.rtti()) {
            case RS2::EntityCircle:
            // Sub containers are always closed
            case RS2::EntityContainer:
                return true;
            case RS2::EntityEllipse:
                return !static_cast<RS_Ellipse*>(&entity)->isArc();
            default:
                return false;
        }
    }

    // Find the nearest distance between the endpoints of an entity to a given point
    double endPointDistance(const RS_Vector& point, const RS_Entity& entity) {
        double distance = RS_MAXDOUBLE;
        entity.getNearestEndpoint(point, nullptr, &distance);
        return distance;
    }
}

/**
 * Default constructor.
 *
 * @param parent
 * @param owner True if we own and also delete the entities.
 */
RS_EntityContainer::RS_EntityContainer(RS_EntityContainer* parent, const bool owner) : RS_Entity(parent) {
    m_autoDelete = owner;
    //    RS_DEBUG->print("RS_EntityContainer::RS_EntityContainer: "
    //                    "owner: %d", (int)owner);
    m_subContainer = nullptr;
    //autoUpdateBorders = true;
    m_entIdx = -1;
}

/**
 * Copy constructor. Makes a deep copy of all entities.
 */

RS_EntityContainer::RS_EntityContainer(const RS_EntityContainer& other) : RS_Entity{other}, m_subContainer{other.m_subContainer},
                                                                          m_entities{other.m_entities},
                                                                          m_autoUpdateBorders{other.m_autoUpdateBorders},
                                                                          m_entIdx{other.m_entIdx}, m_autoDelete{other.m_autoDelete} {
    if (m_autoDelete) {
        // fixme - sand - check this logic, looks suspicious!
        for (auto& it : *this) {
            if (it->isContainer()) {
                it = it->clone();
            }
        }
    }
}

RS_EntityContainer::RS_EntityContainer(const RS_EntityContainer& other, const bool copyChildren) : RS_Entity{other} {
    m_subContainer = nullptr;
    m_autoUpdateBorders = other.m_autoUpdateBorders;
    m_entIdx = other.m_entIdx;
    m_autoDelete = other.m_autoDelete;
    if (copyChildren) {
        m_entities = other.m_entities;
        if (m_autoDelete) {
            // fixme - sand - check this logic, looks suspicious!
            for (auto& it : *this) {
                if (it->isContainer()) {
                    it = it->clone();
                }
            }
        }
    }
}

RS_EntityContainer& RS_EntityContainer::operator =(const RS_EntityContainer& other) {
    this->RS_Entity::operator =(other);
    m_subContainer = other.m_subContainer;
    m_entities = other.m_entities;
    m_autoUpdateBorders = other.m_autoUpdateBorders;
    m_entIdx = other.m_entIdx;
    m_autoDelete = other.m_autoDelete;
    if (m_autoDelete) {
        for (auto& it : *this) {
            if (it->isContainer()) {
                it = it->clone();
            }
        }
    }
    return *this;
}

RS_EntityContainer::RS_EntityContainer(RS_EntityContainer&& other) noexcept : RS_Entity{other}, m_subContainer{other.m_subContainer},
                                                                              m_entities{std::move(other.m_entities)},
                                                                              m_autoUpdateBorders{other.m_autoUpdateBorders},
                                                                              m_entIdx{other.m_entIdx}, m_autoDelete{other.m_autoDelete} {
}

RS_EntityContainer& RS_EntityContainer::operator =(RS_EntityContainer&& other) noexcept {
    this->RS_Entity::operator =(other);
    m_subContainer = other.m_subContainer;
    m_entities = std::move(other.m_entities);
    m_autoUpdateBorders = other.m_autoUpdateBorders;
    m_entIdx = other.m_entIdx;
    m_autoDelete = other.m_autoDelete;
    return *this;
}

/**
 * Destructor.
 */
RS_EntityContainer::~RS_EntityContainer() {
    if (m_autoDelete) {
        while (!m_entities.isEmpty()) {
            delete m_entities.takeFirst();
        }
    }
    else {
        m_entities.clear();
    }
}

RS_Entity* RS_EntityContainer::clone() const {
    RS_DEBUG->print("RS_EntityContainer::clone: ori autoDel: %d", m_autoDelete);

    auto* ec = new RS_EntityContainer(getParent(), isOwner());
    if (isOwner()) {
        for (const RS_Entity* entity : std::as_const(m_entities)) {
            if (entity != nullptr) {
                ec->m_entities.push_back(entity->clone());
            }
        }
    }
    else {
        ec->m_entities = m_entities;
    }

    RS_DEBUG->print("RS_EntityContainer::clone: clone autoDel: %d", ec->isOwner());

    ec->detach();
    return ec;
}

RS_Entity* RS_EntityContainer::cloneProxy() const {
    RS_DEBUG->print("RS_EntityContainer::cloneproxy: ori autoDel: %d", m_autoDelete);

    auto* ec = new RS_EntityContainer(getParent(), isOwner());
    if (isOwner()) {
        for (const RS_Entity* entity : std::as_const(m_entities)) {
            if (entity != nullptr) {
                ec->m_entities.push_back(entity->cloneProxy());
            }
        }
    }
    else {
        ec->m_entities = m_entities;
    }

    RS_DEBUG->print("RS_EntityContainer::cloneproxy: clone autoDel: %d", ec->isOwner());

    ec->detach(); // fixme - review whether detach is always need... looks like a double clone() ??
    return ec;
}

/**
 * Detaches shallow copies and creates deep copies of all subentities.
 * This is called after cloning entity containers.
 */
void RS_EntityContainer::detach() {
    QList<RS_Entity*> clonesList;
    const bool autoDel = isOwner();
    RS_DEBUG->print("RS_EntityContainer::detach: autoDel: %d", autoDel);
    setOwner(false);

    // make deep copies of all entities:
    for (const RS_Entity* e : *this) {
        if (!e->getFlag(RS2::FlagTemp)) {
            clonesList.append(e->clone());
        }
    }

    // clear shared pointers:
    clear();
    setOwner(autoDel);

    // point to new deep copies:
    for (RS_Entity* e : clonesList) {
        push_back(e);
        e->reparent(this);
    }
}

void RS_EntityContainer::reparent(RS_EntityContainer* newParent) {
    RS_Entity::reparent(newParent);

    // All sub-entities:
    for (RS_Entity* e : *this) {
        e->reparent(newParent);
    }
}

void RS_EntityContainer::setVisible(const bool v) {
    RS_Entity::setVisible(v);

    // All sub-entities:
    for (const auto e : std::as_const(m_entities)) {
        e->setVisible(v);
    }
}

/**
 * @return Total length of all m_entities in this container.
 */
double RS_EntityContainer::getLength() const {
    double ret = 0.0;

    for (const RS_Entity* e : *this) {
        if (e->isVisible()) {
            const double length = e->getLength();
            if (std::signbit(length)) {
                ret = -1.0;
                break;
            }
            ret += length;
        }
    }

    return ret;
}

/**
 * Selects this entity.
 */
bool RS_EntityContainer::setSelected(const bool select) {
    // layer is locked:
    if (select && isLocked()) {
        return false;
    }
    RS_Entity::setSelectionFlag(select);
    const auto doc = getDocument();
    addToSelectionSet(select, doc);

    // All sub-entity's select:
    for (RS_Entity* e : *this) {
        if (e->isVisible()) {
            e->setSelectionFlag(select);
        }
    }
    return true;
}

bool RS_EntityContainer::doSelectInDocument(const bool select, RS_Document* doc) {
    if (select && isLocked()) {
        return false;
    }
    RS_Entity::setSelectionFlag(select);
    addToSelectionSet(select, doc);

    // All sub-entity's select:
    for (RS_Entity* e : *this) {
        if (e->isVisible()) {
            e->setSelectionFlag(select);
        }
    }
    return true;
}

void RS_EntityContainer::setSelectionFlag(const bool select) {
    // layer is locked:
    if (select && isLocked()) {
        return;
    }
    RS_Entity::setSelectionFlag(select);
    // All sub-entity's select:
    for (RS_Entity* e : *this) {
        if (e->isVisible()) {
            e->setSelectionFlag(select);
        }
    }
}

void RS_EntityContainer::setHighlighted(const bool on) {
    for (RS_Entity* e : *this) {
        e->setHighlighted(on);
    }
    RS_Entity::setHighlighted(on);
}

/**
 * Adds a entity to this container and updates the borders of this
 * entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::addEntity(const RS_Entity* entity) {
    if (entity == nullptr) {
        return;
    }
    debugEntityAlreadyPresentExists(entity);
    const auto ent = const_cast<RS_Entity*>(entity);
    if (entity->rtti() == RS2::EntityImage || entity->rtti() == RS2::EntityHatch) {
        m_entities.prepend(ent);
    }
    else {
        m_entities.append(ent);
    }
    adjustBordersIfNeeded(entity);
}

/**
 * Insert a entity at the end of entities list and updates the
 * borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::appendEntity(RS_Entity* entity) {
    if (entity == nullptr) {
        return;
    }
    debugEntityAlreadyPresentExists(entity);
    m_entities.append(entity);
    adjustBordersIfNeeded(entity);
}

/**
 * Insert a entity at the start of entities list and updates the
 * borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::prependEntity(RS_Entity* entity) {
    if (entity == nullptr) {
        return;
    }
    debugEntityAlreadyPresentExists(entity);
    m_entities.prepend(entity);
    adjustBordersIfNeeded(entity);
}

/**
 * Move a entity list in this container at the given position,
 * the borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::moveEntity(const int index, QList<RS_Entity*>& entList) {
    if (entList.isEmpty()) {
        return;
    }
    int ci = 0; //current index for insert without invert order
    bool into = false;
    RS_Entity* mid = nullptr;
    if (index < 1) {
        ci = 0;
    }
    else if (index >= m_entities.size()) {
        ci = m_entities.size() - entList.size();
    }
    else {
        into = true;
        mid = m_entities.at(index);
    }

    for (int i = 0; i < entList.size(); ++i) {
        RS_Entity* e = entList.at(i);
        const bool ret = m_entities.removeOne(e);
        //if e not exist in entities list remove from entList
        if (!ret) {
            entList.removeAt(i);
        }
    }
    if (into) {
        ci = m_entities.indexOf(mid);
    }

    for (const auto e : entList) {
        m_entities.insert(ci++, e);
    }
}

void RS_EntityContainer::adjustBordersIfNeeded(const RS_Entity* entity) {
    if (m_autoUpdateBorders) {
        adjustBorders(entity);
    }
}

/**
 * Inserts a entity to this container at the given position and updates
 * the borders of this entity-container if autoUpdateBorders is true.
 */
void RS_EntityContainer::insertEntity(const int index, RS_Entity* entity) {
    if (entity == nullptr) {
        return;
    }
    debugEntityAlreadyPresentExists(entity);
    m_entities.insert(index, entity);
    adjustBordersIfNeeded(entity);
}

/**
 * Removes an entity from this container and updates the borders of
 * this entity-container if autoUpdateBorders is true.
 */
bool RS_EntityContainer::removeEntity(RS_Entity* entity) {
    if (entity != nullptr) {
        const bool ret = m_entities.removeOne(entity);
        if (ret) {
            // actually was contained in container
            const bool mayAffectBorders = entity->isVisible();
            if (m_autoDelete) {
                delete entity;
            }
            if (mayAffectBorders) {
                calculateBordersIfNeeded();
            }
        }
        return ret;
    }
    return false;
}

/**
 * Erases all entities in this container and resets the borders..
 */
void RS_EntityContainer::clear() {
    if (m_autoDelete) {
        while (!m_entities.isEmpty()) {
            const RS_Entity* en = m_entities.takeFirst();
            delete en;
        }
    }
    else {
        m_entities.clear();
    }
    resetBorders();
}

unsigned int RS_EntityContainer::count() const {
    return m_entities.size();
}

/**
 * Counts all entities (leaves of the tree).
 */
unsigned int RS_EntityContainer::countDeep() const {
    unsigned int c = 0;
    for (const auto t : *this) {
        c += t->countDeep();
    }
    return c;
}

/**
 * Adjusts the borders of this graphic (max/min values)
 */
void RS_EntityContainer::adjustBorders(const RS_Entity* entity) {
    //RS_DEBUG->print("RS_EntityContainer::adjustBorders");
    //resetBorders();

    if (entity != nullptr) {
        // make sure a container is not empty (otherwise the border
        //   would get extended to 0/0):
        if (!entity->isContainer() || entity->count() > 0) {
            m_minV = RS_Vector::minimum(entity->getMin(), m_minV);
            m_maxV = RS_Vector::maximum(entity->getMax(), m_maxV);
        }

        // Notify parents. The border for the parent might
        // also change TODO: Check for efficiency
        //if(parent) {
        //parent->adjustBorders(this);
        //}
    }
}

/**
 * Recalculates the borders of this entity container.
 */
void RS_EntityContainer::calculateBorders() {
    // fixme - sand verify that there are no not needed borders calculations!!!
    RS_DEBUG->print("RS_EntityContainer::calculateBorders");

    resetBorders();
    for (RS_Entity* e : *this) {
        if (e != nullptr && e->isVisible()) {
            e->calculateBorders();
            adjustBorders(e);
        }
    }

    RS_DEBUG->print("RS_EntityContainer::calculateBorders: size 1: %f,%f", getSize().x, getSize().y);

    // needed for correcting corrupt data (PLANS.dxf)
    if (m_minV.x > m_maxV.x || m_minV.x > RS_MAXDOUBLE || m_maxV.x > RS_MAXDOUBLE || m_minV.x < RS_MINDOUBLE || m_maxV.x < RS_MINDOUBLE) {
        m_minV.x = 0.0;
        m_maxV.x = 0.0;
    }
    if (m_minV.y > m_maxV.y || m_minV.y > RS_MAXDOUBLE || m_maxV.y > RS_MAXDOUBLE || m_minV.y < RS_MINDOUBLE || m_maxV.y < RS_MINDOUBLE) {
        m_minV.y = 0.0;
        m_maxV.y = 0.0;
    }

    RS_DEBUG->print("RS_EntityContainer::calculateBorders: size: %f,%f", getSize().x, getSize().y);
}

/**
 * Recalculates the borders of this entity container including
 * invisible entities.
 */
void RS_EntityContainer::forcedCalculateBorders() {
    resetBorders();
    for (RS_Entity* e : *this) {
        if (e->isContainer()) {
            const auto container = static_cast<RS_EntityContainer*>(e);
            container->forcedCalculateBorders();
        }
        else {
            e->calculateBorders();
        }
        adjustBorders(e);
    }

    // needed for correcting corrupt data (PLANS.dxf)
    if (m_minV.x > m_maxV.x || m_minV.x > RS_MAXDOUBLE || m_maxV.x > RS_MAXDOUBLE || m_minV.x < RS_MINDOUBLE || m_maxV.x < RS_MINDOUBLE) {
        m_minV.x = 0.0;
        m_maxV.x = 0.0;
    }
    if (m_minV.y > m_maxV.y || m_minV.y > RS_MAXDOUBLE || m_maxV.y > RS_MAXDOUBLE || m_minV.y < RS_MINDOUBLE || m_maxV.y < RS_MINDOUBLE) {
        m_minV.y = 0.0;
        m_maxV.y = 0.0;
    }
}

/**
 * Updates all Dimension entities in this container and / or
 * reposition their labels.
 *
 * @param autoText Automatically reposition the text label bool autoText=true
 */
int RS_EntityContainer::updateDimensions(const bool autoText) {
    RS_DEBUG->print("RS_EntityContainer::updateDimensions()");
    int updatedDimsCount = 0;

    for (RS_Entity* e : *this) {
        if (e->isDeleted()) {
            continue;
        }
        if (e->rtti() == RS2::EntityDimLeader) {
            updatedDimsCount++;
            e->update();
        }
        if (RS_Information::isDimension(e->rtti())) {
            const auto dimension = static_cast<RS_Dimension*>(e);
            // update and reposition label:
            dimension->update();
            updatedDimsCount++;
        }
        else if (e->isContainer()) {
            const auto container = static_cast<RS_EntityContainer*>(e);
            updatedDimsCount += container->updateDimensions(autoText);
        }
    }

    RS_DEBUG->print("RS_EntityContainer::updateDimensions() OK");
    return updatedDimsCount;
}

int RS_EntityContainer::updateVisibleDimensions(const bool autoText) {
    RS_DEBUG->print("RS_EntityContainer::updateVisibleDimensions()");
    int updatedDimsCount = 0;
    for (RS_Entity* e : *this) {
        if (e->isVisible()) {
            if (e->rtti() == RS2::EntityDimLeader) {
                e->update();
                updatedDimsCount++;
            }
            else if (RS_Information::isDimension(e->rtti())) {
                const auto dimension = static_cast<RS_Dimension*>(e);
                // update and reposition label:
                dimension->updateDim(autoText);
                updatedDimsCount++;
            }
            else if (e->isContainer()) {
                const auto container = static_cast<RS_EntityContainer*>(e);
                updatedDimsCount += container->updateVisibleDimensions(autoText);
            }
        }
    }

    RS_DEBUG->print("RS_EntityContainer::updateVisibleDimensions() OK");
    return updatedDimsCount;
}

/**
 * Updates all Insert entities in this container.
 */
void RS_EntityContainer::updateInserts() {
    const std::string idTypeId = std::to_string(getId()) + "/" + std::to_string(rtti());
    RS_DEBUG->print("RS_EntityContainer::updateInserts() ID/type: %s", idTypeId.c_str());

    for (RS_Entity* e : std::as_const(*this)) {
        //// Only update our own inserts and not inserts of inserts
        if (e != nullptr) {
            if (e->getId() != 0 && e->rtti() == RS2::EntityInsert /*&& e->getParent()==this*/) {
                static_cast<RS_Insert*>(e)->update();

                RS_DEBUG->print("RS_EntityContainer::updateInserts: updated ID/type: %s", idTypeId.c_str());
            }
            else if (e->isContainer()) {
                if (e->rtti() == RS2::EntityHatch) {
                    RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_EntityContainer::updateInserts: skip hatch ID/type: %s", idTypeId.c_str());
                }
                else {
                    RS_DEBUG->print("RS_EntityContainer::updateInserts: update container ID/type: %s", idTypeId.c_str());

                    static_cast<RS_EntityContainer*>(e)->updateInserts();
                }
            }
            else {
                RS_DEBUG->print(RS_Debug::D_DEBUGGING, "RS_EntityContainer::updateInserts: skip entity ID/type: %s", idTypeId.c_str());
            }
        }
        RS_DEBUG->print("RS_EntityContainer::updateInserts() ID/type: %s", idTypeId.c_str());
    }
}

/**
 * Renames all inserts with name 'oldName' to 'newName'. This is
 *   called after a block was rename to update the inserts.
 */
void RS_EntityContainer::renameInserts(const QString& oldName, const QString& newName) {
    RS_DEBUG->print("RS_EntityContainer::renameInserts()");
    for (RS_Entity* e : std::as_const(m_entities)) {
        if (e->rtti() == RS2::EntityInsert) {
            auto* i = static_cast<RS_Insert*>(e);
            if (i->getName() == oldName) {
                i->setName(newName);
            }
        }
        if (e->isContainer()) {
            const auto container = static_cast<RS_EntityContainer*>(e);
            container->renameInserts(oldName, newName);
        }
    }
    RS_DEBUG->print("RS_EntityContainer::renameInserts() OK");
}

/**
 * Updates all Spline entities in this container.
 */
void RS_EntityContainer::updateSplines() {
    RS_DEBUG->print("RS_EntityContainer::updateSplines()");
    for (RS_Entity* e : *this) {
        //// Only update our own inserts and not inserts of inserts
        if (e->rtti() == RS2::EntitySpline /*&& e->getParent()==this*/) {
            e->update();
        }
        else if (e->isContainer() && e->rtti() != RS2::EntityHatch) {
            static_cast<RS_EntityContainer*>(e)->updateSplines();
        }
    }
    RS_DEBUG->print("RS_EntityContainer::updateSplines() OK");
}

/**
 * Updates the sub entities of this container.
 */
void RS_EntityContainer::update() {
    for (RS_Entity* e : *this) {
        e->update();
    }
}

void RS_EntityContainer::addRectangle(const RS_Vector& v0, const RS_Vector& v1) {
    addEntity(new RS_Line{this, v0, {v1.x, v0.y}});
    addEntity(new RS_Line{this, {v1.x, v0.y}, v1});
    addEntity(new RS_Line{this, v1, {v0.x, v1.y}});
    addEntity(new RS_Line{this, {v0.x, v1.y}, v0});
}

void RS_EntityContainer::addRectangle(const RS_Vector& v0, const RS_Vector& v1, const RS_Vector& v2, const RS_Vector& v3) {
    addEntity(new RS_Line(this, v0, v1));
    addEntity(new RS_Line(this, v1, v2));
    addEntity(new RS_Line(this, v2, v3));
    addEntity(new RS_Line(this, v3, v0));
}

/**
 * Returns the first entity or nullptr if this graphic is empty.
 * @param level
 */
RS_Entity* RS_EntityContainer::firstEntity(const RS2::ResolveLevel level) const {
    RS_Entity* e = nullptr;
    m_entIdx = -1;
    switch (level) {
        case RS2::ResolveNone: {
            if (!m_entities.isEmpty()) {
                m_entIdx = 0;
                return m_entities.first();
            }
            break;
        }
        case RS2::ResolveAllButInserts: {
            m_subContainer = nullptr;
            if (!m_entities.isEmpty()) {
                m_entIdx = 0;
                e = m_entities.first();
            }
            if (e != nullptr && e->isContainer() && e->rtti() != RS2::EntityInsert) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->firstEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = nextEntity(level);
                }
            }
            return e;
        }
        case RS2::ResolveAllButTextImage:
        case RS2::ResolveAllButTexts: {
            m_subContainer = nullptr;
            if (!m_entities.isEmpty()) {
                m_entIdx = 0;
                e = m_entities.first();
            }
            if (e != nullptr && e->isContainer() && e->rtti() != RS2::EntityText && e->rtti() != RS2::EntityMText) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->firstEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = nextEntity(level);
                }
            }
            return e;
        }
        case RS2::ResolveAll: {
            m_subContainer = nullptr;
            if (!m_entities.isEmpty()) {
                m_entIdx = 0;
                e = m_entities.first();
            }
            if (e != nullptr && e->isContainer()) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->firstEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = nextEntity(level);
                }
            }
            return e;
        }
    }
    return nullptr;
}

/**
 * Returns the last entity or \p nullptr if this graphic is empty.
 *
 * @param level \li \p 0 Groups are not resolved
 *              \li \p 1 (default) only Groups are resolved
 *              \li \p 2 all Entity Containers are resolved
 */
RS_Entity* RS_EntityContainer::lastEntity(const RS2::ResolveLevel level) const {
    RS_Entity* e = nullptr;
    if (m_entities.empty()) {
        return nullptr;
    }
    m_entIdx = m_entities.size() - 1;
    switch (level) {
        case RS2::ResolveNone: {
            if (!m_entities.isEmpty()) {
                return m_entities.last();
            }
            break;
        }
        case RS2::ResolveAllButInserts: {
            if (!m_entities.isEmpty()) {
                e = m_entities.last();
            }
            m_subContainer = nullptr;
            if (e != nullptr && e->isContainer() && e->rtti() != RS2::EntityInsert) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->lastEntity(level);
            }
            return e;
        }
        case RS2::ResolveAllButTextImage:
        case RS2::ResolveAllButTexts: {
            if (!m_entities.isEmpty()) {
                e = m_entities.last();
            }
            m_subContainer = nullptr;
            if (e != nullptr && e->isContainer() && e->rtti() != RS2::EntityText && e->rtti() != RS2::EntityMText) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->lastEntity(level);
            }
            return e;
        }
        case RS2::ResolveAll: {
            if (!m_entities.isEmpty()) {
                e = m_entities.last();
            }
            m_subContainer = nullptr;
            if (e != nullptr && e->isContainer()) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->lastEntity(level);
            }
            return e;
        }
    }
    return nullptr;
}

/**
 * Returns the next entity or container or \p nullptr if the last entity
 * returned by \p next() was the last entity in the container.
 */
RS_Entity* RS_EntityContainer::nextEntity(const RS2::ResolveLevel level) const {
    //set entIdx pointing in next entity and check if is out of range
    ++m_entIdx;
    switch (level) {
        case RS2::ResolveNone: {
            if (m_entIdx < m_entities.size()) {
                return m_entities.at(m_entIdx);
            }
            break;
        }
        case RS2::ResolveAllButInserts: {
            RS_Entity* e = nullptr;
            if (m_subContainer != nullptr) {
                e = m_subContainer->nextEntity(level);
                if (e != nullptr) {
                    --m_entIdx; //return a sub-entity, index not advanced
                    return e;
                }
                if (m_entIdx < m_entities.size()) {
                    e = m_entities.at(m_entIdx);
                }
            }
            else {
                if (m_entIdx < m_entities.size()) {
                    e = m_entities.at(m_entIdx);
                }
            }
            if (e != nullptr && e->isContainer() && e->rtti() != RS2::EntityInsert) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->firstEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = nextEntity(level);
                }
            }
            return e;
        }
        case RS2::ResolveAllButTextImage:
        case RS2::ResolveAllButTexts: {
            RS_Entity* e = nullptr;
            if (m_subContainer != nullptr) {
                e = m_subContainer->nextEntity(level);
                if (e != nullptr) {
                    --m_entIdx; //return a sub-entity, index not advanced
                    return e;
                }
                if (m_entIdx < m_entities.size()) {
                    e = m_entities.at(m_entIdx);
                }
            }
            else {
                if (m_entIdx < m_entities.size()) {
                    e = m_entities.at(m_entIdx);
                }
            }
            if (e != nullptr && e->isContainer() && e->rtti() != RS2::EntityText && e->rtti() != RS2::EntityMText) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->firstEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = nextEntity(level);
                }
            }
            return e;
        }
        case RS2::ResolveAll: {
            RS_Entity* e = nullptr;
            if (m_subContainer != nullptr) {
                e = m_subContainer->nextEntity(level);
                if (e != nullptr) {
                    --m_entIdx; //return a sub-entity, index not advanced
                    return e;
                }
                if (m_entIdx < m_entities.size()) {
                    e = m_entities.at(m_entIdx);
                }
            }
            else {
                if (m_entIdx < m_entities.size()) {
                    e = m_entities.at(m_entIdx);
                }
            }
            if (e != nullptr && e->isContainer()) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->firstEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = nextEntity(level);
                }
            }
            return e;
        }
    }
    return nullptr;
}

/**
 * Returns the prev entity or container or \p nullptr if the last entity
 * returned by \p prev() was the first entity in the container.
 */
RS_Entity* RS_EntityContainer::prevEntity(const RS2::ResolveLevel level) const {
    //set entIdx pointing in prev entity and check if is out of range
    --m_entIdx;
    switch (level) {
        case RS2::ResolveNone: {
            if (m_entIdx >= 0) {
                return m_entities.at(m_entIdx);
            }
            break;
        }
        case RS2::ResolveAllButInserts: {
            RS_Entity* e = nullptr;
            if (m_subContainer != nullptr) {
                e = m_subContainer->prevEntity(level);
                if (e != nullptr) {
                    return e;
                }
                if (m_entIdx >= 0) {
                    e = m_entities.at(m_entIdx);
                }
            }
            else {
                if (m_entIdx >= 0) {
                    e = m_entities.at(m_entIdx);
                }
            }
            if (e != nullptr && e->isContainer() && e->rtti() != RS2::EntityInsert) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->lastEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = prevEntity(level);
                }
            }
            return e;
        }
        case RS2::ResolveAllButTextImage:
        case RS2::ResolveAllButTexts: {
            RS_Entity* e = nullptr;
            if (m_subContainer != nullptr) {
                e = m_subContainer->prevEntity(level);
                if (e != nullptr) {
                    return e;
                }
                if (m_entIdx >= 0) {
                    e = m_entities.at(m_entIdx);
                }
            }
            else {
                if (m_entIdx >= 0) {
                    e = m_entities.at(m_entIdx);
                }
            }
            if (e != nullptr && e->isContainer() && e->rtti() != RS2::EntityText && e->rtti() != RS2::EntityMText) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->lastEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = prevEntity(level);
                }
            }
            return e;
        }
        case RS2::ResolveAll: {
            RS_Entity* e = nullptr;
            if (m_subContainer != nullptr) {
                e = m_subContainer->prevEntity(level);
                if (e != nullptr) {
                    ++m_entIdx; //return a sub-entity, index not advanced
                    return e;
                }
                if (m_entIdx >= 0) {
                    e = m_entities.at(m_entIdx);
                }
            }
            else {
                if (m_entIdx >= 0) {
                    e = m_entities.at(m_entIdx);
                }
            }
            if (e != nullptr && e->isContainer()) {
                m_subContainer = static_cast<RS_EntityContainer*>(e);
                e = m_subContainer->lastEntity(level);
                // empty container:
                if (e == nullptr) {
                    m_subContainer = nullptr;
                    e = prevEntity(level);
                }
            }
            return e;
        }
    }
    return nullptr;
}

/**
 * @return Entity at the given index or nullptr if the index is out of range.
 */
RS_Entity* RS_EntityContainer::entityAt(const int index) const {
    if (m_entities.size() > index && index >= 0) {
        return m_entities.at(index);
    }
    return nullptr;
}

void RS_EntityContainer::setEntityAt(const int index, RS_Entity* en) {
    if (m_autoDelete && (m_entities.at(index) != nullptr)) {
        delete m_entities.at(index);
    }
    debugEntityAlreadyPresentExists(en);
    m_entities[index] = en;
}

/**
 * Finds the given entity and makes it the current entity if found.
 */
int RS_EntityContainer::findEntity(const RS_Entity* const entity) {
    m_entIdx = m_entities.indexOf(const_cast<RS_Entity*>(entity));
    return m_entIdx;
}

int RS_EntityContainer::findEntityIndex(const RS_Entity* const entity) const {
    return m_entities.indexOf(const_cast<RS_Entity*>(entity));
}

bool RS_EntityContainer::areNeighborsEntities(const RS_Entity* const e1, const RS_Entity* const e2) const {
    return abs(m_entities.indexOf(e1) - m_entities.indexOf(e2)) <= 1;
}

/**
 * @return The point which is closest to 'coord'
 * (one of the vertices)
 */
RS_Vector RS_EntityContainer::doGetNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** entity) const {
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    double curDist = 0.; // currently measured distance
    RS_Vector closestPoint(false); // closest found endpoint
    RS_Entity* closestEntity {nullptr};

    // fixme - actually, there could be improvement for snapping - by takind into consideration only entities within visual area
    for (const RS_Entity* en : *this) {
        if (en != nullptr && en->getId() != 0 && en->isVisible()) {
            const auto parent = en->getParent();
            bool checkForEndpoint = true;
            if (parent != nullptr) {
                checkForEndpoint = !parent->ignoredOnModification();
            }
            if (checkForEndpoint) {
                //no end point for Insert, text, Dim
                RS_Entity* closestCandidate = nullptr;
                const RS_Vector point = en->getNearestEndpoint(coord, &closestCandidate, &curDist);
                if (point.valid && curDist < minDist) {
                    closestPoint = point;
                    closestEntity = closestCandidate;
                    minDist = curDist;
                    if (dist != nullptr) {
                        *dist = minDist;
                    }
                }
            }
        }
    }
    if (entity != nullptr) {
        *entity = closestEntity;
    }
    return closestPoint;
}

/**
 * @return The point which is closest to 'coord'
 * (one of the vertices)
 */
RS_Vector RS_EntityContainer::obtainNearestEndpoint(const RS_Vector& coord, double* dist, RS_Entity** pEntity) const {
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    double curDist; // currently measured distance
    RS_Vector closestPoint(false); // closest found endpoint

    for (const auto en : m_entities) {
        if (en->getParent() == nullptr || !en->getParent()->ignoredOnModification()) {
            //no end point for Insert, text, Dim
            //            std::cout<<"find nearest for entity "<<i0<<std::endl;
            const RS_Vector point = en->getNearestEndpoint(coord, nullptr, &curDist);
            if (point.valid && curDist < minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist != nullptr) {
                    *dist = minDist;
                }
                if (pEntity != nullptr) {
                    *pEntity = en;
                }
            }
        }
    }
    return closestPoint;
}

RS_Vector RS_EntityContainer::doGetNearestPointOnEntity(const RS_Vector& coord, const bool onEntity, double* dist, RS_Entity** entity) const {
    RS_Vector point(false);
    const RS_Entity* en = getNearestEntity(coord, dist, RS2::ResolveNone);
    if (en != nullptr && en->isVisible() && !en->getParent()->ignoredSnap()) {
        point = en->getNearestPointOnEntity(coord, onEntity, dist, entity);
    }
    return point;
}

RS_Vector RS_EntityContainer::doGetNearestCenter(const RS_Vector& coord, double* dist, RS_Entity** centerEntity) const {
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    double curDist = RS_MAXDOUBLE; // currently measured distance
    RS_Vector closestPoint(false); // closest found endpoint
    RS_Entity* closestCenterEntity{nullptr};

    for (const auto en : m_entities) {
        if (en != nullptr && en->getId() != 0 && en->isVisible() && !en->getParent()->ignoredSnap()) {
            //no center point for spline, text, Dim
            RS_Entity* centerEnt;
            const RS_Vector point = en->getNearestCenter(coord, &curDist, &centerEnt);
            if (point.valid && curDist < minDist) {
                closestPoint = point;
                closestCenterEntity = centerEnt;
                minDist = curDist;
            }
        }
    }
    if (dist != nullptr) {
        *dist = minDist;
    }
    if (centerEntity != nullptr) {
        *centerEntity = closestCenterEntity;
    }
    return closestPoint;
}

/** @return the nearest of equidistant middle points of the line. */

RS_Vector RS_EntityContainer::doGetNearestMiddle(const RS_Vector& coord, double* dist, const int middlePoints) const {
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    double curDist = RS_MAXDOUBLE; // currently measured distance
    RS_Vector closestPoint(false); // closest found endpoint

    for (const auto en : m_entities) {
        if (en->isVisible() && !en->getParent()->ignoredSnap()) {
            //no midle point for spline, text, Dim
            const RS_Vector point = en->getNearestMiddle(coord, &curDist, middlePoints);
            if (point.valid && curDist < minDist) {
                closestPoint = point;
                minDist = curDist;
            }
        }
    }
    if (dist != nullptr) {
        *dist = minDist;
    }
    return closestPoint;
}

RS_Vector RS_EntityContainer::doGetNearestDist(const double distance, const RS_Vector& coord, double* dist) const {
    const RS_Entity* closestEntity = getNearestEntity(coord, nullptr, RS2::ResolveNone);
    return (closestEntity != nullptr) ? closestEntity->getNearestDist(distance, coord, dist) : RS_Vector{false};
}

/**
 * @return The intersection which is closest to 'coord'
 */
RS_Vector RS_EntityContainer::getNearestIntersection(const RS_Vector& coord, double* dist, RS_Entity** entity, RS_Entity** otherEntity) const {
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    RS_Vector closestPoint(false); // closest found endpoint
    const RS_Entity* closestEntity = getNearestEntity(coord, nullptr, RS2::ResolveAllButTextImage);

    if (closestEntity != nullptr) {
        // fixme - sand - why not via traverser?
        for (const RS_Entity* en = firstEntity(RS2::ResolveAllButTextImage); en != nullptr; en = nextEntity(RS2::ResolveAllButTextImage)) {
            const auto parent = en->getParent();
            bool ignoredSnap = false;
            if (parent != nullptr) {
                // may be null in block editing?
                ignoredSnap = parent->ignoredSnap();
            }
            if (!en->isVisible() || ignoredSnap) {
                continue;
            }

            RS_VectorSolutions sol = RS_Information::getIntersection(closestEntity, en, true);
            double curDist = RS_MAXDOUBLE; // currently measured distance
            const RS_Vector point = sol.getClosest(coord, &curDist, nullptr);
            if (sol.getNumber() > 0 && curDist < minDist) {
                closestPoint = point;
                minDist = curDist;
                if (entity != nullptr) {
                    *entity = const_cast<RS_Entity*>(closestEntity);
                }
                if (otherEntity != nullptr) {
                    *otherEntity = const_cast<RS_Entity*>(en);
                }
            }
        }
    }
    if ((dist != nullptr) && closestPoint.valid) {
        *dist = minDist;
    }
    return closestPoint;
}

RS_Vector RS_EntityContainer::getNearestVirtualIntersection(const RS_Vector& coord, const double angle, double* dist) const {
    const RS_Entity* closestEntity = getNearestEntity(coord, nullptr, RS2::ResolveAllButTextImage);
    if (closestEntity != nullptr) {
        const RS_Vector second_coord{angle};
        const RS_ConstructionLineData data(coord, coord + second_coord);
        const auto line = RS_ConstructionLine(nullptr, data);

        const RS_VectorSolutions sol = RS_Information::getIntersection(closestEntity, &line, true);
        if (sol.getVector().empty()) {
            return coord;
        }
        return sol.getClosest(coord, dist, nullptr);
    }
    return coord;
}

RS_Vector RS_EntityContainer::doGetNearestRef(const RS_Vector& coord, double* dist) const {
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    double curDist; // currently measured distance
    RS_Vector closestPoint(false); // closest found endpoint

    for (const RS_Entity* en : *this) {
        if (en->isVisible()) {
            const RS_Vector point = en->getNearestRef(coord, &curDist);
            if (point.valid && curDist < minDist) {
                closestPoint = point;
                minDist = curDist;
                if (dist != nullptr) {
                    *dist = minDist;
                }
            }
        }
    }
    return closestPoint;
}

RS_Vector RS_EntityContainer::doGetNearestSelectedRef(const RS_Vector& coord, double* dist) const {
    const RefInfo info = getNearestSelectedRefInfo(coord, dist);
    return info.ref;
}

RS_EntityContainer::RefInfo RS_EntityContainer::getNearestSelectedRefInfo(const RS_Vector& coord, double* dist) const {
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    RS_Vector closestPoint(false); // closest found endpoint
    RS_Entity* closestPointEntity = nullptr;

    for (RS_Entity* en : *this) {
        // fixme - sand - iteration of ver all entities
        if (en->isVisible() && en->isSelected() && !en->isParentSelected()) {
            // fixme - SELECTION - selection collection!
            double curDist = 0.; // currently measured distance
            const RS_Vector point = en->getNearestSelectedRef(coord, &curDist);
            if (point.valid && curDist < minDist) {
                closestPoint = point;
                closestPointEntity = en;
                minDist = curDist;
                if (dist != nullptr) {
                    *dist = minDist;
                }
            }
        }
    }
    const RefInfo result{closestPoint, closestPointEntity};
    return result;
}

double RS_EntityContainer::doGetDistanceToPoint(const RS_Vector& coord, RS_Entity** entity, const RS2::ResolveLevel level,
                                              const double solidDist) const {
    RS_DEBUG->print("RS_EntityContainer::getDistanceToPoint");
    double minDist = RS_MAXDOUBLE; // minimum measured distance
    double curDist = 0.; // currently measured distance
    RS_Entity* closestEntity = nullptr; // closest entity found
    RS_Entity* subEntity = nullptr;

    for (RS_Entity* e : *this) {
        const auto entityLayer = e->getLayer();
        if (e->isVisible() && (entityLayer == nullptr || !entityLayer->isLocked())) {
            RS_DEBUG->print("entity: getDistanceToPoint");
            RS_DEBUG->print("entity: %d", e->rtti());
            // bug#426, need to ignore Images to find nearest intersections
            if (level == RS2::ResolveAllButTextImage && e->rtti() == RS2::EntityImage) {
                continue;
            }
            curDist = e->getDistanceToPoint(coord, &subEntity, level, solidDist);

            RS_DEBUG->print("entity: getDistanceToPoint: OK");

            /*
             * By using '<=', we will prefer the *last* item in the container if there are multiple
             * entities that are *exactly* the same distance away, which should tend to be the one
             * drawn most recently, and the one most likely to be visible (as it is also the order
             * that the software draws the entities). This makes a difference when one entity is
             * drawn directly over top of another, and it's reasonable to assume that humans will
             * tend to want to reference entities that they see or have recently drawn as opposed
             * to deeper more forgotten and invisible ones...
             */
            if (curDist <= minDist) {
                switch (level) {
                    case RS2::ResolveAll:
                    case RS2::ResolveAllButTextImage:
                        closestEntity = subEntity;
                        break;
                    default:
                        closestEntity = e;
                        break;
                }
                minDist = curDist;
            }
        }
    }

    if (entity != nullptr) {
        *entity = closestEntity;
    }
    RS_DEBUG->print("RS_EntityContainer::getDistanceToPoint: OK");

    return minDist;
}

RS_Entity* RS_EntityContainer::getNearestEntity(const RS_Vector& coord, double* dist, const RS2::ResolveLevel level) const {
    RS_DEBUG->print("RS_EntityContainer::getNearestEntity");

    RS_Entity* e = nullptr;

    // distance for points inside solids:
    double solidDist = RS_MAXDOUBLE;
    if (dist != nullptr) {
        solidDist = *dist;
    }

    const double d = getDistanceToPoint(coord, &e, level, solidDist);
    if (e != nullptr && e->isVisible() == false) {
        e = nullptr;
    }

    // if d is negative, use the default distance (used for points inside solids)
    if (dist != nullptr) {
        *dist = d;
    }
    RS_DEBUG->print("RS_EntityContainer::getNearestEntity: OK");
    return e;
}

/**
 * Rearranges the atomic entities in this container in a way that connected
 * entities are stored in the right order and direction.
 * Non-recoursive. Only affects atomic entities in this container.
 *
 * @retval true all contours were closed
 * @retval false at least one contour is not closed

 * to do: find closed contour by flood-fill
 */
bool RS_EntityContainer::optimizeContours() {
    //    std::cout<<"RS_EntityContainer::optimizeContours: begin"<<std::endl;
    //    DEBUG_HEADER
    //    std::cout<<"loop with count()="<<count()<<std::endl;
    RS_DEBUG->print("RS_EntityContainer::optimizeContours");

    RS_EntityContainer tmp;
    tmp.setAutoUpdateBorders(false);
    bool closed = true;

    /** accept all full circles **/
    QList<RS_Entity*> enList;
    for (RS_Entity* e1 : *this) {
        if (!e1->isEdge() || e1->isContainer()) {
            enList << e1;
            continue;
        }

        //detect circles and whole ellipses
        switch (e1->rtti()) {
            case RS2::EntityEllipse: {
                if (static_cast<RS_Ellipse*>(e1)->isEllipticArc()) {
                    continue;
                }
                // fall-through
                [[fallthrough]];
            }
            case RS2::EntityCircle: {
                //directly detect circles, bug#3443277
                tmp.addEntity(e1->clone());
                enList << e1;
                // fall-through
                [[fallthrough]];
            }
            default:
                break;
        }
    }

    /** remove unsupported entities */
    for (RS_Entity* it : std::as_const(enList)) {
        removeEntity(it);
    }

    /** check and form a closed contour **/
    //    std::cout<<"RS_EntityContainer::optimizeContours: 2"<<std::endl;
    /** the first entity **/
    const RS_Entity* current(nullptr);
    if (count() > 0) {
        current = entityAt(0)->clone();
        tmp.addEntity(current);
        removeEntity(entityAt(0));
    }
    else {
        if (tmp.count() == 0) {
            return false;
        }
    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: 3"<<std::endl;
    RS_Vector vpStart;
    RS_Vector vpEnd;
    if (current != nullptr) {
        vpStart = current->getStartpoint();
        vpEnd = current->getEndpoint();
    }
    RS_Entity* next = nullptr;
    //    std::cout<<"RS_EntityContainer::optimizeContours: 4"<<std::endl;
    /** connect entities **/
    const auto errMsg = QObject::tr("Hatch failed due to a gap=%1 between (%2, %3) and (%4, %5)");

    while (count() > 0) {
        double dist = 0.;
        const RS_Vector vpTmp = obtainNearestEndpoint(vpEnd, &dist, &next);
        if (dist > CONTOUR_TOLERANCE) {
            if (vpEnd.squaredTo(vpStart) < CONTOUR_TOLERANCE) {
                RS_Entity* e2 = entityAt(0);
                tmp.addEntity(e2->clone());
                vpStart = e2->getStartpoint();
                vpEnd = e2->getEndpoint();
                removeEntity(e2);
                continue;
            }
            // fixme - sand - isn't it a really bad dependency? check later and remove
            QG_DIALOGFACTORY->commandMessage(errMsg.arg(dist).arg(vpTmp.x).arg(vpTmp.y).arg(vpEnd.x).arg(vpEnd.y));
            RS_DEBUG->print(RS_Debug::D_ERROR, "RS_EntityContainer::optimizeContours: hatch failed due to a gap");
            closed = false;
            break;
        }
        if (next == nullptr) {
            //workaround if next is nullptr
            //      	    std::cout<<"RS_EntityContainer::optimizeContours: next is nullptr" <<std::endl;
            RS_DEBUG->print("RS_EntityContainer::optimizeContours: next is nullptr");
            //			closed=false;	//workaround if next is nullptr
            break; //workaround if next is nullptr
        } // workaround if next is nullptr
        if (closed) {
            next->setProcessed(true);
            RS_Entity* eTmp = next->clone();
            if (vpEnd.squaredTo(eTmp->getStartpoint()) > vpEnd.squaredTo(eTmp->getEndpoint())) {
                eTmp->revertDirection();
            }
            vpEnd = eTmp->getEndpoint();
            tmp.addEntity(eTmp);
            removeEntity(next);
        }
    }
    //    DEBUG_HEADER
    //    if(vpEnd.valid && vpEnd.squaredTo(vpStart) > 1e-8) {
    //		QG_DIALOGFACTORY->commandMessage(errMsg.arg(vpEnd.distanceTo(vpStart))
    //											 .arg(vpStart.x).arg(vpStart.y).arg(vpEnd.x).arg(vpEnd.y));
    //        RS_DEBUG->print("RS_EntityContainer::optimizeContours: hatch failed due to a gap");
    //        closed=false;
    //    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: 5"<<std::endl;

    // add new sorted entities:
    for (RS_Entity* en : tmp) {
        en->setProcessed(false);
        addEntity(en->clone());
        en->reparent(this);
    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: 6"<<std::endl;

    if (closed) {
        RS_DEBUG->print("RS_EntityContainer::optimizeContours: OK");
    }
    else {
        RS_DEBUG->print("RS_EntityContainer::optimizeContours: bad");
    }
    //    std::cout<<"RS_EntityContainer::optimizeContours: end: count()="<<count()<<std::endl;
    //    std::cout<<"RS_EntityContainer::optimizeContours: closed="<<closed<<std::endl;
    return closed;
}

bool RS_EntityContainer::hasEndpointsWithinWindow(const RS_Vector& v1, const RS_Vector& v2) const {
    return std::any_of(cbegin(), cend(), [&v1, &v2](const RS_Entity* entity) {
        return entity->hasEndpointsWithinWindow(v1, v2);
    });
}

void RS_EntityContainer::move(const RS_Vector& offset) {
    moveBorders(offset);
    for (RS_Entity* e : *this) {
        e->move(offset);
        adjustBorders(e);
    }
    calculateBordersIfNeeded();
}

void RS_EntityContainer::rotate(const RS_Vector& center, const double angle) {
    RS_EntityContainer::rotate(center, RS_Vector{angle});
}

void RS_EntityContainer::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    resetBorders();
    for (RS_Entity* e : *this) {
        e->rotate(center, angleVector);
        adjustBorders(e);
    }
    calculateBordersIfNeeded();
}

void RS_EntityContainer::scale(const RS_Vector& center, const RS_Vector& factor) {
    if (std::abs(factor.x) > RS_TOLERANCE && std::abs(factor.y) > RS_TOLERANCE) {
        scaleBorders(center, factor);
        for (RS_Entity* e : *this) {
            e->scale(center, factor);
            adjustBorders(e);
        }
        calculateBordersIfNeeded();
    }
}

void RS_EntityContainer::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    if (axisPoint1.distanceTo(axisPoint2) > RS_TOLERANCE) {
        resetBorders();
        for (RS_Entity* e : *this) {
            e->mirror(axisPoint1, axisPoint2);
            adjustBorders(e);
        }
    }
}

RS_Entity& RS_EntityContainer::shear(const double k) {
    for (RS_Entity* e : *this) {
        e->shear(k);
    }
    calculateBorders();
    return *this;
}

void RS_EntityContainer::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset) {
    if (getMin().isInWindow(firstCorner, secondCorner) && getMax().isInWindow(firstCorner, secondCorner)) {
        move(offset);
    }
    else {
        for (RS_Entity* e : *this) {
            e->stretch(firstCorner, secondCorner, offset);
        }
    }
    // some entitiycontainers might need an update (e.g. RS_Leader):
    update();
}

void RS_EntityContainer::calculateBordersIfNeeded() {
    if (m_autoUpdateBorders) {
        calculateBorders();
    }
}

void RS_EntityContainer::moveRef(const RS_Vector& ref, const RS_Vector& offset) {
    resetBorders();
    for (RS_Entity* e : *this) {
        e->moveRef(ref, offset);
        adjustBorders(e);
    }
    calculateBordersIfNeeded();
}

void RS_EntityContainer::moveSelectedRef(const RS_Vector& ref, const RS_Vector& offset) {
    resetBorders();
    for (RS_Entity* e : *this) {
        e->moveSelectedRef(ref, offset);
        adjustBorders(e);
    }
    calculateBordersIfNeeded();
}

void RS_EntityContainer::revertDirection() {
    // revert entity order in the container
    for (int k = 0; k < m_entities.size() / 2; ++k) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
        m_entities.swapItemsAt(k, m_entities.size() - 1 - k);
#else
        entities.swap(k, entities.size() - 1 - k);
#endif
    }

    // revert each entity itself
    for (RS_Entity* entity : std::as_const(m_entities)) {
        entity->revertDirection();
    }
}

/**
 * @brief draw m_entities in order
 * @param painter
 */
void RS_EntityContainer::draw(RS_Painter* painter) {
    for (RS_Entity* e : *this) {
        if (e != nullptr && e->getId() != 0) {
            painter->drawEntity(e);
        }
    }
}

void RS_EntityContainer::drawAsChild(RS_Painter* painter) {
    for (RS_Entity* e : *this) {
        if (e != nullptr && e->getId() != 0) {
            painter->drawAsChild(e);
        }
    }
}

/**
 * @brief areaLineIntegral, line integral for contour area calculation by Green's Theorem
 * Contour Area =\oint x dy
 * @return line integral \oint x dy along the entity
 */
double RS_EntityContainer::areaLineIntegral() const {
    //TODO make sure all contour integral is by counter-clockwise
    double contourArea = 0.;
    //closed area is always positive
    double closedArea = 0.;
    double subArea = 0.;

    // edges:
    RS_Vector previousPoint(false);
    for (unsigned i = 0; i < count(); ++i) {
        RS_Entity* e = m_entities.at(i);
        if (isClosedLoop(*e)) {
            if (e->isContainer()) {
                subArea += e->areaLineIntegral();
            }
            else {
                closedArea += e->areaLineIntegral();
            }
            continue;
        }
        e->setLayer(getLayer());
        const double lineIntegral = e->areaLineIntegral();
        RS_Vector startPoint = e->getStartpoint();
        RS_Vector endPoint = e->getEndpoint();
        //        LC_ERR << e->getId() << ": int = " << lineIntegral << ": " << startPoint.x << " - " << endPoint.x;

        // the line integral is always by the direction: from the start point to the end point
        if (previousPoint.valid) {
            const double distance = endPointDistance(previousPoint, *e);
            if (distance > CONTOUR_TOLERANCE) {
                RS_DEBUG->print(RS_Debug::D_ERROR, "%s(): contour area calculation maybe incorrect: gap of %lg found at (%lg, %lg)",
                                __func__, distance, previousPoint.x, previousPoint.y);
            }
        }
        // assume the contour is a simple connected loop
        if (previousPoint.valid && endPoint.squaredTo(previousPoint) <= RS_TOLERANCE15) {
            contourArea -= lineIntegral;
            previousPoint = startPoint;
        }
        else {
            bool useEndPoint = true;
            if (!previousPoint.valid && i + 1 < count()) {
                const auto currEntity = m_entities.at(i + 1);
                useEndPoint = endPointDistance(endPoint, *currEntity) < endPointDistance(startPoint, *currEntity);
            }
            contourArea += useEndPoint ? lineIntegral : -lineIntegral;
            previousPoint = useEndPoint ? endPoint : startPoint;
        }
    }
    return std::abs(contourArea) + closedArea - subArea;
}

bool RS_EntityContainer::ignoredOnModification() const {
    const RS2::EntityType ownType = rtti();
    if (RS2::isDimensionalEntity(ownType) || RS2::isTextEntity(ownType) || ownType == RS2::EntityHatch) {
        return true;
    }
    return isParentIgnoredOnModifications();
}

bool RS_EntityContainer::ignoredSnap() const {
    // issue #652 , disable snap for hatch
    // TODO, should snapping on hatch be a feature enabled by settings?
    if ((getParent() != nullptr) && getParent()->rtti() == RS2::EntityHatch) {
        return true;
    }
    return ignoredOnModification();
}

#define DEBUG_CONTAINER_DUPLICATE  // fixme - sand - disable before push!

void RS_EntityContainer::debugEntityAlreadyPresentExists(const RS_Entity* entity) const {
#ifdef DEBUG_CONTAINER_DUPLICATE
    const qsizetype countOfEntities = m_entities.count(entity);
    Q_ASSERT(countOfEntities == 0);
#endif
}

QList<RS_Entity*>::const_iterator RS_EntityContainer::begin() const {
    return m_entities.begin();
}

QList<RS_Entity*>::const_iterator RS_EntityContainer::end() const {
    return m_entities.end();
}

QList<RS_Entity*>::const_iterator RS_EntityContainer::cbegin() const {
    return m_entities.cbegin();
}

QList<RS_Entity*>::const_iterator RS_EntityContainer::cend() const {
    return m_entities.cend();
}

QList<RS_Entity*>::iterator RS_EntityContainer::begin() {
    return m_entities.begin();
}

QList<RS_Entity*>::iterator RS_EntityContainer::end() {
    return m_entities.end();
}

/**
 * Dumps the entities to stdout.
 */
std::ostream& operator<<(std::ostream& os, RS_EntityContainer& ec) {
    static int indent = 0;
    const std::string tab(indent * 2, ' ');
    ++indent;
    const unsigned long int id = ec.getId();
    os << tab << "EntityContainer[" << id << "]: \n";
    os << tab << "Borders[" << id << "]: " << ec.m_minV << " - " << ec.m_maxV << "\n";
    if (ec.getLayer() != nullptr) {
        os << tab << "Layer[" << id << "]: " << ec.getLayer()->getName().toLatin1().data() << "\n";
    }
    else {
        os << tab << "Layer[" << id << "]: <nullptr>\n";
    }
    //os << ec.layerList << "\n";

    os << tab << " Flags[" << id << "]: " << (ec.getFlag(RS2::FlagVisible) ? "RS2::FlagVisible" : "");
    os << (ec.getFlag(RS2::FlagDeleted) ? " RS2::FlagUndone" : "");
    os << (ec.getFlag(RS2::FlagSelected) ? " RS2::FlagSelected" : "");
    os << "\n";

    os << tab << "Entities[" << id << "]: \n";
    for (const auto t : ec) {
        switch (t->rtti()) {
            case RS2::EntityInsert:
                os << tab << *static_cast<RS_Insert*>(t);
                os << tab << *t;
                os << tab << *static_cast<RS_EntityContainer*>(t);
                break;
            default:
                if (t->isContainer()) {
                    os << tab << *static_cast<RS_EntityContainer*>(t);
                }
                else {
                    os << tab << *t;
                }
                break;
        }
    }
    os << tab << "\n\n";
    --indent;
    return os;
}

RS_Entity* RS_EntityContainer::first() const {
    return m_entities.first();
}

RS_Entity* RS_EntityContainer::last() const {
    return m_entities.last();
}

const QList<RS_Entity*>& RS_EntityContainer::getEntityList() const {
    return m_entities;
}

std::vector<std::unique_ptr<RS_EntityContainer>> RS_EntityContainer::getLoops() const {
    if (m_entities.empty()) {
        return {};
    }
    std::vector<std::unique_ptr<RS_EntityContainer>> loops;
    RS_EntityContainer edges(nullptr, false);
    for(RS_Entity* en: *this){
        if (en != nullptr && en->isContainer()){
            if (en->isContainer()){
                auto subLoops = static_cast<RS_EntityContainer*>(en)->getLoops();
                for (auto& subLoop: subLoops) {
                    loops.push_back(std::move(subLoop));
                }
            }
            continue;
        }

        if (en == nullptr || !en->isEdge()) {
            continue;
        }

        //detect circles and whole ellipses
        switch (en->rtti()) {
            case RS2::EntityEllipse:
                if (static_cast<RS_Ellipse*>(en)->isEllipticArc()) {
                    edges.addEntity(en);
                    break;
                }
                [[fallthrough]];
            case RS2::EntityCircle: {
                auto ec = std::make_unique<RS_EntityContainer>(nullptr, false);
                ec->addEntity(en);
                loops.push_back(std::move(ec));
                break;
            }
            default:
                edges.addEntity(en);
        }
    }
    //find loops
    while (!edges.isEmpty()) {
        LC_LoopUtils::LoopExtractor extractor{edges};
        auto subLoops = extractor.extract();
        for (auto& loop : subLoops) {
            loops.push_back(std::move(loop));
        }
    }
    return loops;
}
