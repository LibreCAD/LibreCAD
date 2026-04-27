/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright LibreCAD librecad.org
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
#include "rs_modification.h"

#include "lc_containertraverser.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_splinepoints.h"
#include "lc_undosection.h"
#include "rs_arc.h"
#include "rs_atomicentity.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_clipboard.h"
#include "rs_creation.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_insert.h"
#include "rs_layer.h"
#include "rs_line.h"
#include "rs_math.h"
#include "rs_mtext.h"
#include "rs_polyline.h"
#include "rs_selection.h"
#include "rs_settings.h"
#include "rs_text.h"

#ifdef EMU_C99
#include "emu_c99.h"
#endif

class LC_SplinePoints;

namespace {
      // fixme - sand - add option for intersection tolerance????

    RS_VectorSolutions findIntersection(const RS_Entity& trimEntity, const RS_Entity& limitEntity, double tolerance = 1e-4) {
        RS_VectorSolutions sol;
        if (limitEntity.isAtomic()) {
            // intersection(s) of the two entities:
            return RS_Information::getIntersection(&trimEntity, &limitEntity, false);
        }
        if (limitEntity.isContainer()) {
            const auto ec = static_cast<const RS_EntityContainer*>(&limitEntity);

            for (RS_Entity* e : lc::LC_ContainerTraverser{*ec, RS2::ResolveAll}.entities()) {
                RS_VectorSolutions s2 = RS_Information::getIntersection(&trimEntity, e, false);

                std::copy_if(s2.begin(), s2.end(), std::back_inserter(sol), [e, tolerance](const RS_Vector& vp) {
                    return vp.valid && e->isPointOnEntity(vp, tolerance);
                });
            }
        }
        return sol;
    }

    RS_Arc* trimCircle(const RS_Circle* circle, const RS_Vector& trimCoord, const RS_VectorSolutions& sol) {
        double aStart = 0.;
        double aEnd = 2. * M_PI;
        switch (sol.size()) {
            case 0:
                break;
            case 1:
                aStart = circle->getCenter().angleTo(sol.at(0));
                aEnd = aStart + 2. * M_PI;
                break;
            default: case 2:
                //trim according to intersections
                const RS_Vector& center0 = circle->getCenter();
                const std::vector<double> angles{{center0.angleTo(sol[0]), center0.angleTo(sol[1])}};
                const double a0 = center0.angleTo(trimCoord);
                aStart = angles.front();
                aEnd = angles.back();
                if (!RS_Math::isAngleBetween(a0, aStart, aEnd, false)) {
                    std::swap(aStart, aEnd);
                }
                break;
        }
        const RS_ArcData arcData(circle->getCenter(), circle->getRadius(), aStart, aEnd, false);
        return new RS_Arc(circle->getParent(), arcData);
    }

    /**
     * @brief getIdFlagString create a string by the entity and ID and type ID.
     * @param entity - entity, could be nullptr
     * @return std::string - "ID/typeID", or an empty string, if the input entity is nullptr
     */
    // fixme - sand - function is not used!!
    std::string getIdFlagString(const RS_Entity* entity) {
        if (entity == nullptr) {
            return {};
        }
        return std::to_string(entity->getId()) + "/" + std::to_string(entity->rtti());
    }

    // Support fillet trimming for whole ellipses
    RS_AtomicEntity* trimEllipseForRound(RS_AtomicEntity* entity, const RS_Arc& arcFillet) {
        if (entity == nullptr) {
            return entity;
        }
        if (entity->rtti() != RS2::EntityEllipse) {
            return entity;
        }
        auto* ellipse = static_cast<RS_Ellipse*>(entity);
        if (ellipse->isEllipticArc()) {
            return entity;
        }
        RS_Vector tangent = entity->getNearestPointOnEntity(arcFillet.getCenter(), false);
        const RS_Line line{nullptr, {arcFillet.getCenter(), tangent}};
        const RS_Vector middle = arcFillet.getMiddlePoint();
        const RS_Vector opposite = arcFillet.getCenter() + (arcFillet.getCenter() - middle).normalized() * ellipse->getMinorRadius() * 0.01;
        const RS_Vector trimCoord = ellipse->getNearestPointOnEntity(opposite, false);
        const RS_VectorSolutions sol = RS_Information::getIntersection(entity, &line, false);
        ellipse->prepareTrim(trimCoord, sol);
        return entity;
    }

    // A quick fix for rounding on circles
    RS_AtomicEntity* trimCircleForRound(RS_AtomicEntity* entity, const RS_Arc& arcFillet) {
        if (entity == nullptr) {
            return entity;
        }
        if (entity->rtti() == RS2::EntityEllipse) {
            return trimEllipseForRound(entity, arcFillet);
        }
        if (entity->rtti() != RS2::EntityCircle) {
            return entity;
        }
        const RS_Line line{nullptr, {arcFillet.getCenter(), entity->getCenter()}};
        const RS_Vector middle = arcFillet.getMiddlePoint();
        // prefer acute angle for fillet
        // Use a trimCoord at the opposite side of the arc wrt to the
        const RS_Vector opposite = arcFillet.getCenter() + (arcFillet.getCenter() - middle).normalized() * entity->getRadius() * 0.01;
        const RS_Vector trimCoord = entity->getNearestPointOnEntity(opposite, true);
        const RS_VectorSolutions sol = RS_Information::getIntersection(entity, &line, false);
        RS_Arc* arc = trimCircle(static_cast<RS_Circle*>(entity), trimCoord, sol);
        delete entity;
        return arc;
    }

    bool isOneOfPoints(const RS_Vector& candidate, const RS_Vector& point1, const RS_Vector& point2) {
        const bool result = point1.distanceTo(candidate) < RS_TOLERANCE || point2.distanceTo(candidate) < RS_TOLERANCE;
        return result;
    }

    /**
 * @brief getUniqueBlockName - Generates unique block name like "PASTE_0"
 * @param graphic - Target graphic
 * @param baseName - Base ("PASTE" default)
 * @return Unique name
 */
    QString getUniqueBlockName(RS_Graphic* graphic, const QString& baseName = QStringLiteral("PASTE")) {
        if (graphic == nullptr) {
            return baseName;
        }
        RS_BlockList* bl = graphic->getBlockList();
        if (bl == nullptr) {
            return baseName;
        }
        int i = 0;
        QString candidate;
        do {
            candidate = QString("%1_%2").arg(baseName).arg(i++);
        }
        while (bl->find(candidate) != nullptr);
        return candidate;
    }
} // namespace

LC_LibraryInsertData::LC_LibraryInsertData(const RS_Vector& insertionPoint, const double factor, const double angle, const QString& blockName,
                                           RS_Graphic* source)
    : RS_PasteData(insertionPoint, factor, angle), blockName{blockName}, source{source} {
}

void RS_Modification::revertDirection(QList<RS_Entity*>& originalEntities, LC_DocumentModificationBatch& ctx) {
    for (const auto e : originalEntities) {
        RS_Entity* clone = e->clone();
        clone->revertDirection();
        ctx += clone;
    }
    ctx -= originalEntities;
    ctx.success = true;
}

/**
 * Changes the attributes of all selected
 */
bool RS_Modification::changeAttributes(const QList<RS_Entity*>& originalEntities, RS_AttributesData& data,
                                       LC_DocumentModificationBatch& ctx) {
    QSet<RS_Block*> blocks;

    for (const auto en : originalEntities) {
        if (en == nullptr) {
            continue;
        }
        RS_Entity* clone = nullptr;
        doChangeEntityAttributes(en, clone, data, blocks);
        ctx += clone;
        ctx -= en;
    }

    const bool hasInserts = !blocks.empty();

    if (hasInserts && data.applyBlockDeep) {
        QSet<QString> processedBlockName;
        for (const auto block : std::as_const(blocks)) {
            doChangeBlockAttributes(block, data, processedBlockName);
        }
        return hasInserts;
    }
    return hasInserts;
}

void RS_Modification::doChangeEntityAttributes(RS_Entity* const en, RS_Entity*& clone, const RS_AttributesData& data,
                                               QSet<RS_Block*>& blocks) {
    const auto rtti = en->rtti();
    if (rtti == RS2::EntityInsert) {
        RS_Block* bl = static_cast<RS_Insert*>(en)->getBlockForInsert();
        if (bl != nullptr) {
            blocks << bl;
        }
    }
    clone = en->clone();

    if (data.changeLayer) {
        clone->setLayer(data.layer);
    }
    RS_Pen pen = clone->getPen(false);
    if (data.changeColor) {
        pen.setColor(data.pen.getColor());
    }
    if (data.changeLineType) {
        pen.setLineType(data.pen.getLineType());
    }
    if (data.changeWidth) {
        pen.setWidth(data.pen.getWidth());
    }
    clone->setPen(pen);

    if (RS2::isDimensionalEntity(rtti)) {
        clone->update();
    }
}

void RS_Modification::doChangeBlockAttributes(const RS_Block* block, RS_AttributesData& data, QSet<QString>& processedBlockNames) {
    const QString name = block->getName();
    if (processedBlockNames.contains(name)) {
        return;
    }
    processedBlockNames.insert(name);

    QSet<RS_Block*> blocks;
    for (const auto en : *block) {
        // fixme - sand - NOTE - EDITING OF BLOK IS NOT UNDOABLE SO FAR!!
        if (en != nullptr) {
            const auto rtti = en->rtti();
            if (rtti == RS2::EntityInsert) {
                auto bl = static_cast<RS_Insert*>(en)->getBlockForInsert();
                if (bl != nullptr) {
                    blocks << bl;
                }
            }
            if (data.changeLayer) {
                en->setLayer(data.layer);
            }
            RS_Pen pen = en->getPen(false);
            if (data.changeColor) {
                pen.setColor(data.pen.getColor());
            }
            if (data.changeLineType) {
                pen.setLineType(data.pen.getLineType());
            }
            if (data.changeWidth) {
                pen.setWidth(data.pen.getWidth());
            }
            en->setPen(pen);

            if (RS2::isDimensionalEntity(rtti)) {
                en->update();
            }
        }
    }

    if (!blocks.empty()) {
        for (const auto b : std::as_const(blocks)) {
            doChangeBlockAttributes(b, data, processedBlockNames);
        }
    }
}

RS_BoundData RS_Modification::getBoundingRect(QList<RS_Entity*>& selected) {
    auto min = RS_Vector(10e10, 10e10, 0);
    auto max = RS_Vector(-10e10, -10e10, 0);
    for (const auto e : selected) {
        const RS_Vector& entityMin = e->getMin();
        const RS_Vector& entityMax = e->getMax();

        min.x = std::min(min.x, entityMin.x);
        min.y = std::min(min.y, entityMin.y);
        max.x = std::max(max.x, entityMax.x);
        max.y = std::max(max.y, entityMax.y);
    }

    const RS_BoundData result(min, max);
    return result;
}

void RS_Modification::libraryInsert(const LC_LibraryInsertData& data, RS_Graphic* destination, LC_DocumentModificationBatch& ctx) {
    RS_Graphic* src = data.source;
    Q_ASSERT(src != nullptr);

    // Scale (units)
    const RS_Vector scaleV = LC_CopyUtils::getInterGraphicsScaleFactor(data.factor, src, destination);

    src->calculateBorders();
    const RS_Vector center = (src->getMin() + src->getMax()) * 0.5; // fixme - sand - no insertion point for such block???? Why center?

    // === BLOCK: Bake → angle=0 ===
    const QString bname = data.blockName.isEmpty() ? getUniqueBlockName(destination) : data.blockName;
    // fixme - what if the block with such name already exists???

    const auto blockData = RS_BlockData(bname, {0.0, 0.0}, false);
    auto* block = new RS_Block(destination, blockData);

    for (const RS_Entity* e : *src) {
        if (e == nullptr || e->isDeleted()) {
            continue;
        }
        RS_Entity* clone = e->clone();
        // Bake: center→0 → scale/rot → block@0
        clone->move(-center); // fixme - sand should the offset be used there???
        clone->scale(RS_Vector{}, scaleV);
        clone->rotate(RS_Vector{}, data.angle);
        block->addByBlockEntity(clone); // **ByBlock** 👌
    }

    destination->addBlock(block);

    // Insert (baked, **angle=0**)
    const RS_InsertData idata(bname, data.insertionPoint, {1., 1.}, 0.0, 1, 1, {});
    auto* insert = new RS_Insert(nullptr, idata);

    ctx += insert;

    // Props (inherit)
    const RS_Entity* first = src->firstEntity(RS2::ResolveNone); // fixme - why properties for insert are from first entity?
    if (first != nullptr) {
        insert->setLayer(first->getLayer());
        insert->setPen(first->getPen(true));
    }

    insert->update();

    destination->updateInserts(); // fixme - why all inserts are updated there?
}

/**
 * Splits a polyline into two leaving out a gap.
 *
 * @param polyline The original polyline
 * @param e1 1st entity on which the first cutting point is.
 * @param v1 1st cutting point.
 * @param e2 2nd entity on which the first cutting point is.
 * @param v2 2nd cutting point.
 * @param polyline1 Pointer to a polyline pointer which will hold the
 *        1st resulting new polyline. Pass nullptr if you don't
 *        need those pointers.
 * @param polyline2 Pointer to a polyline pointer which will hold the
 *        2nd resulting new polyline. Pass nullptr if you don't
 *        need those pointers.
 * @param ctx
 *
 * @todo Support arcs in polylines, check for wrong parameters
 *
 * @return true
 */
bool RS_Modification::splitPolyline(RS_Polyline* polyline, const RS_Entity& e1, const RS_Vector& v1, const RS_Entity& e2,
                                    const RS_Vector& v2, RS_Polyline** polyline1, RS_Polyline** polyline2,
                                    LC_DocumentModificationBatch& ctx) {
    RS_Entity* firstEntity = polyline->firstEntity();
    RS_Vector firstPoint(false);
    if (firstEntity->rtti() == RS2::EntityLine) {
        firstPoint = static_cast<RS_Line*>(firstEntity)->getStartpoint();
    }
    auto* pl1 = new RS_Polyline(nullptr, RS_PolylineData(firstPoint, RS_Vector(0.0, 0.0), false));
    auto* pl2 = new RS_Polyline(nullptr);
    RS_Polyline* pl = pl1; // Current polyline
    const RS_Line* line = nullptr;

    if (polyline1 != nullptr) {
        *polyline1 = pl1;
    }
    if (polyline2 != nullptr) {
        *polyline2 = pl2;
    }

    for (const auto e : *polyline) {
        [[maybe_unused]] const RS_Arc* arc = nullptr; // for the future support
        if (e->rtti() == RS2::EntityLine) {
            line = static_cast<RS_Line*>(e);
            arc = nullptr;
        }
        else if (e->rtti() == RS2::EntityArc) {
            arc = static_cast<RS_Arc*>(e);
            line = nullptr;
        }
        else {
            line = nullptr;
            arc = nullptr;
        }

        // todo - split in line is supported so far!

        if (line != nullptr /*|| arc*/) {
            if (e == &e1 && e == &e2) {
                // Trim within a single entity:
                RS_Vector sp = line->getStartpoint();
                const double dist1 = (v1 - sp).magnitude();
                const double dist2 = (v2 - sp).magnitude();
                pl->addVertex(dist1 < dist2 ? v1 : v2, 0.0);
                pl = pl2;
                pl->setStartpoint(dist1 < dist2 ? v2 : v1);
                pl->addVertex(line->getEndpoint(), 0.0);
            }
            else if (e == &e1 || e == &e2) {
                // Trim entities:
                RS_Vector v = e == &e1 ? v1 : v2;
                if (pl == pl1) {
                    // Trim endpoint of entity to first vector
                    pl->addVertex(v, 0.0);
                }
                else {
                    // Trim startpoint of entity to second vector
                    pl = pl2;
                    pl->setStartpoint(v);
                    pl->addVertex(line->getEndpoint(), 0.0);
                }
            }
            else {
                // Add entities to polylines
                if (pl != nullptr) {
                    pl->addVertex(line->getEndpoint(), 0.0);
                }
            }
        }
    }

    ctx += pl1;
    ctx += pl2;
    ctx -= polyline;

    return true;
}

void RS_Modification::selectClone(const RS_Entity* original, RS_Entity* clone) {
    const bool select = original->isSelected();
    clone->setSelectionFlag(select);
}

/**
 * Adds a node to the given polyline. The new node is placed between
 * the start and end point of the given segment.
 * This implementation properly handles not only line segments, but arcs too!
 *
 * @param polyline
 * @param segment
 * @param node The position of the new node.
 * @param ctx
 *
 * @return Pointer to the new polyline or nullptr.
 */
RS_Polyline* RS_Modification::addPolylineNode(RS_Polyline* polyline, const RS_AtomicEntity& segment, const RS_Vector& node,
                                              LC_DocumentModificationBatch& ctx) {
    Q_ASSERT(segment.getParent()==polyline);

    auto* clone = static_cast<RS_Polyline*>(polyline->clone());

    const unsigned int segmentIndex = polyline->findEntity(&segment);

    const RS2::EntityType originalSegmentType = segment.rtti();
    switch (originalSegmentType) {
        case RS2::EntityArc:
        case RS2::EntityLine: default: {
            const auto cloneSegmentToSplit = static_cast<RS_AtomicEntity*>(clone->entityAt(segmentIndex));
            const auto segmentToAdd = static_cast<RS_AtomicEntity*>(cloneSegmentToSplit->clone());

            cloneSegmentToSplit->trimEndpoint(node);
            segmentToAdd->trimStartpoint(node);

            clone->insertEntity(segmentIndex + 1, segmentToAdd);
            break;
        }
    }

    ctx.replace(polyline, clone);
    return clone;
}

/**
 * Deletes a node from a polyline.
 *
 * @param polyline
 * @param node The node to delete.
 * @param ctx
 *
 * @return Pointer to the new polyline or nullptr.
 */
RS_Polyline* RS_Modification::deletePolylineNode(RS_Polyline* polyline, const RS_Vector& node, LC_DocumentModificationBatch& ctx) {
    Q_ASSERT(node.valid);

    // check if the polyline is no longer there after deleting the node:
    if (polyline->count() == 1) {
        const RS_Entity* e = polyline->firstEntity();
        if (e != nullptr && e->isAtomic()) {
            if (isOneOfPoints(node, e->getStartpoint(), e->getEndpoint())) {
                ctx -= polyline;
            }
        }
        return nullptr;
    }

    auto* newPolyline = new RS_Polyline(nullptr);
    newPolyline->setClosed(polyline->isClosed());

    // copy polyline and drop deleted node:
    bool first = true;
    bool lastDropped = false;
    const RS_Entity* lastEntity = polyline->lastEntity();
    for (const auto e : *polyline) {
        if (e->isAtomic()) {
            const auto ae = static_cast<RS_AtomicEntity*>(e);
            double bulge = 0.0;
            if (ae->rtti() == RS2::EntityArc) {
                bulge = static_cast<RS_Arc*>(ae)->getBulge();
            }
            else {
                bulge = 0.0;
            }

            // last entity is closing entity and will be added below with endPolyline()
            if (e == lastEntity && polyline->isClosed()) {
                continue;
            }

            auto startpoint = ae->getStartpoint();
            // first vertex (startpoint)
            if (first && LC_LineMath::isMeaningfulDistance(node, startpoint)) {
                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(startpoint);
                first = false;
            }

            auto endpoint = ae->getEndpoint();
            // normal node (not deleted):
            if (first == false && LC_LineMath::isMeaningfulDistance(node, endpoint)) {
                if (lastDropped) {
                    //bulge = 0.0;
                }
                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(endpoint);
                lastDropped = false;
            }
            // drop deleted node:
            else {
                lastDropped = true;
            }
        }
        else {
            // fixme - how it may be in general?
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::deletePolylineNode: Polyline contains non-atomic entities");
        }
    }

    newPolyline->setNextBulge(polyline->getClosingBulge());
    newPolyline->endPolyline();
    newPolyline->updateEndpoints();

    ctx.replace(polyline, newPolyline);

    return newPolyline;
}

/**
 * Deletes all nodes between the two given nodes (exclusive).
 *
 * @param polyline
 * @param node1 First limiting node.
 * @param node2 Second limiting node.
 * @param ctx
 *
 * @return Pointer to the new polyline or nullptr.
 */
RS_Polyline* RS_Modification::deletePolylineNodesBetween(RS_Polyline* polyline, const RS_Vector& node1, const RS_Vector& node2,
                                                         LC_DocumentModificationBatch& ctx) {
    Q_ASSERT(node1.valid && node2.valid);

    if (LC_LineMath::isNotMeaningfulDistance(node1, node2)) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::deletePolylineNodesBetween: nodes are identical");
        return nullptr;
    }

    // check if there's nothing to delete:
    for (const auto e : *polyline) {
        if (e->isAtomic()) {
            const auto* atomic = static_cast<RS_AtomicEntity*>(e);
            /// FIXME- RS_TOLERANCE?
            if ((node1.distanceTo(atomic->getStartpoint()) < RS_TOLERANCE && node2.distanceTo(atomic->getEndpoint()) < 1.0e-6) || (node2.
                distanceTo(atomic->getStartpoint()) < 1.0e-6 && node1.distanceTo(atomic->getEndpoint()) < 1.0e-6)) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::deletePolylineNodesBetween: nothing to delete");
                return nullptr;
            }
        }
    }

    // check if the start point is involved:
    const RS_Vector& polylineStartpoint = polyline->getStartpoint();
    const bool startpointInvolved = isOneOfPoints(polylineStartpoint, node1, node2);

    // check which part of the polyline has to be deleted:
    bool deleteStart = false;
    if (polyline->isClosed()) {
        bool found = false;
        double length1 = 0.0;
        double length2 = 0.0;
        RS_Entity* e = polyline->firstEntity();

        if (startpointInvolved) {
            if (e->isAtomic()) {
                const auto* atomic = static_cast<RS_AtomicEntity*>(e);
                length1 += atomic->getLength();
            }
            e = polyline->nextEntity();
        }
        for (; e != nullptr; e = polyline->nextEntity()) {
            if (e->isAtomic()) {
                const auto* atomic = static_cast<RS_AtomicEntity*>(e);

                if (isOneOfPoints(atomic->getStartpoint(), node1, node2)) {
                    found = !found;
                }

                if (found) {
                    length2 += atomic->getLength();
                }
                else {
                    length1 += atomic->getLength();
                }
            }
        }
        if (length1 < length2) {
            deleteStart = true;
        }
        else {
            deleteStart = false;
        }
    }

    auto* newPolyline = new RS_Polyline(nullptr);
    newPolyline->setClosed(polyline->isClosed());
    if (startpointInvolved && deleteStart && polyline->isClosed()) {
        newPolyline->setNextBulge(0.0);
        newPolyline->addVertex(polylineStartpoint);
    }

    // copy polyline and drop deleted nodes:
    bool first = true;
    bool removing = deleteStart;
    bool done = false;
    bool nextIsStraight = false;
    const RS_Entity* lastEntity = polyline->lastEntity();

    double bulge = 0.0;

    for (const auto e : *polyline) {
        if (e->isAtomic()) {
            const auto atomic = static_cast<RS_AtomicEntity*>(e);
            if (atomic->rtti() == RS2::EntityArc) {
                const auto* arc = static_cast<RS_Arc*>(atomic);
                bulge = arc->getBulge();
            }
            else {
                bulge = 0.0;
            }

            const RS_Vector& endpoint = atomic->getEndpoint();
            const RS_Vector& startpoint = atomic->getStartpoint();

            // last entity is closing entity and will be added below with endPolyline()
            if (e == lastEntity && polyline->isClosed()) {
                continue;
            }

            // first vertex (startpoint)
            if (first) {
                if (!removing) {
                    newPolyline->setNextBulge(bulge);
                    newPolyline->addVertex(startpoint);
                    first = false;
                }
            }

            // stop removing nodes:
            if (removing && isOneOfPoints(endpoint, node1, node2)) {
                removing = false;
                done = true;
                if (!first) {
                    nextIsStraight = true;
                }
            }

            // normal node (not deleted):
            if (!removing && (!done || !deleteStart)) {
                if (nextIsStraight) {
                    bulge = 0.0;
                    nextIsStraight = false;
                }
                newPolyline->setNextBulge(bulge);
                newPolyline->addVertex(endpoint);
            }
            else {
                // drop deleted node:
            }

            // start to remove nodes from now on:
            if (!done && !removing && isOneOfPoints(endpoint, node1, node2)) {
                removing = true;
            }

            if (done) {
                done = false;
            }
        }
        else {
            // fixme - is it possible at all?
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::deletePolylineNodesBetween: Polyline contains non-atomic entities");
        }
    }

    newPolyline->setNextBulge(polyline->getClosingBulge());
    newPolyline->endPolyline();

    ctx.replace(polyline, newPolyline);

    return newPolyline;
}

/**
 * Trims two segments of a polyline all nodes between the two trim segments
 * are removed.
 *
 * @param polyline The polyline entity.
 * @param segment1 First segment to trim.
 * @param segment2 Second segment to trim.
 * @param ctx
 *
 * @return Pointer to the new polyline or nullptr.
 */
RS_Polyline* RS_Modification::polylineTrim(RS_Polyline* polyline, RS_AtomicEntity& segment1, RS_AtomicEntity& segment2,
                                           LC_DocumentModificationBatch& ctx) {
    Q_ASSERT(segment1.getParent() == polyline || segment2.getParent() == polyline);
    Q_ASSERT(&segment1 != &segment2);

    RS_VectorSolutions sol;
    sol = RS_Information::getIntersection(&segment1, &segment2, false);

    if (sol.getNumber() == 0) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::polylineTrim: segments cannot be trimmed");
        return nullptr;
    }

    // check which segment comes first in the polyline:
    RS_AtomicEntity* firstSegment = nullptr;
    if (polyline->findEntity(&segment1) > polyline->findEntity(&segment2)) {
        firstSegment = &segment2;
    }
    else {
        firstSegment = &segment1;
    }

    // find out if we need to trim towards the open part of the polyline
    const bool reverseTrim = !RS_Math::isSameDirection(firstSegment->getDirection1(), firstSegment->getStartpoint().angleTo(sol.get(0)),
                                                       M_PI_2);

    auto* newPolyline = new RS_Polyline(nullptr);
    const bool polylineClosed = polyline->isClosed();
    newPolyline->setClosed(polylineClosed);

    // normal trimming: start removing nodes at trim segment. ends stay the same
    if (!reverseTrim) {
        // copy polyline, trim segments and drop between nodes:
        bool first = true;
        bool removing = false;
        bool nextIsStraight = false;
        const RS_Entity* lastEntity = polyline->lastEntity();
        for (const auto e : *polyline) {
            if (e->isAtomic()) {
                double bulge = 0.0;
                if (e->rtti() == RS2::EntityArc) {
                    bulge = static_cast<RS_Arc*>(e)->getBulge();
                }
                else {
                    bulge = 0.0;
                }

                // last entity is closing entity and will be added below with endPolyline()
                if (e == lastEntity && polylineClosed) {
                    continue;
                }

                // first vertex (startpoint)
                if (first) {
                    newPolyline->setNextBulge(bulge);
                    newPolyline->addVertex(e->getStartpoint());
                    first = false;
                }

                const bool isBoundarySegment = e == &segment1 || e == &segment2;
                // trim and start removing nodes:
                if (!removing && isBoundarySegment) {
                    newPolyline->setNextBulge(0.0);
                    newPolyline->addVertex(sol.get(0));
                    removing = true;
                    nextIsStraight = true;
                }

                // stop removing nodes:
                else if (removing && isBoundarySegment) {
                    removing = false;
                }

                // normal node (not deleted):
                if (!removing) {
                    if (nextIsStraight) {
                        newPolyline->setNextBulge(0.0);
                        nextIsStraight = false;
                    }
                    else {
                        newPolyline->setNextBulge(bulge);
                    }
                    newPolyline->addVertex(e->getEndpoint());
                }
            }
            else {
                // fixme - how it could be?
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::polylineTrim: Polyline contains non-atomic entities");
            }
        }
    }
    // reverse trimming: remove nodes at the ends and keep those in between
    else {
        // copy polyline, trim segments and drop between nodes:

        bool removing = true;
        bool nextIsStraight = false;
        const RS_Entity* lastEntity = polyline->lastEntity();
        for (const auto e : *polyline) {
            if (e->isAtomic()) {
                double bulge = 0.0;
                if (e->rtti() == RS2::EntityArc) {
                    bulge = static_cast<RS_Arc*>(e)->getBulge();
                }
                else {
                    bulge = 0.0;
                }

                // last entity is closing entity and will be added below with endPolyline()
                if (e == lastEntity && polylineClosed) {
                    continue;
                }

                const bool isBoundarySegment = e == &segment1 || e == &segment2;
                // trim and stop removing nodes:
                if (removing && isBoundarySegment) {
                    newPolyline->setNextBulge(0.0);
                    // start of new polyline:
                    newPolyline->addVertex(sol.get(0));
                    removing = false;
                    nextIsStraight = true;
                }
                else if (!removing && isBoundarySegment) {
                    // start removing nodes again:
                    newPolyline->setNextBulge(0.0);
                    // start of new polyline:
                    newPolyline->addVertex(sol.get(0));
                    removing = true;
                }

                // normal node (not deleted):
                if (!removing) {
                    if (nextIsStraight) {
                        newPolyline->setNextBulge(0.0);
                        nextIsStraight = false;
                    }
                    else {
                        newPolyline->setNextBulge(bulge);
                    }
                    newPolyline->addVertex(e->getEndpoint());
                }
            }
            else {
                // fixme - how it could be?
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::polylineTrim: Polyline contains non-atomic entities");
            }
        }
    }

    newPolyline->setNextBulge(polyline->getClosingBulge());
    newPolyline->endPolyline();
    newPolyline->updateEndpoints();

    // add new polyline:
    ctx.replace(polyline, newPolyline);
    return newPolyline;
}

/**
 * Moves all selected entities with the given data for the move
 * modification.
 */
void RS_Modification::move(const RS_MoveData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                           LC_DocumentModificationBatch& ctx) {
    const int numberOfCopies = data.obtainNumberOfCopies();
    for (const auto e : entitiesList) {
        // Create new entities
        for (int num = 1; num <= numberOfCopies; num++) {
            RS_Entity* clone = getClone(forPreviewOnly, e);
            clone->move(data.offset * num);
            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= entitiesList;
    }
    ctx.setActiveLayer = data.useCurrentLayer;
    ctx.setActivePen = data.useCurrentAttributes;
    ctx.success = true;
}

RS_Entity* RS_Modification::getClone(const bool forPreviewOnly, const RS_Entity* e) {
    RS_Entity* result = nullptr;
    if (forPreviewOnly) {
        const int rtti = e->rtti();
        switch (rtti) {
            case RS2::EntityText:
            case RS2::EntityMText: {
                // fixme - sand - ucs - BAD dependency, rework.
                const bool drawTextAsDraftInPreview = LC_GET_ONE_BOOL("Render", "DrawTextsAsDraftInPreview", true);
                if (drawTextAsDraftInPreview) {
                    result = e->cloneProxy();
                }
                else {
                    result = e->clone();
                }
                break;
            }
            case RS2::EntityImage: {
                result = e->cloneProxy();
                break;
            }
            default:
                result = e->clone();
        }
    }
    else {
        result = e->clone();
    }

    return result;
}

bool RS_Modification::alignRef(const LC_AlignRefData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                               LC_DocumentModificationBatch& ctx) {
    constexpr int numberOfCopies = 1; /*data.obtainNumberOfCopies();*/
    const RS_Vector offset = data.offset;
    // too slow:
    for (const auto e : entitiesList) {
        // Create new entities
        for (int num = 1; num <= numberOfCopies; num++) {
            RS_Entity* clone = getClone(forPreviewOnly, e);
            clone->rotate(data.rotationCenter, data.rotationAngle);
            if (data.scale && LC_LineMath::isMeaningful(data.scaleFactor - 1.0)) {
                clone->scale(data.rotationCenter, data.scaleFactor);
            }
            clone->move(offset * num);
            ctx.replace(e, clone);
        }
    }
    return true;
}

/**
 * Offset all selected entities with the given mouse position and distance
 *
 *@Author: Dongxu Li
 */
bool RS_Modification::offset(const RS_OffsetData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                             LC_DocumentModificationBatch& ctx) {
    const int numberOfCopies = data.obtainNumberOfCopies();
    // Create new entities
    // too slow:
    for (const auto e : entitiesList) {
        for (int num = 1; num <= numberOfCopies; num++) {
            const auto clone = getClone(forPreviewOnly, e);
            //highlight is used by trim actions. do not carry over flag
            clone->setHighlighted(false);

            if (!clone->offset(data.coord, num * data.distance)) {
                delete clone;
                continue;
            }
            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= entitiesList;
    }

    return true;
}

/**
 * Rotates all selected entities with the given data for the rotation.
 */
bool RS_Modification::rotate(const RS_RotateData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                             LC_DocumentModificationBatch& ctx) {
    // Create new entities
    const int numberOfCopies = data.obtainNumberOfCopies();
    for (const auto e : entitiesList) {
        for (int num = 1; num <= numberOfCopies; num++) {
            RS_Entity* clone = getClone(forPreviewOnly, e);

            const double rotationAngle = data.angle * num;
            clone->rotate(data.center, rotationAngle);

            bool rotateTwice = data.twoRotations;
            const double distance = data.refPoint.distanceTo(data.center);
            if (distance < RS_TOLERANCE) {
                rotateTwice = false;
            }

            if (rotateTwice) {
                RS_Vector rotatedRefPoint = data.refPoint;
                rotatedRefPoint.rotate(data.center, rotationAngle);

                double secondRotationAngle = data.secondAngle;
                if (data.secondAngleIsAbsolute) {
                    secondRotationAngle -= rotationAngle;
                }
                clone->rotate(rotatedRefPoint, secondRotationAngle);
            }
            ctx += clone;
        }
    }
    if (!data.keepOriginals) {
        ctx -= entitiesList;
    }
    return true;
}

/**
 * Moves all selected entities with the given data for the scale
 * modification.
 */
bool RS_Modification::scale(const RS_ScaleData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                            LC_DocumentModificationBatch& ctx) {
    QList<RS_Entity*> entitiesToScale;
    QList<RS_Entity*> createdEntities;

    for (const auto e : entitiesList) {
        RS_Entity* entityToScale = e;
        if (e->rtti() == RS2::EntityImage) {
            entityToScale = getClone(forPreviewOnly, e);
            createdEntities.append(entityToScale);
        }
        else if (data.isotropicScaling) {
            // do nothing, scale original entity
        }
        else {
            const RS2::EntityType rtti = e->rtti();
            if (rtti == RS2::EntityCircle) {
                //non-isotropic scaling, replacing selected circles with ellipses
                const auto* c = static_cast<RS_Circle*>(e);
                entityToScale = new RS_Ellipse{nullptr, {c->getCenter(), {c->getRadius(), 0.}, 1., 0., 0., false}};
                createdEntities.append(entityToScale);
            }
            else if (rtti == RS2::EntityArc) {
                //non-isotropic scaling, replacing selected arcs with ellipses
                const auto* c = static_cast<RS_Arc*>(e);
                entityToScale = new RS_Ellipse{
                    nullptr,
                    {c->getCenter(), {c->getRadius(), 0.}, 1.0, c->getAngle1(), c->getAngle2(), c->isReversed()}
                };
                createdEntities.append(entityToScale);
            }
            // todo - non isotropicScaling - blocks? splines? images?  Should they be also affected?
        }
        entitiesToScale.push_back(entityToScale);
    }

    const int numberOfCopies = data.obtainNumberOfCopies();

    // Create new entities
    for (const RS_Entity* e : entitiesToScale) {
        if (e != nullptr) {
            for (int num = 1; num <= numberOfCopies; num++) {
                RS_Entity* clone = getClone(forPreviewOnly, e);
                clone->scale(data.referencePoint, RS_Math::pow(data.factor, num));
                ctx += clone;
            }
        }
    }

    // delete temporary entities created for scale
    for (const auto e : createdEntities) {
        delete e;
    }

    if (!data.keepOriginals) {
        ctx -= entitiesList;
    }

    return true;
}

/**
 * Mirror all selected entities with the given data for the mirror
 * modification.
 */

bool RS_Modification::mirror(const RS_MirrorData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                             LC_DocumentModificationBatch& ctx) {
    //    int numberOfCopies = obtainNumberOfCopies(data);
    constexpr int numberOfCopies = 1;
    // fixme - think about support of multiple copies.... may it be something like moving the central point of selection? Like mirror+move?

    // Create new entities
    for (const auto e : entitiesList) {
        for (int num = 1; num <= numberOfCopies; ++num) {
            RS_Entity* clone = getClone(forPreviewOnly, e);
            clone->mirror(data.axisPoint1, data.axisPoint2);
            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= entitiesList;
    }

    return true;
}

/**
 * Rotates entities around two centers with the given parameters.
 */
bool RS_Modification::rotate2(const RS_Rotate2Data& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                              LC_DocumentModificationBatch& ctx) {
    const int numberOfCopies = data.obtainNumberOfCopies();

    // Create new entities
    for (const auto e : entitiesList) {
        for (int num = 1; num <= numberOfCopies; num++) {
            RS_Entity* clone = getClone(forPreviewOnly, e);

            const double angle1ForCopy = /*data.sameAngle1ForCopies ?  data.angle1 :*/ data.angle1 * num;
            const double angle2ForCopy = data.sameAngle2ForCopies ? data.angle2 : data.angle2 * num;
            clone->rotate(data.center1, angle1ForCopy);

            RS_Vector center2 = data.center2;
            center2.rotate(data.center1, angle1ForCopy);
            clone->rotate(center2, angle2ForCopy);

            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= entitiesList;
    }
    return true;
}

/**
 * Moves and rotates entities with the given parameters.
 */
bool RS_Modification::moveRotate(const RS_MoveRotateData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                                 LC_DocumentModificationBatch& ctx) {
    const int numberOfCopies = data.obtainNumberOfCopies();

    // Create new entities
    for (const auto entity : entitiesList) {
        for (int num = 1; num <= numberOfCopies; ++num) {
            RS_Entity* clone = getClone(forPreviewOnly, entity);

            const RS_Vector& offset = data.offset * num;
            clone->move(offset);
            const double angleForCopy = data.sameAngleForCopies ? data.angle : data.angle * num;
            clone->rotate(data.referencePoint + offset, angleForCopy);

            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= entitiesList;
    }
    return true;
}

/**
 * Trims or extends the given trimEntity to the intersection point of the
 * trimEntity and the limitEntity.
 *
 * @param trimCoord Coordinate which defines which endpoint of the
 *   trim entity to trim.
 * @param trimEntity Entity which will be trimmed.
 * @param limitCoord Coordinate which defines the intersection to which the
 *    trim entity will be trimmed.
 * @param limitEntity Entity to which the trim entity will be trimmed.
 * @param both true: Trim both entities. false: trim trimEntity only.
 * @param ctx
 *
 */
LC_TrimResult RS_Modification::trim(const RS_Vector& trimCoord, RS_AtomicEntity* trimEntity, const RS_Vector& limitCoord,
                                    RS_Entity* limitEntity, const bool both, LC_DocumentModificationBatch& ctx) {
    Q_ASSERT(trimEntity != nullptr && limitEntity != nullptr);

    if (both && !limitEntity->isAtomic()) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::trim: limitEntity is not atomic");
    }

    LC_TrimResult result;

    if (trimEntity->isLocked() || !trimEntity->isVisible()) {
        return result;
    }

    RS_VectorSolutions sol = findIntersection(*trimEntity, *limitEntity);

    //if intersection are in start or end point can't trim/extend in this point, remove from solution. sf.net #3537053
    if (trimEntity->rtti() == RS2::EntityLine) {
        const auto* line = static_cast<RS_Line*>(trimEntity);
        for (size_t i = 0; i < sol.size(); i++) {
            const RS_Vector v = sol.at(i);
            if (v == line->getStartpoint()) {
                sol.removeAt(i);
            }
            else if (v == line->getEndpoint()) {
                sol.removeAt(i);
            }
        }
    }

    if (!sol.hasValid()) {
        if (both) {
            return trim(limitCoord, static_cast<RS_AtomicEntity*>(limitEntity), trimCoord, trimEntity, false, ctx);
        }
        return result;
    }

    RS_AtomicEntity* trimmed1 = nullptr;
    RS_AtomicEntity* trimmed2 = nullptr;

    // FIXME - sand - ARC IS TRIMMED INCORRECTLY BY LINE, ONLY ONE INTERSECTION IS SUPPORTED INSTEAD OF 2!!!!
    // FIXME - EXPAND ENTITIES TRIM/TRIM COMBINATIONS

    if (trimEntity->rtti() == RS2::EntityCircle) {
        // convert a circle into a trimmable arc, need to start from intersections
        trimmed1 = trimCircle(static_cast<RS_Circle*>(trimEntity), trimCoord, sol);
    }
    else {
        trimmed1 = static_cast<RS_AtomicEntity*>(trimEntity->clone());
    }

    // trim trim entity
    size_t ind = 0;
    RS_Vector is(false);
    RS_Vector is2(false);

    if (trimEntity->trimmable()) {
        is = trimmed1->prepareTrim(trimCoord, sol);
    }
    else {
        is = sol.getClosest(limitCoord, nullptr, &ind);
        is2 = sol.get(ind == 0 ? 1 : 0);
    }

    // remove limit entity from view:
    const bool trimBoth = both && !limitEntity->isLocked() && limitEntity->isVisible();
    if (trimBoth) {
        trimmed2 = static_cast<RS_AtomicEntity*>(limitEntity->clone());
    }

    trimEnding(trimCoord, trimmed1, is);

    // trim limit entity:
    if (trimBoth) {
        if (trimmed2->trimmable()) {
            is2 = trimmed2->prepareTrim(limitCoord, sol);
        }
        else {
            is2 = sol.getClosest(trimCoord);
        }

        trimEnding(limitCoord, trimmed2, is2);
    }

    ctx += trimmed1;
    ctx -= trimEntity;

    if (trimBoth) {
        ctx += trimmed2;
        ctx -= limitEntity;
    }

    result.result = true;
    result.trimmed1 = trimmed1;
    result.trimmed2 = trimmed2;
    result.intersection1 = is;
    result.intersection2 = is2;

    if (trimmed1->isArc()) {
        result.intersection1 = trimmed1->getStartpoint();
        result.intersection2 = trimmed1->getEndpoint();
    }
    return result;
}

void RS_Modification::trimEnding(const RS_Vector& trimCoord, RS_AtomicEntity* trimmed1, const RS_Vector& is) {
    const RS2::Ending ending = trimmed1->getTrimPoint(trimCoord, is);
    switch (ending) {
        case RS2::EndingStart: {
            trimmed1->trimStartpoint(is);
            break;
        }
        case RS2::EndingEnd: {
            trimmed1->trimEndpoint(is);
            break;
        }
        default:
            break;
    }
}

/**
 * Trims or extends the given trimEntity by the given amount.
 *
 * @param trimCoord Coordinate which defines which endpoint of the
 *   trim entity to trim.
 * @param entityToTrim Entity which will be trimmed.
 * @param dist Amount to trim by.
 * @param trimBoth
 * @param trimStart
 * @param trimEnd
 * @param ctx
 */
RS_Entity* RS_Modification::trimAmount(const RS_Vector& trimCoord, RS_AtomicEntity* entityToTrim, const double dist, const bool trimBoth,
                                       bool& trimStart, bool& trimEnd, LC_DocumentModificationBatch& ctx) {
    Q_ASSERT(entityToTrim != nullptr);

    if (entityToTrim->isLocked() || !entityToTrim->isVisible()) {
        return nullptr;
    }

    // remove trim entity:
    const auto trimmed = static_cast<RS_AtomicEntity*>(entityToTrim->clone());

    // trim trim entity
    trimStart = false;
    trimEnd = false;
    if (trimBoth) {
        const RS_Vector isStart = trimmed->getNearestDist(-dist, trimmed->getStartpoint());
        const RS_Vector isEnd = trimmed->getNearestDist(-dist, trimmed->getEndpoint());

        trimmed->trimStartpoint(isStart);
        trimmed->trimEndpoint(isEnd);
        trimStart = true;
        trimEnd = true;
    }
    else {
        const RS_Vector is = trimmed->getNearestDist(-dist, trimCoord);

        if (trimCoord.distanceTo(trimmed->getStartpoint()) < trimCoord.distanceTo(trimmed->getEndpoint())) {
            trimmed->trimStartpoint(is);
            trimStart = true;
        }
        else {
            trimmed->trimEndpoint(is);
            trimEnd = true;
        }
    }

    // add new trimmed trim entity:
    ctx += trimmed;
    ctx -= entityToTrim;

    return trimmed;
}

/**
 * Cuts the given entity at the given point.
 */
bool RS_Modification::cut(const RS_Vector& cutCoord, RS_AtomicEntity* cutEntity, LC_DocumentModificationBatch& ctx) {
#ifndef EMU_C99
    using std::isnormal;
#endif

    Q_ASSERT(cutEntity != nullptr);

    if (cutEntity->isLocked() || !cutEntity->isVisible()) {
        return false;
    }
    if (!cutCoord.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::cut: Point invalid.");
        return false;
    }

    // cut point is at endpoint of entity:
    if (cutCoord.distanceTo(cutEntity->getStartpoint()) < RS_TOLERANCE || cutCoord.distanceTo(cutEntity->getEndpoint()) < RS_TOLERANCE) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::cut: Cutting point on endpoint");
        return false;
    }

    RS_AtomicEntity* cut1 = nullptr;
    RS_AtomicEntity* cut2 = nullptr;
    double a = NAN;

    switch (cutEntity->rtti()) {
        case RS2::EntityCircle: {
            // convert to a whole 2 pi range arc
            const auto* originalCircle = static_cast<RS_Circle*>(cutEntity);
            a = originalCircle->getCenter().angleTo(cutCoord);
            cut1 = new RS_Arc(cutEntity->getParent(),
                              RS_ArcData(originalCircle->getCenter(), originalCircle->getRadius(), a, a + 2. * M_PI, false));
            // cut2 is nullptr by default
            break;
        }
        case RS2::EntitySplinePoints: {
            // interpolation spline can be closed
            // so we cannot use the default implementation
            // fixme - review cutting for spline points!!!!
            cut2 = static_cast<LC_SplinePoints*>(cutEntity->clone())->cut(cutCoord);
            cut1 = static_cast<RS_AtomicEntity*>(cutEntity->clone());
            break;
        }
        case RS2::EntityEllipse: {
            // handle ellipse arc the using the default method
            // ToDo, to really handle Ellipse Arcs properly, we need to create a new class RS_EllipseArc, keep RS_Ellipse for whole range Ellipses
            const auto* const ellipse = static_cast<const RS_Ellipse*>(cutEntity);
            if (RS_Math::isSameDirection(ellipse->getAngle1(), ellipse->getAngle2(), RS_TOLERANCE_ANGLE) && !/*std::*/
                isnormal(ellipse->getAngle1()) && !/*std::*/isnormal(ellipse->getAngle2())) {
                // whole ellipse, convert to a whole range elliptic arc
                a = ellipse->getEllipseAngle(cutCoord);
                cut1 = new RS_Ellipse{
                    cutEntity->getParent(),
                    RS_EllipseData{ellipse->getCenter(), ellipse->getMajorP(), ellipse->getRatio(), a, a + 2. * M_PI, ellipse->isReversed()}
                };
                break;
            }
            //elliptic arc
            //missing "break;" line is on purpose
            //elliptic arc should be handled by default:
            //do not insert between here and default:
        }
        // fall-through
        default:
            cut1 = static_cast<RS_AtomicEntity*>(cutEntity->clone());
            cut2 = static_cast<RS_AtomicEntity*>(cutEntity->clone());

            cut1->trimEndpoint(cutCoord);
            cut2->trimStartpoint(cutCoord);
    }

    // add new cut entity:
    ctx += cut1;
    if (cut2 != nullptr) {
        ctx += cut2;
    }

    ctx -= cutEntity;

    return true;
}

/**
 * Bevels a corner.
 *
 * @param atomicToTrim
 * @param trimPoint
 * @param ending
 */

bool RS_Modification::trimAtomicByEnding(RS_AtomicEntity* atomicToTrim, const RS_Vector& trimPoint, const RS2::Ending ending) {
    bool fromStart = true;
    switch (ending) {
        case RS2::EndingStart:
            atomicToTrim->trimStartpoint(trimPoint);
            break;
        case RS2::EndingEnd:
            atomicToTrim->trimEndpoint(trimPoint);
            fromStart = false;
            break;
        default:
            break;
    }
    return fromStart;
}

LC_BevelResult RS_Modification::bevel(const RS_Vector& coord1, RS_AtomicEntity* entity1, const RS_Vector& coord2, RS_AtomicEntity* entity2,
                                      RS_BevelData& data, [[maybe_unused]] bool previewOnly, LC_DocumentModificationBatch& ctx) {
    Q_ASSERT(entity1 != nullptr && entity2 != nullptr);

    LC_BevelResult result;

    if (entity1->isLocked() || !entity1->isVisible() || entity2->isLocked() || !entity2->isVisible()) {
        result.error = LC_BevelResult::ERR_VISIBILITY;
        return result;
    }

    // check that both are lines
    if (!(entity1->is(RS2::EntityLine) && entity2->is(RS2::EntityLine))) {
        result.error = LC_BevelResult::ERR_NOT_LINES;
        return result;
    }

    // getting intersection;
    const RS_VectorSolutions intersections = RS_Information::getIntersection(entity1, entity2, false);

    if (intersections.isEmpty()) {
        result.error = LC_BevelResult::ERR_NO_INTERSECTION;
        return result;
    }

    RS_AtomicEntity* trimmed1 = nullptr;
    RS_AtomicEntity* trimmed2 = nullptr;

    int entity1IndexInPolyline = 0;
    int entity2IndexInPolyline = 0;
    unsigned int polylineSegmentsCount = 0;
    RS_Polyline* polylineClone = nullptr;
    RS_Polyline* samePolyline = nullptr;

    bool isPolyline = false;
    if (data.trim) {
        // find out whether we're bevelling within a polyline:
        auto parent1 = entity1->getParent();
        auto parent2 = entity2->getParent();

        if ((parent1 != nullptr && parent1->rtti() == RS2::EntityPolyline) || (parent2 != nullptr && parent2->rtti() ==
            RS2::EntityPolyline)) {
            if (parent1 != parent2) {
                // different polylines
                data.trim = false; // just create bevel
            }
            else {
                // check that adjacent segments of polyline are selected.
                // we'll modify polyline only for adjacent segments (or for first/last ones), as otherwise it
                // may lead to breaking of the polyline structure and quite unpredictable
                // behavior. Example - bevel on two segments that looks like X, with no vertex in intesection
                samePolyline = static_cast<RS_Polyline*>(parent1);
                entity1IndexInPolyline = samePolyline->findEntity(entity1);
                entity2IndexInPolyline = samePolyline->findEntity(entity2);

                polylineSegmentsCount = samePolyline->count();

                bool polylineIsClosedOrEndpointsAreTheSame = samePolyline->isClosed() || LC_LineMath::isNotMeaningfulDistance(
                    samePolyline->getStartpoint(), samePolyline->getEndpoint());

                unsigned int delta = std::abs(entity1IndexInPolyline - entity2IndexInPolyline);
                bool adjacentSegments = (delta == 1 ) || (polylineIsClosedOrEndpointsAreTheSame && (delta == polylineSegmentsCount - 1));

                if (adjacentSegments) {
                }
                else {
                    data.trim = false; // don't modify polyline for non-adjacent segments.
                }

                isPolyline = data.trim; // modify polyline only if we could trim
            }
        }
    }

    if (data.trim && isPolyline) {
        polylineClone = static_cast<RS_Polyline*>(samePolyline->clone());
        trimmed1 = static_cast<RS_AtomicEntity*>(polylineClone->entityAt(entity1IndexInPolyline));
        trimmed2 = static_cast<RS_AtomicEntity*>(polylineClone->entityAt(entity2IndexInPolyline));
    }
    else {
        trimmed1 = static_cast<RS_AtomicEntity*>(entity1->clone());
        trimmed2 = static_cast<RS_AtomicEntity*>(entity2->clone());
    }

    // trim first entity to intersection
    RS_Vector intersectionPoint = intersections.getClosest(coord2);
    result.intersectionPoint = intersectionPoint;
    const RS2::Ending ending1 = trimmed1->getTrimPoint(coord1, intersectionPoint);
    bool start1 = trimAtomicByEnding(trimmed1, intersectionPoint, ending1);
    result.trimStart1 = start1;

    // trim second entity to intersection
    intersectionPoint = intersections.getClosest(coord1);
    const RS2::Ending ending2 = trimmed2->getTrimPoint(coord2, intersectionPoint);
    bool start2 = trimAtomicByEnding(trimmed2, intersectionPoint, ending2);
    result.trimStart2 = start2;

    // find definitive bevel points
    const RS_Vector bevelPoint1 = trimmed1->getNearestDistToEndpoint(data.length1, start1);
    const RS_Vector bevelPoint2 = trimmed2->getNearestDistToEndpoint(data.length2, start2);

    // final trim
    if (data.trim) {
        trimAtomicByEnding(trimmed1, bevelPoint1, ending1);
        trimAtomicByEnding(trimmed2, bevelPoint2, ending2);
    }

    // add bevel line:
    auto* bevel = new RS_Line(nullptr, bevelPoint1, bevelPoint2);

    if (isPolyline) {
        if (data.trim) {
            int indexInPolylineToInsert = entity1IndexInPolyline;
            //Verify correct order segment in polylines
            if (entity1IndexInPolyline > entity2IndexInPolyline) {
                //inverted, reorder it (swap).
                entity1IndexInPolyline = entity2IndexInPolyline;
                entity2IndexInPolyline = indexInPolylineToInsert;

                RS_AtomicEntity* trimmedTmp = trimmed1;
                trimmed1 = trimmed2;
                trimmed2 = trimmedTmp;
            }
            indexInPolylineToInsert = entity1IndexInPolyline;

            // insert bevel at the right position:
            if (entity1IndexInPolyline == 0 && entity2IndexInPolyline == polylineSegmentsCount - 1 && polylineSegmentsCount > 2) {
                //bevel are from last and first segments, add at the end
                if (LC_LineMath::isMeaningfulDistance(trimmed2->getEndpoint(), bevel->getStartpoint())) {
                    bevel->reverse();
                }
                indexInPolylineToInsert = entity2IndexInPolyline;
            }
            else {
                //consecutive segments
                if (LC_LineMath::isMeaningfulDistance(trimmed1->getEndpoint(), bevel->getStartpoint())) {
                    bevel->reverse();
                }
            }
            bevel->setParent(polylineClone);
            polylineClone->insertEntity(indexInPolylineToInsert + 1, bevel);
            polylineClone->updateEndpoints();

            result.polyline = polylineClone;
            result.isPolyline = true;

            ctx.replace(samePolyline, polylineClone);
        }
        else {
            ctx += bevel;
        }
    }
    else {
        if (data.trim) {
            ctx.replace(entity2, trimmed2);
            ctx.replace(entity1, trimmed1);
        }
        ctx += bevel;
    }

    if (!data.trim) {
        delete trimmed1;
        delete trimmed2;
        trimmed1 = nullptr;
        trimmed2 = nullptr;
    }

    result.bevel = bevel;
    result.trimmed1 = trimmed1;
    result.trimmed2 = trimmed2;

    return result;
}

/**
 * Rounds a corner.
 *
 * @param coord Mouse coordinate to specify the rounding.
 * @param coord1
 * @param entity1 First entity of the corner.
 * @param coord2
 * @param entity2 Second entity of the corner.
 * @param data Radius and trim flag.
 * @param ctx
 */
LC_RoundResult RS_Modification::round(const RS_Vector& coord, const RS_Vector& coord1, RS_AtomicEntity* entity1, const RS_Vector& coord2,
                                      RS_AtomicEntity* entity2, RS_RoundData& data, LC_DocumentModificationBatch& ctx) {
    Q_ASSERT(entity1 != nullptr && entity2 != nullptr);

    LC_RoundResult result;

    if (entity1->isLocked() || !entity1->isVisible() || entity2->isLocked() || !entity2->isVisible()) {
        result.error = LC_RoundResult::ERR_INPUT;
        return result;
    }

    // create 2 tmp parallels
    QList<RS_Entity*> parallels;
    RS_Creation::createParallel(coord, data.radius, 1, entity1, false, parallels);
    std::unique_ptr<RS_Entity> par1{parallels.empty() ? nullptr : parallels.front()};
    parallels.clear();

    RS_Creation::createParallel(coord, data.radius, 1, entity2, false, parallels);
    std::unique_ptr<RS_Entity> par2{parallels.empty() ? nullptr : parallels.front()};

    if (par1 == nullptr || par2 == nullptr) {
        result.error = LC_RoundResult::NO_PARALLELS;
        return result;
    }

    RS_VectorSolutions parallelsIntersection = RS_Information::getIntersection(par1.get(), par2.get(), false);

    if (parallelsIntersection.getNumber() == 0) {
        result.error = LC_RoundResult::ERR_NO_INTERSECTION;
        return result;
    }

    RS_AtomicEntity* trimmed1 = nullptr;
    RS_AtomicEntity* trimmed2 = nullptr;

    int entity1IndexInPolyline = 0;
    int entity2IndexInPolyline = 0;
    unsigned int polylineSegmentsCount = 0;
    RS_Polyline* polylineClone = nullptr;
    RS_Polyline* samePolyline = nullptr;

    bool samePolylineModification = false;
    if (data.trim) {
        // find out whether we're rounding within a polyline:
        const auto parent1 = entity1->getParent();
        const auto parent2 = entity2->getParent();

        bool firstFromPolyline = parent1 != nullptr && parent1->rtti() == RS2::EntityPolyline;
        bool secondFromPolyline = parent2 != nullptr && parent2->rtti() == RS2::EntityPolyline;
        bool atLeastOneFromPolyline = firstFromPolyline || secondFromPolyline;
        if (atLeastOneFromPolyline) {
            if (parent1 != parent2) {
                // different polylines
                data.trim = false; // just create bevel
            }
            else {
                bool bothAreLines = entity1->is(RS2::EntityLine) && entity2->is(RS2::EntityLine);
                if (!bothAreLines) {
                    //
                    result.error = LC_BevelResult::ERR_NOT_LINES;
                    return result;
                }
                // check that adjacent segments of polyline are selected.
                // we'll modify polyline only for adjacent segments (or for first/last ones), as otherwise it
                // may lead to breaking of the polyline structure and quite unpredictable
                // behavior. Example - bevel on two segments that looks like X, with no vertex in intesection
                samePolyline = static_cast<RS_Polyline*>(parent1);
                entity1IndexInPolyline = samePolyline->findEntity(entity1);
                entity2IndexInPolyline = samePolyline->findEntity(entity2);

                polylineSegmentsCount = samePolyline->count();

                bool polylineIsClosedOrEndpointsAreTheSame = samePolyline->isClosed() || LC_LineMath::isNotMeaningfulDistance(
                    samePolyline->getStartpoint(), samePolyline->getEndpoint());

                int delta = std::abs(entity1IndexInPolyline - entity2IndexInPolyline);
                bool adjacentSegments = (delta == 1 ) || (polylineIsClosedOrEndpointsAreTheSame && delta == polylineSegmentsCount - 1);

                if (adjacentSegments) {
                }
                else {
                    data.trim = false; // don't modify polyline for non-adjacent segments.
                }
                samePolylineModification = data.trim; // modify polyline only if we could trim
            }
        }
    }
    result.isPolyline = samePolylineModification;

    // there might be two intersections: choose the closest:
    const RS_Vector is = parallelsIntersection.getClosest(coord);
    const RS_Vector p1 = entity1->getNearestPointOnEntity(is, false);
    const RS_Vector p2 = entity2->getNearestPointOnEntity(is, false);
    const double ang1 = is.angleTo(p1);
    const double ang2 = is.angleTo(p2);
    const bool reversed = RS_Math::getAngleDifference(ang1, ang2) > M_PI;

    auto* arc = new RS_Arc(nullptr, RS_ArcData(is, data.radius, ang1, ang2, reversed));

    if (data.trim && samePolylineModification) {
        polylineClone = static_cast<RS_Polyline*>(samePolyline->clone());
        trimmed1 = static_cast<RS_AtomicEntity*>(polylineClone->entityAt(entity1IndexInPolyline));
        trimmed2 = static_cast<RS_AtomicEntity*>(polylineClone->entityAt(entity2IndexInPolyline));
    }
    else {
        trimmed1 = static_cast<RS_AtomicEntity*>(entity1->clone());
        trimmed2 = static_cast<RS_AtomicEntity*>(entity2->clone());
    }

    if (data.trim) {
        // trim entities to intersection
        RS_VectorSolutions entitiesIntersection = RS_Information::getIntersection(entity1, entity2, false);

        result.trim1Mode = roundingTrimEntity(entitiesIntersection, trimmed1, arc, p1, coord2, coord1);
        result.trim2Mode = roundingTrimEntity(entitiesIntersection, trimmed2, arc, p2, coord1, coord2);
        result.trimmed = true;
    }

    // add rounding:
    if (samePolylineModification) {
        if (data.trim) {
            int indexInPolylineToInsert = entity1IndexInPolyline;
            //Verify correct order segment in polylines
            if (entity1IndexInPolyline > entity2IndexInPolyline) {
                //inverted, reorder it (swap).
                entity1IndexInPolyline = entity2IndexInPolyline;
                entity2IndexInPolyline = indexInPolylineToInsert;

                RS_AtomicEntity* trimmedTmp = trimmed1;
                trimmed1 = trimmed2;
                trimmed2 = trimmedTmp;
            }
            indexInPolylineToInsert = entity1IndexInPolyline;

            // insert bevel at the right position:
            if (entity1IndexInPolyline == 0 && entity2IndexInPolyline == polylineSegmentsCount - 1 && polylineSegmentsCount > 2) {
                //bevel are from last and first segments, add at the end
                if (LC_LineMath::isMeaningfulDistance(trimmed2->getEndpoint(), arc->getStartpoint())) {
                    arc->reverse();
                }
                indexInPolylineToInsert = entity2IndexInPolyline;
            }
            else {
                //consecutive segments
                if (LC_LineMath::isMeaningfulDistance(trimmed1->getEndpoint(), arc->getStartpoint())) {
                    arc->reverse();
                }
            }
            arc->setParent(polylineClone);
            polylineClone->insertEntity(indexInPolylineToInsert + 1, arc);
            polylineClone->updateEndpoints();

            result.polyline = polylineClone;
            result.isPolyline = true;

            ctx.replace(samePolyline, polylineClone);
        }
        else {
            ctx += arc;
        }
    }
    else {
        if (data.trim) {
            ctx.replace(entity2, trimmed2);
            ctx.replace(entity1, trimmed1);
        }
        ctx += arc;
    }

    if (!data.trim) {
        delete trimmed1;
        delete trimmed2;
        trimmed1 = nullptr;
        trimmed2 = nullptr;
    }

    result.round = arc;
    result.trimmed1 = trimmed1;
    result.trimmed2 = trimmed2;
    result.trimmingPoint1 = p1;
    result.trimmingPoint2 = p2;

    return result;
}

LC_RoundResult::TrimMode RS_Modification::roundingTrimEntity(const RS_VectorSolutions& entitiesIntersection, RS_AtomicEntity*& entityToTrim,
                                                             const RS_Arc* arc, const RS_Vector& trimPoint,
                                                             const RS_Vector& selectionPoint1, const RS_Vector& selectionPoint2) {
    const RS_Vector is2 = entitiesIntersection.getClosest(selectionPoint1);
    LC_RoundResult::TrimMode trimMode = LC_RoundResult::TRIM_CIRCLE;
    const RS2::Ending ending = entityToTrim->getTrimPoint(selectionPoint2, is2);
    switch (ending) {
        case RS2::EndingStart: {
            entityToTrim->trimStartpoint(trimPoint);
            trimMode = LC_RoundResult::TRIM_START;
            break;
        }
        case RS2::EndingEnd: {
            entityToTrim->trimEndpoint(trimPoint);
            trimMode = LC_RoundResult::TRIM_END;
            break;
        }
        default: {
            entityToTrim = trimCircleForRound(entityToTrim, *arc);
            trimMode = LC_RoundResult::TRIM_CIRCLE;
            break;
        }
    }
    return trimMode;
}

/**
 * Stretching.
 */

bool RS_Modification::stretch(const RS_Vector& firstCorner, const RS_Vector& secondCorner, const RS_Vector& offset,
                              const QList<RS_Entity*>& entitiesList, const bool removeOriginals, LC_DocumentModificationBatch& ctx) {
    if (!offset.valid) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Modification::stretch: Offset invalid");
        return false;
    }

    // Create new entities

    for (const auto e : entitiesList) {
        // fixme - sand - iteration over all entities in container???
        /*if (e != nullptr && e->isVisible() && !e->isLocked()) {
            if ((e->isInWindow(firstCorner, secondCorner) ||
                e->hasEndpointsWithinWindow(firstCorner, secondCorner))) {*/
        RS_Entity* clone = e->clone(); // fixme - sand - should we use proxy there?
        clone->stretch(firstCorner, secondCorner, offset);
        ctx += clone;
        if (removeOriginals) {
            ctx -= e;
        }
        /*}
    }*/
    }
    return true;
}

/**
 * Repetitive recursive block of code for the explode() method.
 */
static void updateExplodedChildrenRecursively(const RS_EntityContainer* ec, const RS_Entity* e, RS_Entity* clone,
                                              const RS2::ResolveLevel rl, const bool resolveLayer, const bool resolvePen) {
    if (resolveLayer) {
        clone->setLayer(ec->getLayer());
    }
    else {
        clone->setLayer(e->getLayer());
    }

    if (resolvePen) {
        // fixme - sand - resolve always???
        //clone->setPen(ec->getPen(true));
        clone->setPen(ec->getPen(false));
    }
    else {
        clone->setPen(e->getPen(false));
    }

    clone->update();

    if (clone->isContainer()) {
        const auto subContainer = static_cast<RS_EntityContainer*>(clone);
        // fixme - sand - while we can't iterate just by children and go deeper???
        for (RS_Entity* en : lc::LC_ContainerTraverser{*subContainer, rl}.entities()) {
            if (en != nullptr) {
                // Run the same code for every child recursively
                updateExplodedChildrenRecursively(subContainer, clone, en, rl, resolveLayer, resolvePen);
            }
        }
    }
}

bool RS_Modification::explode(const QList<RS_Entity*>& entitiesList, LC_DocumentModificationBatch& ctx) {
    for (const auto e : entitiesList) {
        if (e->isContainer()) {
            // add entities from container:
            const auto* container = static_cast<RS_EntityContainer*>(e);

            RS2::ResolveLevel resolveLevel = RS2::ResolveNone;
            bool resolvePen = false;
            bool resolveLayer = false;

            const auto containerType = container->rtti(); // fixme - sand - review this logic.
            switch (containerType) {
                case RS2::EntityMText:
                case RS2::EntityText:
                case RS2::EntityHatch:
                case RS2::EntityPolyline: {
                    resolveLevel = RS2::ResolveAll;
                    resolveLayer = true;
                    resolvePen = true;
                    break;
                }
                case RS2::EntityInsert: {
                    resolveLevel = RS2::ResolveNone;
                    resolveLayer = false;
                    resolvePen = false;
                    break;
                }
                default:
                    if (RS2::isDimensionalEntity(containerType)) {
                        resolveLevel = RS2::ResolveNone;
                        resolveLayer = true;
                        resolvePen = true; // fixme - sand - is it so? What about dimstyle?
                    }
                    else {
                        resolveLevel = RS2::ResolveAll;
                        resolveLayer = true;
                        resolvePen = false;
                    }
                    break;
            }

            // fixme - sand - why we use flat list of all dependend entities?
            // fixme - sand - whye we can't just interate by children and go deep for containers?
            auto entities = lc::LC_ContainerTraverser{*container, resolveLevel}.entities();
            for (const RS_Entity* e2 : entities) {
                if (e2 != nullptr) {
                    RS_Entity* clone = e2->clone();
                    ctx += clone;

                    // In order to fix bug #819 and escape similar issues,
                    // we have to update all children of exploded entity,
                    // even those (below the tree) which are not direct
                    // subjects to the current explode() call.
                    updateExplodedChildrenRecursively(container, e2, clone, resolveLevel, resolveLayer, resolvePen);
                }
            }
            ctx -= e;
        }
    }

    return true;
}

bool RS_Modification::explodeTextIntoLetters(const QList<RS_Entity*>& selectedEntitiesList, LC_DocumentModificationBatch& ctx) {
    for (const auto e : selectedEntitiesList) {
        // fixme - create generic iterator function with lambda!
        if (e == nullptr || e->isDeleted() || !e->isVisible()) {
            continue;
        }
        const auto rtti = e->rtti();
        switch (rtti) {
            case RS2::EntityMText: {
                // add letters of text:
                const auto text = dynamic_cast<RS_MText*>(e);
                doExplodeTextIntoLetters(text, ctx);
                ctx -= e; // originals are always deleted
                break;
            }
            case RS2::EntityText: {
                // add letters of text:
                const auto text = dynamic_cast<RS_Text*>(e);
                doExplodeTextIntoLetters(text, ctx);
                ctx -= e; // originals are always deleted
                break;
            }
            default: {
                break;
            }
        }
    }
    return true;
}

bool RS_Modification::doExplodeTextIntoLetters(const RS_MText* text, LC_DocumentModificationBatch& ctx) {
    if (text == nullptr) {
        return false;
    }
    if (text->isLocked() || !text->isVisible()) {
        return false;
    }

    // iterate though lines:
    for (const auto e2 : *text) {
        if (e2 == nullptr) {
            break;
        }
        // text lines:
        if (e2->rtti() == RS2::EntityContainer) {
            const auto line = static_cast<RS_EntityContainer*>(e2);

            // iterate though letters:
            for (const auto e3 : *line) {
                if (e3 == nullptr) {
                    break;
                }

                if (e3->rtti() == RS2::EntityMText) {
                    // super / sub texts:
                    const auto e3MText = static_cast<RS_MText*>(e3);
                    doExplodeTextIntoLetters(e3MText, ctx);
                }
                else if (e3->rtti() == RS2::EntityInsert) {
                    // normal letters:
                    const auto letter = static_cast<RS_Insert*>(e3);

                    const auto letterAsText = new RS_MText(nullptr, RS_MTextData(letter->getInsertionPoint(), text->getHeight(), 100.0,
                                                                                 RS_MTextData::VABottom, RS_MTextData::HALeft,
                                                                                 RS_MTextData::LeftToRight, RS_MTextData::Exact, 1.0,
                                                                                 letter->getName(), text->getStyle(), letter->getAngle(),
                                                                                 RS2::Update));

                    letterAsText->setLayer(text->getLayer());
                    letterAsText->setPen(text->getPen(false));

                    ctx += letterAsText;
                    letterAsText->update();
                }
            }
        }
    }

    return true;
}

bool RS_Modification::doExplodeTextIntoLetters(RS_Text* text, LC_DocumentModificationBatch& ctx) {
    if (text->isLocked() || !text->isVisible()) {
        return false;
    }

    // iterate though letters:
    for (const auto e2 : *text) {
        if (e2 == nullptr) {
            break;
        }

        if (e2->rtti() == RS2::EntityInsert) {
            const auto letter = static_cast<RS_Insert*>(e2);
            const auto letterAsText = new RS_Text(nullptr, RS_TextData(letter->getInsertionPoint(), letter->getInsertionPoint(),
                                                                       text->getHeight(), text->getWidthRel(), RS_TextData::VABaseline,
                                                                       RS_TextData::HALeft, RS_TextData::None,
                                                                       /*text->getTextGeneration(),*/
                                                                       letter->getName(), text->getStyle(), letter->getAngle(),
                                                                       RS2::Update));

            letterAsText->setLayer(text->getLayer());
            letterAsText->setPen(text->getPen(false));

            ctx += letterAsText;
            letterAsText->update();
        }
    }
    return true;
}
