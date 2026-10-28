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
#include "lc_offsetpreviewsnapshot.h"
#include "lc_offsetbatchpreparation.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvefinalinput.h"
#include "../offset_worker/lc_offsetworkerclient.h"
#include <QCborValue>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <cstdint>
#include <new>
#include <stdexcept>

#include "lc_containertraverser.h"
#include "lc_curveoffset.h"
#include "lc_graphicviewport.h"
#include "lc_hyperbola.h"
#include "lc_linemath.h"
#include "lc_offsetresultcheck.h"
#include "lc_offsetcurve.h"
#include "lc_offsetcurvetransform.h"
#include "lc_offsetcurvexdata.h"
#include "rs_document.h"
#include "lc_splinepoints.h"
#include "rs_spline.h"
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
namespace offsetTx = LC_OffsetCurveTransform;

namespace {
      // fixme - sand - add option for intersection tolerance????

    bool acceptedInsertSourceEdit(const RS_Entity& entity) {
        return entity.rtti() != RS2::EntityInsert
               || static_cast<const RS_Insert&>(entity).lastSourceEditSucceeded();
    }

    // RS_Modification::trim() replaces what it trims by a copy it adds to the document, which is right only for
    // an entity the document holds itself. A segment of a polyline would leave a gap in it, and a line of a block
    // reference, dimension or spline would be freed by its owner when that rebuilds its lines, while the undo
    // history still holds it.
    bool isReplaceableByTrim(const RS_Entity& entity) {
        const RS_EntityContainer* parent = entity.getParent();
        return parent == nullptr || parent->isDocument();
    }

    bool isNativeCacheChild(const RS_Entity& entity) {
        return entity.getParent() && entity.getParent()->rtti() == RS2::EntityOffsetCurve;
    }

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
        pen.setLineTypeFromPen(data.pen);
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
                pen.setLineTypeFromPen(data.pen);
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
    Q_ASSERT(destination != nullptr);

    // The insert goes on the layer that is current now: the layer dock makes
    // every layer it sees added the current one, the library's included.
    RS_Layer* const insertLayer = destination->getActiveLayer();
    RS_LayerList* srcLayers = src->getLayerList();
    if (srcLayers != nullptr) {
        for (const RS_Layer* layer : *srcLayers) {
            if (layer == nullptr) {
                continue;
            }
            const QString ln = layer->getName();
            if (destination->findLayer(ln) == nullptr) {
                destination->addLayer(layer->clone());
            }
        }
    }
    // Its line types come along as its layers do.
    destination->mergeLineTypes(*src);

    // Everything read is new to this drawing, and goes on its layers of the same names.
    constexpr unsigned provenance = RS_Entity::Identity | RS_Entity::TableRefs;
    for (const RS_Entity* e : *src) {
        if (e == nullptr || e->rtti() != RS2::EntityInsert) {
            continue;
        }
        const RS_Block* block = static_cast<const RS_Insert*>(e)->getBlockForInsert();
        if (block != nullptr) {
            LC_CopyUtils::doCopyBlock(block, destination, provenance);
        }
    }

    const RS_Vector scaleV = LC_CopyUtils::getInterGraphicsScaleFactor(data.factor, src, destination);

    // fixme - blocks - block name: prefer provided, fallback to auto-generated
    const QString bname = data.blockName.isEmpty() ? getUniqueBlockName(destination) : data.blockName;
    RS_Block* block = destination->findBlock(bname);
    if (block == nullptr) {
        const auto blockData = RS_BlockData(bname, {0.0, 0.0}, false);
        block = new RS_Block(destination, blockData);

        for (const RS_Entity* e : *src) {
            if (e == nullptr || e->isDeleted()) {
                continue;
            }
            RS_Entity* clone = e->clone();
            clone->clearDwgProvenance(provenance);
            LC_CopyUtils::doCopyEntityLayer(clone, destination, src);
            block->addEntity(clone);
        }

        destination->addBlock(block);
        // as in LC_CopyUtils::doCopyBlock(): the expansions of its inserts are the library drawing's
        block->updateInserts();
    }

    // fixme - blocks - outer Insert carries all transformations (scale + angle)
    const RS_InsertData idata(bname, data.insertionPoint, scaleV, data.angle, 1, 1, RS_Vector{0.0, 0.0});
    auto* insert = new RS_Insert(nullptr, idata);

    ctx += insert;

    insert->setLayer(insertLayer != nullptr ? insertLayer : destination->getActiveLayer());
    insert->setPen(RS_Pen(RS_Color(RS2::FlagByLayer), RS2::WidthByLayer, RS2::LineByLayer));
    insert->update();

    ctx.dontSetActiveLayerAndPen();

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
    QList<RS_Entity*> transformedOriginals = entitiesList;
    for (const auto e : entitiesList) {
        // Create new entities
        for (int num = 1; num <= numberOfCopies; num++) {
            RS_Entity* clone = getTransformedClone(forPreviewOnly, e, {offsetTx::Step::move(data.offset * num)});
            if (!clone || !acceptedInsertSourceEdit(*clone)) {
                transformedOriginals.removeAll(e);
                delete clone;
                continue;
            }
            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= transformedOriginals;
    }
    ctx.setActiveLayer = data.useCurrentLayer;
    ctx.setActivePen = data.useCurrentAttributes;
    ctx.success = !transformedOriginals.isEmpty();
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

RS_Entity* RS_Modification::getTransformedClone(const bool preview, const RS_Entity* e,
    const LC_OffsetCurveTransform::Steps& steps, const bool forcePlain) {
    if (!e || !steps.valid()) return nullptr;
    if (LC_OffsetCurveTransform::needsStaging(*e)) {
        auto prepared = LC_OffsetCurveTransform::prepare(*e, steps, forcePlain || preview);
        if (!prepared) return nullptr;
        if (prepared.keptPlain && !preview)
            RS_DEBUG->print(RS_Debug::D_WARNING, "Offset definition demoted to a transformed ordinary spline");
        return prepared.entity.release();
    }
    auto clone = std::unique_ptr<RS_Entity>{getClone(preview, e)};
    if (!clone) return nullptr;
    using Kind = LC_OffsetCurveTransform::Step::Kind;
    for (std::size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        switch (step.kind) {
        case Kind::Move: clone->move(step.first); break;
        case Kind::Rotate: clone->rotate(step.first, step.scalar); break;
        case Kind::Scale: clone->scale(step.first, step.second); break;
        case Kind::Mirror: clone->mirror(step.first, step.second); break;
        case Kind::Shear: clone->shear(step.scalar); break;
        case Kind::Reverse: clone->revertDirection(); break;
        }
    }
    return clone.release();
}

bool RS_Modification::alignRef(const LC_AlignRefData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                               LC_DocumentModificationBatch& ctx) {
    constexpr int numberOfCopies = 1; /*data.obtainNumberOfCopies();*/
    const RS_Vector offset = data.offset;
    bool succeeded = false;
    // too slow:
    for (const auto e : entitiesList) {
        // Create new entities
        for (int num = 1; num <= numberOfCopies; num++) {
            offsetTx::Steps steps{offsetTx::Step::rotate(data.rotationCenter, data.rotationAngle)};
            if (data.scale && (!std::isfinite(data.scaleFactor) || LC_LineMath::isMeaningful(data.scaleFactor - 1.0))) {
                steps.append(offsetTx::Step::scale(data.rotationCenter, {data.scaleFactor, data.scaleFactor}));
            }
            steps.append(offsetTx::Step::move(offset * num));
            RS_Entity* clone = getTransformedClone(forPreviewOnly, e, steps);
            if (!clone) continue;
            ctx.replace(e, clone);
            succeeded = true;
        }
    }
    return succeeded;
}

bool LC_OffsetBatchOutcome::anySourceSucceeded() const {
    return std::any_of(sources.begin(), sources.end(),
                       [](const LC_OffsetSourceOutcome& source) { return source.succeeded(); });
}

namespace {
/** How near @p p may be to @p e and lie on it, to rounding: no drawing tolerance. */
double onEntityTolerance(const RS_Entity& e, const RS_Vector& p) {
    const RS_Vector lo = e.getMin();
    const RS_Vector hi = e.getMax();
    const double extent = lo.valid && hi.valid ? lo.distanceTo(hi) : 0.0;
    return 1e-9 * std::max({1.0, p.magnitude(), extent});
}

/**
 * How far from its curve a point @p p on @p source can be and still be on it
 * as the user sees it: an RS_Spline is drawn with chords within a thousandth
 * of its control points' extent (RS_Spline::fillDisplayPoints()), so a point
 * snapped onto its drawing lies that near the curve and gives no side the
 * user chose. An ellipse is drawn by the painter's own ellipse calls, exactly:
 * a point near it is on it only to rounding.
 */
double drawnCurveTolerance(const RS_Entity& source, const RS_Vector& p) {
    if (source.rtti() == RS2::EntityEllipse) {
        return onEntityTolerance(source, p);
    }
    RS_Vector lo = source.getMin();
    RS_Vector hi = source.getMax();
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&source)) {
        for (const RS_Vector& p : spline->getData().controlPoints) {
            lo = RS_Vector::minimum(lo, p);
            hi = RS_Vector::maximum(hi, p);
        }
    }
    return (lo.valid && hi.valid) ? 1e-3 * lo.distanceTo(hi) : 0.0;
}

/**
 * Selected segments of polylines, grouped: each run of consecutive selected
 * segments of one polyline, in the polyline's order and, on a closed one,
 * across its seam. A run is offset as one chain, as its polyline would be,
 * so that its segments stay joined; a lone segment is offset on its own.
 */
struct LC_SegmentRun {
    std::vector<RS_Entity*> members;
    /** The chain offset for a run of two or more segments; null for one. */
    std::unique_ptr<RS_Entity> chain;
};

/** The polyline that holds @p e as one of its segments, or null. */
RS_Polyline* holdingPolyline(const RS_Entity* e) {
    RS_EntityContainer* parent = e != nullptr ? e->getParent() : nullptr;
    return (parent != nullptr && parent->rtti() == RS2::EntityPolyline) ? static_cast<RS_Polyline*>(parent) : nullptr;
}

/** Whether @p a and @p b are the same point, to the tolerance a polyline's vertices meet to. */
bool meet(const RS_Vector& a, const RS_Vector& b) {
    return a.distanceTo(b) <= RS_TOLERANCE * std::max(1.0, a.magnitude());
}

/** Whether segment @p next shares an end with segment @p previous. */
bool touches(const RS_Entity* previous, const RS_Entity* next) {
    for (const RS_Vector& p : {previous->getStartpoint(), previous->getEndpoint()}) {
        if (meet(p, next->getStartpoint()) || meet(p, next->getEndpoint())) {
            return true;
        }
    }
    return false;
}

/**
 * Whether a polyline offsets @p segment with the others: a line or an arc.
 * RS_Polyline::offset() leaves an elliptic segment where it is, so one is
 * offset on its own.
 */
bool chainable(const RS_Entity* segment) {
    return segment->rtti() == RS2::EntityLine || segment->rtti() == RS2::EntityArc;
}

/** A copy of @p e that is neither selected nor highlighted, nor are its children. */
RS_Entity* plainCopy(const RS_Entity& e) {
    RS_Entity* copy = e.clone();
    copy->setSelectionFlag(false);
    copy->setHighlighted(false);
    if (copy->isContainer()) {
        for (RS_Entity* child : *static_cast<RS_EntityContainer*>(copy)) {
            child->setSelectionFlag(false);
            child->setHighlighted(false);
        }
    }
    return copy;
}

/**
 * An open polyline through copies of @p segments, in order, with the layer
 * and pen of @p polyline; a segment stored the other way round is turned.
 */
std::unique_ptr<RS_Entity> chainThrough(const RS_Polyline& polyline, const std::vector<RS_Entity*>& segments) {
    auto chain = std::make_unique<RS_Polyline>(polyline.getParent());
    chain->setLayer(polyline.getLayer(false));
    chain->setPen(polyline.getPen(false));
    RS_Entity* previous = nullptr;
    for (size_t i = 0; i < segments.size(); ++i) {
        RS_Entity* piece = plainCopy(*segments[i]);
        bool turned = false;
        if (previous != nullptr) {
            turned = !meet(previous->getEndpoint(), piece->getStartpoint()) &&
                     meet(previous->getEndpoint(), piece->getEndpoint());
        }
        else if (i + 1 < segments.size()) {
            // the first turns when it starts, rather than ends, where the second one is
            const RS_Entity* next = segments[i + 1];
            const auto atNext = [next](const RS_Vector& p) {
                return meet(p, next->getStartpoint()) || meet(p, next->getEndpoint());
            };
            turned = !atNext(piece->getEndpoint()) && atNext(piece->getStartpoint());
        }
        if (turned) {
            piece->revertDirection();
        }
        piece->setParent(chain.get());
        chain->RS_EntityContainer::addEntity(piece);
        previous = piece;
    }
    chain->setStartpoint(chain->firstEntity()->getStartpoint());
    chain->setEndpoint(previous->getEndpoint());
    chain->calculateBorders();
    return chain;
}

/**
 * The runs of the selected segments among @p sources, by the segments they
 * hold. A polyline that is itself among the sources takes its segments with
 * it, so they are not offset twice. A deleted segment, or a gap between two
 * segments, ends a run; an elliptic segment is a run of its own.
 */
std::vector<std::unique_ptr<LC_SegmentRun>> segmentRuns(const QList<RS_Entity*>& sources,
                                                        std::unordered_map<const RS_Entity*, LC_SegmentRun*>& runOf) {
    std::unordered_set<const RS_Entity*> selected{sources.cbegin(), sources.cend()};
    std::vector<RS_Polyline*> polylines;
    std::unordered_set<const RS_Polyline*> known;
    for (const RS_Entity* e : sources) {
        RS_Polyline* polyline = holdingPolyline(e);
        if (polyline != nullptr && selected.count(polyline) == 0 && known.insert(polyline).second) {
            polylines.push_back(polyline);
        }
    }
    std::vector<std::unique_ptr<LC_SegmentRun>> runs;
    for (RS_Polyline* polyline : polylines) {
        std::vector<RS_Entity*> segments;
        for (RS_Entity* segment : *polyline) {
            segments.push_back(segment);
        }
        const size_t n = segments.size();
        std::vector<bool> chosen(n);
        size_t count = 0;
        for (size_t i = 0; i < n; ++i) {
            chosen[i] = selected.count(segments[i]) != 0 && !segments[i]->isDeleted();
            count += chosen[i] ? 1 : 0;
        }
        if (count == 0) {
            continue;
        }
        // whether a run through segment a goes on into segment b
        const auto continues = [&](const size_t a, const size_t b) {
            return chosen[a] && chosen[b] && chainable(segments[a]) && chainable(segments[b]) &&
                   touches(segments[a], segments[b]);
        };
        // on a closed polyline, start where a run cannot come in from the
        // segment before, so that a run across the seam is one run
        size_t first = 0;
        if (polyline->isClosed()) {
            for (size_t i = 0; i < n; ++i) {
                if (!continues((i + n - 1) % n, i)) {
                    first = i;
                    break;
                }
            }
        }
        std::vector<std::vector<RS_Entity*>> found;
        for (size_t step = 0; step < n; ++step) {
            const size_t i = (first + step) % n;
            if (!chosen[i]) {
                continue;
            }
            if (step == 0 || !continues((i + n - 1) % n, i)) {
                found.emplace_back();
            }
            found.back().push_back(segments[i]);
        }
        for (std::vector<RS_Entity*>& members : found) {
            auto run = std::make_unique<LC_SegmentRun>();
            if (members.size() == n && n > 1) {
                run->chain.reset(plainCopy(*polyline)); // every segment: the polyline, closed if it is
                run->chain->setParent(polyline->getParent());
            }
            else if (members.size() > 1) {
                run->chain = chainThrough(*polyline, members);
            }
            run->members = std::move(members);
            for (const RS_Entity* member : run->members) {
                runOf[member] = run.get();
            }
            runs.push_back(std::move(run));
        }
    }
    return runs;
}

/** total += more, unless a count would overflow. */
bool addUsage(LC_OffsetOutputUsage& total, const LC_OffsetOutputUsage& more) {
    constexpr std::size_t max = std::numeric_limits<std::size_t>::max();
    if (more.cubicPieces > max - total.cubicPieces || more.outputEntities > max - total.outputEntities ||
        more.deepEntities > max - total.deepEntities) {
        return false;
    }
    total.cubicPieces += more.cubicPieces;
    total.outputEntities += more.outputEntities;
    total.deepEntities += more.deepEntities;
    return true;
}

/** What is left of @p budget after @p used; a field exhausted to zero makes it invalid. */
LC_OffsetSourceBudget remainingBudget(const LC_OffsetSourceBudget& budget, const LC_OffsetOutputUsage& used) {
    auto left = [](const std::size_t cap, const std::size_t spent) { return cap > spent ? cap - spent : 0; };
    return {left(budget.maxCubicPieces, used.cubicPieces), left(budget.maxOutputEntities, used.outputEntities),
            left(budget.maxDeepEntities, used.deepEntities)};
}

bool withinBudget(const LC_OffsetOutputUsage& used, const LC_OffsetSourceBudget& budget) {
    return used.cubicPieces <= budget.maxCubicPieces && used.outputEntities <= budget.maxOutputEntities &&
           used.deepEntities <= budget.maxDeepEntities;
}

LC_OffsetSourceStatus engineFailure(const LC_CurveOffsetStatus status) {
    return status == LC_CurveOffsetStatus::LimitExceeded ? LC_OffsetSourceStatus::LimitExceeded
                                                         : LC_OffsetSourceStatus::OffsetFailed;
}

/** Every field of @p a at most that of @p b. */
bool noLargerThan(const LC_OffsetSourceBudget& a, const LC_OffsetSourceBudget& b) {
    return a.maxCubicPieces <= b.maxCubicPieces && a.maxOutputEntities <= b.maxOutputEntities &&
           a.maxDeepEntities <= b.maxDeepEntities;
}

/** Whether @p polyline holds an elliptic segment, as a non-uniform scale makes. */
bool hasEllipticSegment(const RS_Polyline& polyline) {
    return std::any_of(polyline.begin(), polyline.end(),
                       [](const RS_Entity* child) { return child != nullptr && child->rtti() == RS2::EntityEllipse; });
}

/**
 * The band, relative to the larger of a source's extent and the distance,
 * within which an exact offset has nothing left: the curve-offset engine's
 * tolerance and classification margin (lc_curveoffset.cpp, trimBranches()).
 */
constexpr double kOffsetVanishBand = 1.25e-6;

/**
 * Whether @p p lies on @p e, to rounding: on a line's infinite line, on a
 * circle's or an arc's circle, on a polyline's nearest segment. Such a point
 * gives no side. Only the types offset without the curve-offset engine,
 * which judges its own sources, are known here.
 */
bool sideIsAmbiguous(const RS_Entity& e, const RS_Vector& p) {
    if (!p.valid) {
        return true;
    }
    const double onIt = onEntityTolerance(e, p);
    switch (e.rtti()) {
        case RS2::EntityLine: {
            const RS_Vector a = e.getStartpoint();
            const RS_Vector along = e.getEndpoint() - a;
            const RS_Vector off = p - a;
            const double length = std::hypot(along.x, along.y);
            return length > 0.0 ? std::abs(along.x * off.y - along.y * off.x) / length <= onIt
                                : p.distanceTo(a) <= onIt;
        }
        case RS2::EntityCircle:
        case RS2::EntityArc:
            return std::abs(p.distanceTo(e.getCenter()) - e.getRadius()) <= onIt;
        case RS2::EntityPolyline:
            for (const RS_Entity* child : static_cast<const RS_Polyline&>(e)) {
                LC_OffsetSegment segment;
                if (child != nullptr && makeOffsetSegment(*child, segment) &&
                    pointSegmentDistance(p, segment) <= onIt) {
                    return true;
                }
            }
            return false;
        default:
            return false;
    }
}
} // namespace

LC_OffsetBatchLimits LC_OffsetBatchLimits::preview() {
    LC_OffsetBatchLimits limits;
    limits.perSource = {kDefaultOffsetCubicPiecesPerSource / 8, kDefaultOffsetOutputEntitiesPerSource / 8,
                        kDefaultOffsetDeepEntitiesPerSource / 8};
    limits.maxDeepEntitiesPerRequest = kDefaultOffsetDeepEntitiesPerRequest / 8;
    limits.maxSamples = LC_CurveOffset::kDefaultMaxSamples / 8;
    limits.maxIntersectionPairs = LC_CurveOffset::kDefaultMaxIntersectionPairs / 8;
    return limits;
}

struct LC_OffsetPreviewCache::Entry {
    LC_OffsetSourceBudget budget;
    LC_CurveOffsetStatus status;
    LC_OffsetOutputUsage usage;
    LC_OffsetValidationLevel validationLevel;
    double maxObservedError;
    std::vector<std::unique_ptr<RS_Entity>> entities;
};

bool LC_OffsetPreviewSnapshot::admitted() const noexcept {
    if (revision >= std::uint64_t(std::numeric_limits<qint64>::max()) ||
        otherEntries > maximumEntries || entries.size() > maximumEntries - otherEntries) return false;
    const auto hard = makeDefaultOffsetSourceBudget();
    std::size_t values = 8;
    auto take = [&](std::size_t n) {
        if (n > maximumValues - values) return false;
        values += n; return true;
    };
    const auto point = [](const RS_Vector& p) { return LC_EllipseOffsetGeometry::isFinite2D(p); };
    std::optional<std::pair<int, double>> previous;
    for (const auto& e : entries) {
        if ((e.side != LC_CurveOffsetSide::Left && e.side != LC_CurveOffsetSide::Right) ||
            !std::isfinite(e.magnitude) || e.magnitude <= 0 || !take(20) ||
            !noLargerThan(e.budget, hard) || !withinBudget(e.usage, e.budget) ||
            int(e.status) < int(LC_CurveOffsetStatus::Ok) || int(e.status) > int(LC_CurveOffsetStatus::LimitExceeded) ||
            int(e.validation) < 0 || int(e.validation) > int(LC_OffsetValidationLevel::IntervalCertified) ||
            (e.observedError && (!std::isfinite(*e.observedError) || *e.observedError < 0))) return false;
        const auto key = std::make_pair(int(e.side), e.magnitude);
        if (previous && !(*previous < key)) return false;
        previous = key;
        if (e.geometry.size() > e.budget.maxOutputEntities ||
            (e.status == LC_CurveOffsetStatus::Ok ? e.geometry.size() != e.usage.outputEntities : !e.geometry.empty()))
            return false;
        for (const auto& value : e.geometry) {
            if (const auto* line = std::get_if<RS_LineData>(&value)) {
                if (!point(line->startpoint) || !point(line->endpoint) || !take(13)) return false;
            } else {
                const auto& d = std::get<RS_SplineData>(value);
                const auto controls = d.controlPoints.size();
                if ((d.degree != 1 && d.degree != 3) || controls < d.degree + 1 || controls > 16384 ||
                    (controls - 1) % d.degree || d.type != RS_SplineData::SplineType::ClampedOpen ||
                    d.savedOpenType != d.type || !d.fitPoints.empty() || !d.savedOpenKnots.empty() ||
                    d.weights.size() != controls || d.knotslist.size() != controls + d.degree + 1 ||
                    d.knotslist.size() > 32768 || !take(13 + 6 * controls + d.knotslist.size())) return false;
                for (const auto& p : d.controlPoints) if (!point(p)) return false;
                for (double w : d.weights) if (w != 1) return false;
                std::size_t index = 0;
                double last = 0;
                const auto spans = (controls - 1) / d.degree;
                if (spans > e.budget.maxCubicPieces) return false;
                for (std::size_t k = 0; k <= spans; ++k) {
                    const double knot = d.knotslist[index];
                    if (!std::isfinite(knot) || (k && !(last < knot))) return false;
                    const auto repeats = k == 0 || k == spans ? d.degree + 1 : d.degree;
                    for (std::size_t j = 0; j < repeats; ++j) if (d.knotslist[index++] != knot) return false;
                    last = knot;
                }
                const auto display = RS_Spline::displayEntityUpperBound(d);
                if (!display || *display > e.budget.maxDeepEntities) return false;
            }
        }
    }
    return true;
}

LC_OffsetPreviewCache::LC_OffsetPreviewCache() = default;
LC_OffsetPreviewCache::~LC_OffsetPreviewCache() = default;

bool LC_OffsetPreviewCache::find(const RS_Entity* source, const LC_CurveOffsetSide side, const double magnitude,
                                 const LC_OffsetSourceBudget& budget, LC_CurveOffsetMaterializationResult& out) const {
    const auto it = m_entries.find({source, static_cast<int>(side), magnitude});
    if (it == m_entries.end()) {
        return false;
    }
    const Entry& entry = *it->second;
    bool reusable = true; // a refusal on the geometry holds under any budget
    if (entry.status == LC_CurveOffsetStatus::Ok) {
        reusable = withinBudget(entry.usage, budget);
    }
    else if (entry.status == LC_CurveOffsetStatus::LimitExceeded) {
        reusable = noLargerThan(budget, entry.budget);
    }
    if (!reusable) {
        return false;
    }
    out = LC_CurveOffsetMaterializationResult{};
    out.status = entry.status;
    out.usage = entry.usage;
    out.validationLevel = entry.validationLevel;
    out.maxObservedError = entry.maxObservedError;
    for (const std::unique_ptr<RS_Entity>& entity : entry.entities) {
        out.entities.emplace_back(entity->clone());
    }
    return true;
}

void LC_OffsetPreviewCache::keep(const RS_Entity* source, const LC_CurveOffsetSide side, const double magnitude,
                                 const LC_OffsetSourceBudget& budget, const LC_CurveOffsetMaterializationResult& result) {
    if (m_revision >= std::uint64_t(std::numeric_limits<qint64>::max()) - 1)
        throw std::overflow_error("Offset preview cache revision exhausted");
    auto entry = std::make_unique<Entry>(
        Entry{budget, result.status, result.usage, result.validationLevel, result.maxObservedError, {}});
    for (const std::unique_ptr<RS_Entity>& entity : result.entities) {
        entry->entities.emplace_back(entity->clone());
    }
    // Allocate the map node before clearing/overwriting existing state.
    decltype(m_entries) staged;
    const auto key = std::make_tuple(source, static_cast<int>(side), magnitude);
    staged.emplace(key, std::move(entry));
    if (m_entries.size() + m_otherEntries >= LC_OffsetPreviewSnapshot::maximumEntries) {
        m_entries.clear(); m_otherEntries = 0; m_cleared = true;
    }
    m_entries.erase(key);
    m_entries.insert(staged.extract(staged.begin()));
    ++m_revision;
}

void LC_OffsetPreviewCache::clear() {
    if (m_revision >= std::uint64_t(std::numeric_limits<qint64>::max()) - 1)
        throw std::overflow_error("Offset preview cache revision exhausted");
    m_entries.clear(); m_otherEntries = 0; m_cleared = true; ++m_revision;
}

std::unique_ptr<LC_OffsetPreviewSnapshot> LC_OffsetPreviewCache::snapshot(const RS_Entity* source) const {
    auto out = std::make_unique<LC_OffsetPreviewSnapshot>();
    out->revision = m_revision; out->cleared = m_cleared; out->otherEntries = m_otherEntries;
    std::size_t values = 8;
    const auto take = [&](std::size_t n) {
        if (n > LC_OffsetPreviewSnapshot::maximumValues - values) return false;
        values += n; return true;
    };
    for (const auto& item : m_entries) {
        if (std::get<0>(item.first) != source) { ++out->otherEntries; continue; }
        const auto& entry = *item.second;
        if (!take(20)) return {};
        LC_OffsetPreviewSnapshot::Entry value;
        value.side = LC_CurveOffsetSide(std::get<1>(item.first)); value.magnitude = std::get<2>(item.first);
        value.budget = entry.budget; value.status = entry.status; value.usage = entry.usage;
        value.validation = entry.validationLevel;
        if (!std::isnan(entry.maxObservedError)) value.observedError = entry.maxObservedError;
        for (const auto& entity : entry.entities) {
            if (entity->rtti() == RS2::EntityLine) {
                if (!take(13)) return {};
                value.geometry.emplace_back(static_cast<const RS_Line&>(*entity).getData());
            } else if (entity->rtti() == RS2::EntitySpline) {
                const auto& d = static_cast<const RS_Spline&>(*entity).getData();
                if (d.controlPoints.size() > 16384 || d.knotslist.size() > 32768 ||
                    d.weights.size() != d.controlPoints.size() || !d.fitPoints.empty() || !d.savedOpenKnots.empty() ||
                    !take(13 + 6 * d.controlPoints.size() + d.knotslist.size())) return {};
                value.geometry.emplace_back(d);
            } else return {}; // Never transport an editable native cache as authority.
        }
        out->entries.push_back(std::move(value));
    }
    return out->admitted() ? std::move(out) : nullptr;
}

bool LC_OffsetPreviewCache::restore(const RS_Entity* source, const LC_OffsetPreviewSnapshot& snapshot) {
    if (!snapshot.admitted()) return false;
    LC_OffsetPreviewCache staged;
    for (const auto& value : snapshot.entries) {
        auto entry = std::make_unique<Entry>(Entry{value.budget, value.status, value.usage,
            value.validation, value.observedError.value_or(std::numeric_limits<double>::quiet_NaN()), {}});
        for (const auto& geometry : value.geometry) {
            if (const auto* line = std::get_if<RS_LineData>(&geometry))
                entry->entities.push_back(std::make_unique<RS_Line>(nullptr, *line));
            else {
                const auto& data = std::get<RS_SplineData>(geometry);
                auto spline = std::make_unique<RS_Spline>(nullptr, data);
                if (!spline->validate() || RS_Spline::geometryFingerprint(data) !=
                    RS_Spline::geometryFingerprint(spline->getData())) return false;
                entry->entities.push_back(std::move(spline));
            }
        }
        staged.m_entries.emplace(std::make_tuple(source, int(value.side), value.magnitude), std::move(entry));
    }
    m_entries.swap(staged.m_entries); m_otherEntries = snapshot.otherEntries;
    m_revision = snapshot.revision; m_cleared = false;
    return true;
}

bool LC_OffsetPreviewCache::merge(const RS_Entity* source, const LC_OffsetPreviewSnapshot& before,
    const LC_OffsetPreviewSnapshot& after) {
    if (m_otherEntries || before.revision != m_revision || !before.admitted() || !after.admitted() ||
        after.revision < before.revision || after.revision - before.revision > RS_OffsetData::kMaximumOffsetCopies ||
        after.otherEntries != (after.cleared ? 0 : before.otherEntries)) return false;
    LC_OffsetPreviewCache staged;
    if (!staged.restore(source, after)) return false;
    if (!after.cleared) {
        std::size_t other = 0;
        for (const auto& item : m_entries) if (std::get<0>(item.first) != source) {
            staged.m_entries.emplace(item.first, nullptr); ++other;
        }
        if (other != before.otherEntries) return false;
        for (auto& item : m_entries) if (std::get<0>(item.first) != source)
            staged.m_entries.find(item.first)->second = std::move(item.second);
    }
    m_entries.swap(staged.m_entries); m_revision = after.revision;
    m_cleared |= after.cleared;
    return true;
}

/**
 * Offset all selected entities with the given mouse position and distance
 *
 *@Author: Dongxu Li
 */
bool RS_Modification::offset(const RS_OffsetData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                             LC_DocumentModificationBatch& ctx) {
    return offsetWithOutcome(data, entitiesList, forPreviewOnly, LC_OffsetBatchLimits{}, ctx).anySourceSucceeded();
}

LC_OffsetSourceStatus RS_Modification::prepareOffsetSource(const RS_OffsetData& data, RS_Entity& e,
    const bool forPreviewOnly, const LC_OffsetBatchLimits& limits, const std::size_t requestDeepLeft,
    const LC_OffsetSourceContext sourceContext, std::vector<std::unique_ptr<RS_Entity>>& roots,
    LC_OffsetSourceOutcome& result, LC_OffsetPreviewCache* const cache) {
    const int numberOfCopies = data.obtainNumberOfCopies();
    result.copiesRequested = numberOfCopies;
    const auto discard = [&]() noexcept {
        roots.clear(); result.copiesMade = result.copiesKeptPlain = 0;
        result.copyOutputCounts.clear(); result.keptPlain.clear(); result.usage = {};
    };
    try {
        const auto status = prepareOffsetSourceImpl(data, e, forPreviewOnly, limits, requestDeepLeft, roots, result,
            cache, &sourceContext, numberOfCopies);
        if (status != LC_OffsetSourceStatus::Succeeded) discard();
        return status;
    } catch (...) {
        discard();
        throw;
    }
}

LC_OffsetSourceStatus RS_Modification::prepareOffsetSourceImpl(const RS_OffsetData& data, RS_Entity& e,
    const bool forPreviewOnly, const LC_OffsetBatchLimits& limits, const std::size_t requestDeepLeft,
    std::vector<std::unique_ptr<RS_Entity>>& roots, LC_OffsetSourceOutcome& result,
    LC_OffsetPreviewCache* const cache, const LC_OffsetSourceContext* const sourceContext, const int numberOfCopies) {
    // All copies stay provisional until the caller accepts the source as a whole.
    LC_OffsetOutputUsage& usage = result.usage;
    LC_OffsetSourceBudget budget = limits.perSource;
    budget.maxDeepEntities = std::min(budget.maxDeepEntities, requestDeepLeft);
    auto measureAll = [&roots, &budget]() {
        std::vector<const RS_Entity*> all;
        for (const std::unique_ptr<RS_Entity>& root : roots) {
            all.push_back(root.get());
        }
        return measureOffsetOutput(all, budget.maxDeepEntities);
    };
    // whether a copy that cannot be made ends the series rather than fails the source
    auto stopsSeries = [&result]() {
        if (result.copiesMade == 0) {
            return false;
        }
        result.stoppedBy = LC_OffsetSourceStatus::OffsetFailed;
        return true;
    };
    const auto adoptProduced = [&](LC_CurveOffsetMaterializationResult& copy, const int ordinal,
                                   const double magnitude, const LC_CurveOffsetSide side,
                                   const LC_OffsetSourceBudget& left,
                                   const LC_OffsetPlainReason nativeFallback,
                                   const std::optional<LC_OffsetCheckResult::Reason> nativeCheck) {
        if (!data.asOffsetCurve || forPreviewOnly ||
            (e.rtti() != RS2::EntityEllipse && e.rtti() != RS2::EntityOffsetCurve &&
                !(sourceContext && sourceContext->nativeSource))) return;
        const auto kept = [&](const LC_OffsetPlainReason reason,
                              std::optional<LC_OffsetCheckResult::Reason> checked = std::nullopt) {
            result.keptPlain.push_back({ordinal, reason, checked});
            ++result.copiesKeptPlain;
        };
        for (auto& entity : copy.entities) {
            entity->RS_Entity::operator=(e);
            entity->setLayer(e.getLayer(false));
            entity->clearDwgProvenance(RS_Entity::Identity);
            entity->setLineage(RS_Entity::newLineage()); entity->setOwnerLineage(0);
            entity->delFlag(RS2::FlagHatchChild);
            entity->calculateBorders();
        }
        if (sourceContext ? !sourceContext->topLevel :
            (!e.getParent() || !e.getParent()->isDocument() || e.isDeleted() ||
                e.getParent()->findEntityIndex(&e) < 0)) {
            kept(LC_OffsetPlainReason::NotTopLevel); return;
        }
        if (copy.entities.size() != 1) { kept(LC_OffsetPlainReason::SeveralPieces); return; }
        const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&e);
        if (!ellipse) { kept(nativeFallback, nativeCheck); return; }
        // A one-leaf LINE expands to a native spline's display children.
        if (left.maxDeepEntities < kOffsetDeepEntitiesPerOutputSpline) {
            kept(LC_OffsetPlainReason::OutputBudget); return;
        }
        LC_EllipseOffsetDefinition definition;
        definition.basis = ellipse->getData(); definition.distance = magnitude;
        definition.towardCentre = (side == LC_CurveOffsetSide::Left) != definition.basis.reversed;
        definition.relTolerance = LC_CurveOffset::kEllipseRelativeOffsetTolerance;
        const double tolerance = LC_CurveOffset::toleranceFor(e, magnitude, definition.relTolerance);
        LC_OffsetCheckResult checked;
        LC_OffsetCurveRefusal refusal;
        if (result.adoptionAttempts == std::size_t(RS_OffsetData::kMaximumOffsetCopies))
            throw std::overflow_error("Offset adoption attempt count exceeded");
        ++result.adoptionAttempts;
        auto native = LC_OffsetCurve::adopt(*copy.entities.front(), definition, tolerance, &checked, {}, &refusal);
        auto total = result.adoptionCheckWork;
        const LC_OffsetCheckLimits hard;
        const auto add = [&](std::size_t& value, std::size_t more, std::size_t perCheck) {
            const auto cap = result.adoptionAttempts * perCheck;
            if (value > cap || more > cap - value)
                throw std::overflow_error("Offset adoption work exceeded");
            value += more;
        };
        add(total.cacheSpans, checked.work.cacheSpans, hard.maxCacheSpans);
        add(total.vertices, checked.work.vertices, hard.maxVertices);
        add(total.distancePairs, checked.work.distancePairs, hard.maxDistancePairs);
        add(total.orderingComparisons, checked.work.orderingComparisons, hard.maxOrderingComparisons);
        if (checked.work.deepest > hard.maxDepth)
            throw std::overflow_error("Offset adoption depth exceeded");
        total.deepest = std::max(total.deepest, checked.work.deepest);
        result.adoptionCheckWork = total;
        if (!native) {
            const auto reason = refusal == LC_OffsetCurveRefusal::CheckFailed || checked ?
                std::optional<LC_OffsetCheckResult::Reason>{checked.reason} : std::nullopt;
            kept(refusal == LC_OffsetCurveRefusal::CheckFailed ? LC_OffsetPlainReason::CheckFailed :
                 LC_OffsetPlainReason::NotCacheRepresentable, reason);
            return;
        }
        const auto cost = measureOffsetOutput({native.get()}, left.maxDeepEntities);
        if (cost.status != LC_OffsetTreeStatus::Ok) {
            kept(LC_OffsetPlainReason::OutputBudget, checked.reason); return;
        }
        copy.usage.deepEntities = cost.deepEntities;
        copy.entities.front() = std::move(native);
    };

    // causes known before anything is offset
    if (e.rtti() == RS2::EntityLine && (e.getEndpoint() - e.getStartpoint()).magnitude() < RS_TOLERANCE) {
        result.detail = LC_OffsetFailureDetail::ZeroLength;
        return LC_OffsetSourceStatus::OffsetFailed;
    }
    if (const auto* hyperbola = dynamic_cast<const LC_Hyperbola*>(&e);
        hyperbola != nullptr && hyperbola->isInfinite()) {
        result.detail = LC_OffsetFailureDetail::Unbounded;
        return LC_OffsetSourceStatus::OffsetFailed;
    }

    if (LC_CurveOffset::isSupportedSource(e)) {
        // The side is resolved once, so no copy can land on another side.
        const LC_CurveOffsetOptions sideOptions = LC_CurveOffset::makeOffsetOptions(e, std::abs(data.distance));
        LC_OffsetSideResolution side = LC_CurveOffset::resolveSide(e, data.coord, sideOptions);
        // a point on the curve, or on the segments it is drawn with, gives no
        // side the user chose: the fallback point decides, when it can
        const bool onCurve =
            side.status == LC_CurveOffsetStatus::AmbiguousSide ||
            (side.status == LC_CurveOffsetStatus::Ok && side.distance <= drawnCurveTolerance(e, data.coord));
        if (onCurve && data.sideFallback.valid) {
            const LC_OffsetSideResolution fallback = LC_CurveOffset::resolveSide(e, data.sideFallback, sideOptions);
            if (fallback.status == LC_CurveOffsetStatus::Ok) {
                side = fallback;
            }
        }
        if (side.status != LC_CurveOffsetStatus::Ok) {
            result.engineStatus = side.status;
            return engineFailure(side.status);
        }
        result.resolvedSide = side.side;
        for (int num = 1; num <= numberOfCopies; ++num) {
            const LC_OffsetSourceBudget left = remainingBudget(budget, usage);
            if (!isValidOffsetBudget(left)) {
                return LC_OffsetSourceStatus::LimitExceeded;
            }
            const double magnitude = std::abs(num * data.distance);
            LC_CurveOffsetMaterializationResult copy;
            auto options = LC_CurveOffset::makeOffsetOptions(e, magnitude);
            if (limits.maxSamples) options.maxSamples = std::min(options.maxSamples, limits.maxSamples);
            if (limits.maxIntersectionPairs)
                options.maxIntersectionPairs = std::min(options.maxIntersectionPairs, limits.maxIntersectionPairs);
            std::size_t samplesLeft = options.maxSamples, pairsLeft = options.maxIntersectionPairs;
            const auto* nativeSource = sourceContext ? nullptr : dynamic_cast<const LC_OffsetCurve*>(&e);
            const bool nativeRequested = data.asOffsetCurve && !forPreviewOnly &&
                (sourceContext ? sourceContext->nativeSource : nativeSource != nullptr);
            bool rebased = false;
            auto plainReason = LC_OffsetPlainReason::NotCacheRepresentable;
            std::optional<LC_OffsetCheckResult::Reason> nativeCheck;
            if (nativeRequested) {
                LC_OffsetRebaseOutcome attempt; attempt.copy = num;
                plainReason = LC_OffsetPlainReason::RebaseRefused;
                const auto* parent = sourceContext ? nullptr : e.getParent();
                if (sourceContext ? !sourceContext->topLevel :
                    (!parent || !parent->isDocument() || e.isDeleted() || parent->findEntityIndex(&e) < 0)) {
                    plainReason = LC_OffsetPlainReason::NotTopLevel;
                } else if (sourceContext ? !sourceContext->nativeDefinition.has_value() :
                    !nativeSource->hasEditableDefinition()) {
                    plainReason = LC_OffsetPlainReason::UndefinedDefinition;
                } else {
                    const auto& savedDefinition = sourceContext ? *sourceContext->nativeDefinition : nativeSource->definition();
                    const auto proposal = LC_OffsetCurveAgain::propose(savedDefinition, side.side, magnitude);
                    attempt.proposalRefusal = proposal.refusal;
                    if (!proposal) {
                        if (proposal.refusal == LC_OffsetCurveAgain::Refusal::ZeroSum)
                            plainReason = LC_OffsetPlainReason::ZeroSum;
                    } else if (samplesLeft && pairsLeft >= 2) {
                        const auto& definition = proposal.proposal->definition;
                        RS_Ellipse source{nullptr, definition.basis};
                        source.RS_Entity::operator=(e);
                        LC_OffsetRunLimits run; run.outputBudget = left; run.maxSamples = samplesLeft;
                        // Reserve per-query ceilings, not observed total boxes.
                        // Each attempt can run several independently bounded queries.
                        run.maxIntersectionPairs = pairsLeft / 2;
                        auto made = LC_OffsetCurve::create(source, definition.distance, definition.towardCentre,
                            run, definition.relTolerance, definition.cacheReversed);
                        attempt.engineRuns = made.engineRuns; attempt.sampleUnits = made.sampleUnits;
                        attempt.nativeRefusal = made.refusal; attempt.engineStatus = made.engineStatus;
                        attempt.checkReason = made.checkReason; attempt.checkWork = made.checkWork;
                        nativeCheck = made.checkReason;
                        result.rebaseEngineRuns += made.engineRuns; result.sampleUnits += made.sampleUnits;
                        if (made.engineRuns) {
                            attempt.intersectionPairAllowance = run.maxIntersectionPairs;
                            result.intersectionPairAllowance += run.maxIntersectionPairs;
                            pairsLeft -= run.maxIntersectionPairs;
                            samplesLeft -= std::min(samplesLeft, made.sampleUnits);
                        }
                        if (!addUsage(result.attemptedOutputUsage, made.attemptedOutputUsage))
                            return LC_OffsetSourceStatus::LimitExceeded;
                        result.attemptedDeepCountComplete &= made.attemptedDeepCountComplete;
                        if (made) {
                            copy.status = LC_CurveOffsetStatus::Ok; copy.usage = made.outputUsage;
                            copy.entities.emplace_back(std::move(made.entity));
                            rebased = attempt.adopted = true;
                        }
                    }
                }
                result.rebases.push_back(std::move(attempt));
            }
            if (!rebased) {
                bool reused = cache && cache->find(&e, side.side, magnitude, left, copy);
                // A preview refusal carries no sample/pair allowance provenance.
                // Native retries may reuse geometry, never an effort-based refusal.
                if (reused && nativeRequested && copy.status != LC_CurveOffsetStatus::Ok) reused = false;
                if (reused) { ++result.reusedCacheCopies; result.reusedCopies.push_back({num, left}); }
                else {
                    copy = LC_CurveOffsetMaterializationResult{};
                    if (!samplesLeft || !pairsLeft) copy.status = LC_CurveOffsetStatus::LimitExceeded;
                    else {
                        options.maxSamples = samplesLeft; options.maxIntersectionPairs = pairsLeft;
                        ++result.cacheEngineRuns;
                        copy = LC_CurveOffset::createEntities(e, LC_CurveOffset::makeSideRequest(side.side, magnitude),
                                                              options, left);
                        result.sampleUnits += copy.sampleUnits;
                        result.intersectionPairAllowance += pairsLeft;
                        if (!addUsage(result.attemptedOutputUsage, copy.attemptedOutputUsage))
                            return LC_OffsetSourceStatus::LimitExceeded;
                        result.attemptedDeepCountComplete &= copy.attemptedDeepCountComplete;
                        if (cache && !nativeRequested) cache->keep(&e, side.side, magnitude, left, copy);
                    }
                }
            }
            if (copy.status != LC_CurveOffsetStatus::Ok) {
                result.engineStatus = copy.status;
                if (copy.status != LC_CurveOffsetStatus::LimitExceeded && stopsSeries()) {
                    break;
                }
                return engineFailure(copy.status);
            }
            if (copy.entities.empty()) {
                result.stoppedBy = LC_OffsetSourceStatus::Vanished;
                break; // trimmed away entirely, as it is at any larger distance on this side
            }
            if (!rebased) adoptProduced(copy, num, magnitude, side.side, left, plainReason, nativeCheck);
            else for (auto& entity : copy.entities) {
                entity->RS_Entity::operator=(e); entity->setLayer(e.getLayer(false));
                entity->clearDwgProvenance(RS_Entity::Identity);
                entity->setLineage(RS_Entity::newLineage()); entity->setOwnerLineage(0);
                entity->delFlag(RS2::FlagHatchChild); entity->calculateBorders();
            }
            if (!addUsage(usage, copy.usage)) {
                return LC_OffsetSourceStatus::LimitExceeded;
            }
            for (std::unique_ptr<RS_Entity>& entity : copy.entities) {
                roots.push_back(std::move(entity));
            }
            result.copyOutputCounts.push_back(copy.entities.size());
            ++result.copiesMade;
        }
    }
    else {
        // RS_Polyline::offset() moves lines and arcs only: an elliptic
        // segment would stay where it is, between offset neighbours.
        const auto* const polyline = e.rtti() == RS2::EntityPolyline ? static_cast<const RS_Polyline*>(&e) : nullptr;
        if (polyline != nullptr && hasEllipticSegment(*polyline)) {
            result.detail = LC_OffsetFailureDetail::EllipticSegments;
            return LC_OffsetSourceStatus::OffsetFailed;
        }
        // A point on the entity gives no side the user chose, and each
        // type would settle it its own way (a line to the left, a circle
        // inwards): the second click decides, and without one, nothing.
        RS_Vector coord = data.coord;
        if (sideIsAmbiguous(e, coord)) {
            if (!data.sideFallback.valid || sideIsAmbiguous(e, data.sideFallback)) {
                result.engineStatus = LC_CurveOffsetStatus::AmbiguousSide;
                result.detail = LC_OffsetFailureDetail::AmbiguousSide;
                return LC_OffsetSourceStatus::OffsetFailed;
            }
            coord = data.sideFallback;
        }
        // Inwards, a circle or an arc has nothing left once its radius is
        // within the band every exact offset vanishes in, relative to its size.
        const bool circular = e.rtti() == RS2::EntityCircle || e.rtti() == RS2::EntityArc;
        const bool inwards = circular && coord.distanceTo(e.getCenter()) < e.getRadius();
        const double extent = e.getMin().valid && e.getMax().valid ? e.getMin().distanceTo(e.getMax()) : 0.0;
        for (int num = 1; num <= numberOfCopies; ++num) {
            if (!isValidOffsetBudget(remainingBudget(budget, usage))) {
                return LC_OffsetSourceStatus::LimitExceeded;
            }
            const double magnitude = std::abs(num * data.distance);
            if (inwards && e.getRadius() - magnitude <= kOffsetVanishBand * std::max(extent, magnitude)) {
                result.stoppedBy = LC_OffsetSourceStatus::Vanished;
                break; // as it is at any larger distance inwards
            }
            std::vector<std::unique_ptr<RS_Entity>> copy;
            // First try the type-changing path (e.g. ellipse → spline).
            for (RS_Entity* off : e.createOffset(coord, magnitude)) {
                copy.emplace_back(off);
            }
            if (copy.empty()) {
                // Fall back to the in-place clone+offset path.
                std::unique_ptr<RS_Entity> clone{getClone(forPreviewOnly, &e)};
                if (!clone->offset(coord, magnitude)) {
                    if (e.rtti() == RS2::EntityCircle || e.rtti() == RS2::EntityArc) {
                        result.stoppedBy = LC_OffsetSourceStatus::Vanished;
                        break; // the radius would vanish, as it does at any larger distance inwards
                    }
                    if (stopsSeries()) {
                        break;
                    }
                    return LC_OffsetSourceStatus::OffsetFailed;
                }
                if (polyline != nullptr) {
                    // RS_Polyline::offset() joins neighbouring offsets and
                    // never trims a loop, and it always reports success:
                    // what it made is used only if it is a trimmed offset
                    const LC_OffsetCheckReport check =
                        checkLegacyPolylineOffset(*polyline, static_cast<const RS_Polyline&>(*clone), magnitude);
                    if (check.verdict == LC_OffsetCheckVerdict::NothingLeft) {
                        result.stoppedBy = LC_OffsetSourceStatus::Vanished;
                        break; // shrunk past its size, as it is at any larger distance
                    }
                    if (check.verdict == LC_OffsetCheckVerdict::Invalid) {
                        result.detail = LC_OffsetFailureDetail::PolylineNotTrimmed;
                        if (stopsSeries()) {
                            break;
                        }
                        return LC_OffsetSourceStatus::OffsetFailed;
                    }
                }
                copy.push_back(std::move(clone));
            }
            std::vector<const RS_Entity*> copyRoots;
            for (const std::unique_ptr<RS_Entity>& entity : copy) {
                copyRoots.push_back(entity.get());
            }
            // against what the earlier copies left, as for splines
            const LC_OffsetTreeCost cost =
                measureOffsetOutput(copyRoots, remainingBudget(budget, usage).maxDeepEntities);
            if (cost.status != LC_OffsetTreeStatus::Ok ||
                !addUsage(usage, LC_OffsetOutputUsage{0, copy.size(), cost.deepEntities}) ||
                !withinBudget(usage, budget)) {
                return LC_OffsetSourceStatus::LimitExceeded;
            }
            for (std::unique_ptr<RS_Entity>& entity : copy) {
                roots.push_back(std::move(entity));
            }
            result.copyOutputCounts.push_back(copy.size());
            ++result.copiesMade;
        }
    }
    if (result.copiesMade == 0) {
        return LC_OffsetSourceStatus::Vanished;
    }
    // the whole provisional set: within the source's limits, and no root
    // or child shared between copies
    const LC_OffsetTreeCost all = measureAll();
    if (all.status != LC_OffsetTreeStatus::Ok || !withinBudget(usage, budget)) {
        return LC_OffsetSourceStatus::LimitExceeded;
    }
    usage.deepEntities = all.deepEntities;
    return LC_OffsetSourceStatus::Succeeded;
}

struct LC_OffsetBatchPreparation::Data {
    struct Row {
        LC_OffsetSourceOutcome outcome;
        std::vector<std::unique_ptr<RS_Entity>> roots;
        std::size_t remaining = 0;
    };
    RS_OffsetData data;
    QList<RS_Entity*> sources;
    bool preview = false, ready = false, released = false;
    LC_OffsetBatchLimits limits;
    std::map<const RS_Entity*, Row> rows;
    RS_Entity* pending = nullptr;
    std::size_t remaining = 0;
    LC_OffsetPreviewCache* pendingCache = nullptr;
    std::unique_ptr<LC_OffsetWorkerRequest::Request> expected;
    LC_OffsetBatchOutcome outcome;
    std::vector<RS_Entity*> finalAdded;
};

LC_OffsetBatchPreparation::LC_OffsetBatchPreparation(const RS_OffsetData& data,
    const QList<RS_Entity*>& sources, bool preview, const LC_OffsetBatchLimits& limits)
    : m_data(std::make_unique<Data>()) {
    if (sources.size() > maximumSources) throw std::length_error("Offset source inventory exceeded");
    m_data->data = data; m_data->sources = sources; m_data->preview = preview; m_data->limits = limits;
}
LC_OffsetBatchPreparation::~LC_OffsetBatchPreparation() = default;

bool LC_OffsetBatchPreparation::advance(LC_DocumentModificationBatch& batch, LC_OffsetPreviewCache* cache) {
    auto& d = *m_data;
    if (d.released) throw std::logic_error("Offset staging already published");
    d.pending = nullptr; d.pendingCache = nullptr; d.expected.reset(); d.ready = false;
    batch = {};
    d.outcome = RS_Modification::offsetWithOutcomeImpl(d.data, d.sources, d.preview, d.limits, batch, cache, this);
    if (d.pending) { batch = {}; return false; }
    d.finalAdded.assign(batch.entitiesToAdd.begin(), batch.entitiesToAdd.end());
    std::sort(d.finalAdded.begin(), d.finalAdded.end(), std::less<RS_Entity*>{});
    if (std::adjacent_find(d.finalAdded.begin(), d.finalAdded.end()) != d.finalAdded.end())
        throw std::logic_error("Offset staging shares an output root");
    d.ready = true;
    return true;
}
const RS_Entity* LC_OffsetBatchPreparation::pendingSource() const noexcept { return m_data->pending; }
const LC_OffsetBatchOutcome& LC_OffsetBatchPreparation::outcome() const noexcept { return m_data->outcome; }

std::unique_ptr<LC_OffsetWorkerRequest::Request> LC_OffsetBatchPreparation::request(
    const LC_OffsetWorkerRequest::Token& token, LC_OffsetPreviewCache* cache) {
    namespace req = LC_OffsetWorkerRequest;
    auto& d = *m_data;
    if (!d.pending || d.released) return {};
    auto& source = *d.pending;
    req::Produce op;
    if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&source)) op.source = ellipse->getData();
    else if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(&source)) {
        const auto& raw = native->getData();
        if (raw.controlPoints.size() > 16384 || raw.knotslist.size() > 32768 || raw.weights.size() > 16384 ||
            raw.fitPoints.size() > 4096 || raw.savedOpenKnots.size() > 32768) return {};
        op.source = raw; op.context.nativeSource = true;
        if (native->hasEditableDefinition()) op.context.nativeDefinition = native->definition();
    } else return {};
    auto* parent = source.getParent();
    op.context.topLevel = parent && parent->isDocument() && !source.isDeleted() && parent->findEntityIndex(&source) >= 0;
    if (source.getMin().valid) op.minimum = source.getMin();
    if (source.getMax().valid) op.maximum = source.getMax();
    op.coordinate = d.data.coord;
    if (d.data.sideFallback.valid) op.fallback = d.data.sideFallback;
    op.distance = d.data.distance; op.copies = d.data.obtainNumberOfCopies();
    op.asOffsetCurve = d.data.asOffsetCurve; op.preview = d.preview; op.limits = d.limits;
    op.limits.maxDeepEntitiesPerRequest = d.remaining;
    auto value = std::make_unique<req::Request>(); value->token = token;
    if (cache) {
        auto snapshot = cache->snapshot(d.pending);
        if (!snapshot) return {};
        value->operation = req::CachedProduce{std::move(op), std::move(*snapshot)};
    } else value->operation = std::move(op);
    if (!req::admitted(*value)) return {};
    d.expected = std::make_unique<req::Request>(*value); d.pendingCache = cache;
    return value;
}

bool LC_OffsetBatchPreparation::accept(const LC_OffsetWorkerCompletion& completion) {
    namespace req = LC_OffsetWorkerRequest;
    auto& d = *m_data;
    if (!d.pending || !d.expected || d.released || !completion.isJoined()) return false;
    if (req::encode(*d.expected) != req::encode(completion.request())) return false;
    auto prepared = completion.produced(*d.pending);
    if (!prepared) return false;
    Data::Row row; row.outcome = std::move(prepared->outcome); row.roots = std::move(prepared->entities);
    row.outcome.source = d.pending; row.remaining = d.remaining;
    decltype(d.rows) staged; staged.emplace(d.pending, std::move(row));
    if (d.pendingCache && !completion.updatePreviewCache(*d.pendingCache, d.pending)) return false;
    d.rows.insert(staged.extract(staged.begin()));
    d.pending = nullptr; d.expected.reset(); d.pendingCache = nullptr;
    return true;
}

void LC_OffsetBatchPreparation::release() noexcept {
    auto& d = *m_data;
    if (!d.ready || d.released) return;
    for (auto& row : d.rows) for (auto& root : row.second.roots)
        if (std::binary_search(d.finalAdded.begin(), d.finalAdded.end(), root.get(), std::less<RS_Entity*>{})) root.release();
    d.released = true;
}

LC_OffsetBatchOutcome RS_Modification::offsetWithOutcome(const RS_OffsetData& data, const QList<RS_Entity*>& entitiesList,
                                                         const bool forPreviewOnly, const LC_OffsetBatchLimits& limits,
                                                         LC_DocumentModificationBatch& ctx,
                                                         LC_OffsetPreviewCache* const cache) {
    return offsetWithOutcomeImpl(data, entitiesList, forPreviewOnly, limits, ctx, cache, nullptr);
}

LC_OffsetBatchOutcome RS_Modification::offsetWithOutcomeImpl(const RS_OffsetData& data, const QList<RS_Entity*>& entitiesList,
    const bool forPreviewOnly, const LC_OffsetBatchLimits& limits, LC_DocumentModificationBatch& ctx,
    LC_OffsetPreviewCache* const cache, LC_OffsetBatchPreparation* preparation) {
    LC_OffsetBatchOutcome outcome;
    ctx.setActiveLayer = data.useCurrentLayer;
    ctx.setActivePen = data.useCurrentAttributes;

    // each identity once, in the order given
    QList<RS_Entity*> sources;
    std::unordered_set<const RS_Entity*> seen;
    for (RS_Entity* e : entitiesList) {
        if (seen.insert(e).second) {
            sources.append(e);
        }
    }

    const int numberOfCopies = data.obtainNumberOfCopies();
    bool distancesFinite = std::isfinite(data.distance);
    for (int num = 1; distancesFinite && num <= numberOfCopies; ++num) {
        distancesFinite = std::isfinite(num * data.distance);
    }

    // The current layer, when asked for, is resolved once: a missing, frozen or
    // locked layer would receive entities the user cannot see or edit.
    bool targetUsable = true;
    if (data.useCurrentLayer) {
        for (const RS_Entity* e : sources) {
            if (e == nullptr) {
                continue;
            }
            if (RS_Graphic* graphic = e->getGraphic()) {
                const RS_Layer* layer = graphic->getActiveLayer();
                targetUsable = layer != nullptr && !layer->isFrozen() && !layer->isLocked();
            }
            break;
        }
    }

    // Selected segments of a polyline: each run of consecutive ones is offset
    // once, as a chain; its other segments report the same outcome. A
    // segment is never removed, whatever the options say: that would cut
    // its polyline.
    std::unordered_map<const RS_Entity*, LC_SegmentRun*> runOf;
    const std::vector<std::unique_ptr<LC_SegmentRun>> runs = segmentRuns(sources, runOf);
    std::unordered_map<const LC_SegmentRun*, LC_OffsetSourceOutcome> runOutcome;
    // a segment whose polyline is a source too, and that polyline
    std::unordered_map<const RS_Entity*, const RS_Polyline*> coveredByPolyline;
    for (RS_Entity* e : sources) {
        const RS_Polyline* polyline = holdingPolyline(e);
        if (polyline != nullptr && seen.count(polyline) != 0) {
            coveredByPolyline.emplace(e, polyline);
        }
    }

    std::size_t requestDeepLeft = limits.maxDeepEntitiesPerRequest;
    QList<RS_Entity*> offsetOriginals;
    for (RS_Entity* e : sources) {
        if (coveredByPolyline.count(e) != 0) {
            continue; // offset with its polyline, and reported after it
        }
        const auto run = runOf.find(e);
        LC_SegmentRun* const segmentRun = run != runOf.end() ? run->second : nullptr;
        if (segmentRun != nullptr) {
            const auto done = runOutcome.find(segmentRun);
            if (done != runOutcome.end()) {
                LC_OffsetSourceOutcome member = done->second;
                member.source = e;
                member.createdEntities.clear(); // handed over once, with the run's first segment
                outcome.sources.append(member);
                continue;
            }
        }
        RS_Entity* const target =
            (segmentRun != nullptr && segmentRun->chain != nullptr) ? segmentRun->chain.get() : e;
        LC_OffsetSourceOutcome result;
        result.source = e;
        result.copiesRequested = numberOfCopies;
        std::vector<std::unique_ptr<RS_Entity>> roots;
        auto* ownedRoots = &roots;
        if (e == nullptr) {
            result.status = LC_OffsetSourceStatus::InvalidSource;
        }
        else if (e->isDeleted() || !e->isVisible() || e->isLocked()) {
            result.status = LC_OffsetSourceStatus::NotVisibleOrLocked;
        }
        else if (!targetUsable) {
            result.status = LC_OffsetSourceStatus::TargetLayerUnavailable;
        }
        else if (!distancesFinite || !isValidOffsetBudget(limits.perSource)) {
            result.status = LC_OffsetSourceStatus::OffsetFailed;
        }
        else if (std::abs(data.distance) <= RS_TOLERANCE) {
            // the distance is its size, whatever its sign; none makes a duplicate
            result.status = LC_OffsetSourceStatus::OffsetFailed;
            result.detail = LC_OffsetFailureDetail::InvalidDistance;
        }
        else if (requestDeepLeft == 0) {
            result.status = LC_OffsetSourceStatus::LimitExceeded; // not evaluated
        }
        else {
            auto* staged = preparation ? preparation->m_data.get() : nullptr;
            const auto stored = staged ? staged->rows.find(e) : decltype(staged->rows)::iterator{};
            if (staged && stored != staged->rows.end()) {
                if (stored->second.remaining != requestDeepLeft) throw std::logic_error("Offset source allowance changed");
                result = stored->second.outcome; ownedRoots = &stored->second.roots;
            } else if (staged && (dynamic_cast<const RS_Ellipse*>(target) || dynamic_cast<const LC_OffsetCurve*>(target))) {
                staged->pending = e; staged->remaining = requestDeepLeft;
                return outcome; // Owned numeric prefixes remain staged, never published or recomputed.
            } else result.status = prepareOffsetSourceImpl(data, *target, forPreviewOnly, limits, requestDeepLeft,
                roots, result, cache, nullptr, numberOfCopies);
            if (!result.succeeded()) {
                result.copiesMade = 0; // none is handed over
                result.copyOutputCounts.clear();
                result.copiesKeptPlain = 0; result.keptPlain.clear();
                result.usage = {}; // committed output only; attempted work stays separate
            }
            if (staged && stored == staged->rows.end()) {
                LC_OffsetBatchPreparation::Data::Row row;
                row.outcome = result; row.roots = std::move(roots); row.remaining = requestDeepLeft;
                const auto added = staged->rows.emplace(e, std::move(row));
                ownedRoots = &added.first->second.roots;
            }
        }
        if (result.succeeded()) {
            // only a source that succeeded is handed over, and only then debits the request
            requestDeepLeft -= std::min(requestDeepLeft, result.usage.deepEntities);
            for (std::unique_ptr<RS_Entity>& root : *ownedRoots) {
                //highlight is used by trim actions. do not carry over flag
                root->setHighlighted(false);
                result.createdEntities.append(root.get());
                ctx += preparation ? root.get() : root.release();
            }
            if (result.complete() && segmentRun == nullptr &&
                (!e->getParent() || (e->getParent()->isDocument() && e->getParent()->findEntityIndex(e) >= 0))) {
                offsetOriginals.append(e); // a source short of copies is kept
                result.sourceRemoved = !data.keepOriginals && !forPreviewOnly;
            }
        }
        if (segmentRun != nullptr) {
            runOutcome[segmentRun] = result;
        }
        outcome.sources.append(result);
    }
    // a segment offset with its polyline fares as the polyline does
    for (RS_Entity* e : sources) {
        const auto covered = coveredByPolyline.find(e);
        if (covered == coveredByPolyline.end()) {
            continue;
        }
        const auto done = std::find_if(outcome.sources.cbegin(), outcome.sources.cend(),
                                       [&covered](const LC_OffsetSourceOutcome& o) { return o.source == covered->second; });
        if (done != outcome.sources.cend()) {
            LC_OffsetSourceOutcome member = *done;
            member.source = e;
            member.createdEntities.clear(); // handed over once, with the polyline
            outcome.sources.append(member);
        }
    }

    if (!data.keepOriginals && !forPreviewOnly) {
        ctx -= offsetOriginals;
    }
    ctx.success = outcome.anySourceSucceeded();
    return outcome;
}

/**
 * Rotates all selected entities with the given data for the rotation.
 */
bool RS_Modification::rotate(const RS_RotateData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                             LC_DocumentModificationBatch& ctx) {
    // Create new entities
    const int numberOfCopies = data.obtainNumberOfCopies();
    QList<RS_Entity*> transformedOriginals = entitiesList;
    for (const auto e : entitiesList) {
        for (int num = 1; num <= numberOfCopies; num++) {
            const double rotationAngle = data.angle * num;
            offsetTx::Steps steps{offsetTx::Step::rotate(data.center, rotationAngle)};

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
                steps.append(offsetTx::Step::rotate(rotatedRefPoint, secondRotationAngle));
            }
            RS_Entity* clone = getTransformedClone(forPreviewOnly, e, steps);
            if (!clone) {
                transformedOriginals.removeAll(e);
                delete clone;
                continue;
            }
            ctx += clone;
        }
    }
    if (!data.keepOriginals) {
        ctx -= transformedOriginals;
    }
    return !transformedOriginals.isEmpty();
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
    QList<RS_Entity*> transformedOriginals = entitiesList;

    // Create new entities
    for (const RS_Entity* e : entitiesToScale) {
        if (e != nullptr) {
            for (int num = 1; num <= numberOfCopies; num++) {
                const auto factor = RS_Math::pow(data.factor, num);
                RS_Entity* clone = getTransformedClone(forPreviewOnly, e,
                    {offsetTx::Step::scale(data.referencePoint, factor)}, std::abs(factor.x) != std::abs(factor.y));
                if (!clone || !acceptedInsertSourceEdit(*clone)) {
                    transformedOriginals.removeAll(const_cast<RS_Entity*>(e));
                    delete clone;
                    continue;
                }
                ctx += clone;
            }
        }
    }

    // delete temporary entities created for scale
    for (const auto e : createdEntities) {
        delete e;
    }

    if (!data.keepOriginals) {
        ctx -= transformedOriginals;
    }

    return !transformedOriginals.isEmpty();
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

    QList<RS_Entity*> transformedOriginals = entitiesList;
    // Create new entities
    for (const auto e : entitiesList) {
        for (int num = 1; num <= numberOfCopies; ++num) {
            RS_Entity* clone = getTransformedClone(forPreviewOnly, e,
                {offsetTx::Step::mirror(data.axisPoint1, data.axisPoint2)});
            if (!clone || !acceptedInsertSourceEdit(*clone)) {
                transformedOriginals.removeAll(e);
                delete clone;
                continue;
            }
            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= transformedOriginals;
    }

    return !transformedOriginals.isEmpty();
}

/**
 * Rotates entities around two centers with the given parameters.
 */
bool RS_Modification::rotate2(const RS_Rotate2Data& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                              LC_DocumentModificationBatch& ctx) {
    const int numberOfCopies = data.obtainNumberOfCopies();
    QList<RS_Entity*> transformedOriginals = entitiesList;

    // Create new entities
    for (const auto e : entitiesList) {
        for (int num = 1; num <= numberOfCopies; num++) {
            const double angle1ForCopy = /*data.sameAngle1ForCopies ?  data.angle1 :*/ data.angle1 * num;
            const double angle2ForCopy = data.sameAngle2ForCopies ? data.angle2 : data.angle2 * num;
            RS_Vector center2 = data.center2;
            center2.rotate(data.center1, angle1ForCopy);
            RS_Entity* clone = getTransformedClone(forPreviewOnly, e,
                {offsetTx::Step::rotate(data.center1, angle1ForCopy), offsetTx::Step::rotate(center2, angle2ForCopy)});
            if (!clone) { transformedOriginals.removeAll(e); continue; }
            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= transformedOriginals;
    }
    return !transformedOriginals.isEmpty();
}

/**
 * Moves and rotates entities with the given parameters.
 */
bool RS_Modification::moveRotate(const RS_MoveRotateData& data, const QList<RS_Entity*>& entitiesList, const bool forPreviewOnly,
                                 LC_DocumentModificationBatch& ctx) {
    const int numberOfCopies = data.obtainNumberOfCopies();
    QList<RS_Entity*> transformedOriginals = entitiesList;

    // Create new entities
    for (const auto entity : entitiesList) {
        for (int num = 1; num <= numberOfCopies; ++num) {
            const RS_Vector& offset = data.offset * num;
            const double angleForCopy = data.sameAngleForCopies ? data.angle : data.angle * num;
            RS_Entity* clone = getTransformedClone(forPreviewOnly, entity,
                {offsetTx::Step::move(offset), offsetTx::Step::rotate(data.referencePoint + offset, angleForCopy)});
            if (!clone) { transformedOriginals.removeAll(entity); continue; }
            ctx += clone;
        }
    }

    if (!data.keepOriginals) {
        ctx -= transformedOriginals;
    }
    return !transformedOriginals.isEmpty();
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
    if (both && (limitEntity->rtti() == RS2::EntityOffsetCurve || isNativeCacheChild(*limitEntity))) return {};

    // A container limit entity can still bound a one-sided trim, but it must never
    // be cast to RS_AtomicEntity: trimStartpoint(), trimEndpoint() and getTrimPoint()
    // are declared only there, so dispatching them through a container pointer calls
    // whatever occupies the same vtable slot. Nor is a limit the document does not
    // hold itself trimmed, see isReplaceableByTrim().
    const bool limitTrimmable = limitEntity->isAtomic() && isReplaceableByTrim(*limitEntity);
    if (both && !limitTrimmable) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "RS_Modification::trim: limitEntity cannot be trimmed, trimming one entity only");
    }

    LC_TrimResult result;

    if (trimEntity->isLocked() || !trimEntity->isVisible() || !isReplaceableByTrim(*trimEntity)) {
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
        if (both && limitTrimmable) {
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
    const bool trimBoth = both && limitTrimmable && !limitEntity->isLocked() && limitEntity->isVisible();
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

namespace {
namespace storedTrim = LC_OffsetCacheTrim;
namespace trimRequest = LC_OffsetWorkerRequest;

storedTrim::Limits lowerStoredTrimLimits(storedTrim::Limits remaining) noexcept {
    const storedTrim::Limits hard;
#define LC_TRIM_LOWER(f) remaining.f = std::min(remaining.f, hard.f)
    LC_TRIM_LOWER(maxSpansPerCurve); LC_TRIM_LOWER(maxInputSpans); LC_TRIM_LOWER(maxInputRecords);
    LC_TRIM_LOWER(maxBoxPairs); LC_TRIM_LOWER(maxLeaves); LC_TRIM_LOWER(maxGeneratedItems);
    LC_TRIM_LOWER(maxOrderingComparisons); LC_TRIM_LOWER(maxRoots); LC_TRIM_LOWER(maxDepth);
    LC_TRIM_LOWER(maxLeafDepth); LC_TRIM_LOWER(maxFragments); LC_TRIM_LOWER(maxOutputSpans);
    LC_TRIM_LOWER(maxOutputControls); LC_TRIM_LOWER(maxOutputKnots);
#undef LC_TRIM_LOWER
    return remaining;
}

bool debitStoredTrimWork(storedTrim::Limits& remaining, storedTrim::Work& total,
                         const storedTrim::Work& work) noexcept {
#define LC_TRIM_DEBIT(f, amount) if ((amount) > remaining.f) return false; remaining.f -= (amount)
    LC_TRIM_DEBIT(maxInputSpans, work.inputSpans); LC_TRIM_DEBIT(maxInputRecords, work.inputRecords);
    LC_TRIM_DEBIT(maxRoots, work.rootRecords); LC_TRIM_DEBIT(maxBoxPairs, work.intersections.totalBoxPairs);
    LC_TRIM_DEBIT(maxLeaves, work.intersections.generatedLeaves);
    LC_TRIM_DEBIT(maxGeneratedItems, work.kernelItems + work.intersections.generatedItems);
    LC_TRIM_DEBIT(maxOrderingComparisons, work.kernelComparisons + work.intersections.orderingComparisons);
    LC_TRIM_DEBIT(maxOutputSpans, work.outputSpans); LC_TRIM_DEBIT(maxOutputControls, work.outputControls);
    LC_TRIM_DEBIT(maxOutputKnots, work.outputKnots);
#undef LC_TRIM_DEBIT
#define LC_TRIM_SUM(f) total.f += work.f
    LC_TRIM_SUM(inputSpans); LC_TRIM_SUM(inputRecords); LC_TRIM_SUM(rootRecords); LC_TRIM_SUM(kernelItems);
    LC_TRIM_SUM(kernelComparisons); LC_TRIM_SUM(outputSpans); LC_TRIM_SUM(outputControls); LC_TRIM_SUM(outputKnots);
#undef LC_TRIM_SUM
    auto& s = total.intersections; const auto& d = work.intersections;
#define LC_TRIM_SUM(f) s.f += d.f
    LC_TRIM_SUM(candidateBoxPairs); LC_TRIM_SUM(refinementBoxPairs); LC_TRIM_SUM(certificateBoxPairs);
    LC_TRIM_SUM(totalBoxPairs); LC_TRIM_SUM(generatedLeaves); LC_TRIM_SUM(generatedItems);
    LC_TRIM_SUM(orderingComparisons); LC_TRIM_SUM(validationSegmentVisits); LC_TRIM_SUM(validationNextCalls);
#undef LC_TRIM_SUM
#define LC_TRIM_MAX(f) s.f = std::max(s.f, d.f)
    LC_TRIM_MAX(deepestPreparation); LC_TRIM_MAX(deepestPairRefinement);
    LC_TRIM_MAX(deepestOrderingRefinement); LC_TRIM_MAX(deepestRootRefinement);
#undef LC_TRIM_MAX
    return true;
}

bool sameStoredTrimLimits(const storedTrim::Limits& a, const storedTrim::Limits& b) noexcept {
#define LC_TRIM_FIELDS(v) std::tie(v.maxSpansPerCurve, v.maxInputSpans, v.maxInputRecords, \
    v.maxBoxPairs, v.maxLeaves, v.maxGeneratedItems, v.maxOrderingComparisons, v.maxRoots, \
    v.maxDepth, v.maxLeafDepth, v.maxFragments, v.maxOutputSpans, v.maxOutputControls, v.maxOutputKnots)
    return LC_TRIM_FIELDS(a) == LC_TRIM_FIELDS(b);
#undef LC_TRIM_FIELDS
}

const storedTrim::Result* storedTrimPayload(const LC_OffsetWorkerCompletion& completion) noexcept {
    if (!completion.isJoined()) return nullptr;
    const auto& request = completion.request(); const auto& reply = completion.reply();
    const auto& a = request.token; const auto& b = reply.token;
    if ((request.operation.index() != 2 && request.operation.index() != 3) ||
        reply.operation != request.operation.index() || reply.failure != LC_OffsetWorkerResult::Failure::None ||
        std::tie(a.requestId, a.generation, a.sourceRevision, a.undoEpoch) !=
            std::tie(b.requestId, b.generation, b.sourceRevision, b.undoEpoch)) return nullptr;
    return std::get_if<storedTrim::Result>(&reply.result);
}

bool sameStoredTrimValue(const std::optional<QCborArray>& a, const std::optional<QCborArray>& b) {
    // Compare the complete admitted typed value, including latent fields and
    // signed zero, rather than a lossy geometry fingerprint or CBOR numeric equality.
    return a && b && QCborValue(*a).toCbor() == QCborValue(*b).toCbor();
}

std::optional<trimRequest::Cutter> storedTrimCutter(const RS_Entity& source) {
    if (const auto* line = dynamic_cast<const RS_Line*>(&source))
        return trimRequest::Cutter{trimRequest::Segment{line->getStartpoint(), line->getEndpoint()}};
    if (const auto* arc = dynamic_cast<const RS_Arc*>(&source)) return trimRequest::Cutter{arc->getData()};
    if (const auto* circle = dynamic_cast<const RS_Circle*>(&source)) return trimRequest::Cutter{circle->getData()};
    if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&source)) return trimRequest::Cutter{ellipse->getData()};
    if (const auto* spline = dynamic_cast<const RS_Spline*>(&source)) {
        // The one-sided ordinary limit was not replacement-admitted. Bound its
        // raw arrays before copying, just as the kernel does on the sync path.
        if (!trimRequest::Values::encodeCache(spline->getData())) return std::nullopt;
        return trimRequest::Cutter{spline->getData()};
    }
    return std::nullopt;
}

const storedTrim::Result* matchedStoredTrim(const trimRequest::Request& retained,
                                            const LC_OffsetWorkerCompletion& completion) {
    const auto* result = storedTrimPayload(completion);
    if (!result || retained.operation.index() != completion.request().operation.index() ||
        !sameStoredTrimValue(trimRequest::encode(retained), trimRequest::encode(completion.request()))) return nullptr;
    return result;
}

std::optional<storedTrim::Result> consumeStoredTrimCache(const trimRequest::Request& retained,
    const LC_OffsetWorkerCompletion& completion, const RS_SplineData& cache, const RS_Entity& cutter,
    const storedTrim::Pick& pick, const storedTrim::Limits& remaining) {
    const auto* result = matchedStoredTrim(retained, completion);
    const auto* request = std::get_if<trimRequest::TrimCache>(&completion.request().operation);
    if (!result || !request || !sameStoredTrimLimits(request->limits, remaining) ||
        !sameStoredTrimValue(trimRequest::Values::encodeCache(cache),
            trimRequest::Values::encodeCache(request->cache)) ||
        !sameStoredTrimValue(trimRequest::Values::encodePick(pick),
            trimRequest::Values::encodePick(request->pick))) return std::nullopt;
    const auto value = storedTrimCutter(cutter);
    if (!value || !sameStoredTrimValue(trimRequest::Values::encodeCutter(*value),
        trimRequest::Values::encodeCutter(request->cutter))) return std::nullopt;
    return *result;
}

std::optional<storedTrim::Result> consumeStoredTrimConic(const trimRequest::Request& retained,
    const LC_OffsetWorkerCompletion& completion, const RS_SplineData& cache, const RS_Entity& conic,
    const storedTrim::Pick& conicPick, const std::optional<storedTrim::Pick>& cachePick,
    const storedTrim::Limits& remaining) {
    const auto* result = matchedStoredTrim(retained, completion);
    const auto* request = std::get_if<trimRequest::TrimConic>(&completion.request().operation);
    if (!result || !request || !sameStoredTrimLimits(request->limits, remaining) ||
        !sameStoredTrimValue(trimRequest::Values::encodeCache(cache),
            trimRequest::Values::encodeCache(request->cache)) ||
        !sameStoredTrimValue(trimRequest::Values::encodePick(conicPick),
            trimRequest::Values::encodePick(request->conicPick)) ||
        cachePick.has_value() != request->cachePick.has_value() ||
        (cachePick && !sameStoredTrimValue(trimRequest::Values::encodePick(*cachePick),
            trimRequest::Values::encodePick(*request->cachePick)))) return std::nullopt;
    const auto value = storedTrimCutter(conic);
    if (!value || !sameStoredTrimValue(trimRequest::Values::encodeCutter(*value),
        trimRequest::Values::encodeCutter(request->conic))) return std::nullopt;
    return *result;
}
} // namespace

std::optional<LC_OffsetCacheTrim::Limits> RS_Modification::storedCacheTrimRemainingLimits(
    const LC_OffsetCacheTrim::Limits& limits, const LC_OffsetWorkerCompletion* previous) noexcept {
    auto remaining = lowerStoredTrimLimits(limits);
    if (!previous) return remaining;
    const auto* request = std::get_if<trimRequest::TrimCache>(&previous->request().operation);
    const auto* result = storedTrimPayload(*previous);
    if (!request || !result || result->refusal != storedTrim::Refusal::None ||
        !sameStoredTrimLimits(request->limits, remaining)) return std::nullopt;
    storedTrim::Work ignored;
    if (!debitStoredTrimWork(remaining, ignored, result->work)) return std::nullopt;
    return remaining;
}

LC_TrimResult RS_Modification::trimStoredCache(RS_Document& document, RS_Entity& target,
    const RS_Vector& targetPick, RS_Entity& cutter, const RS_Vector& cutterPick, const bool both,
    LC_DocumentModificationBatch& ctx, const LC_StoredCacheTrimOptions& options) {
    return trimStoredCacheImpl(document, target, targetPick, cutter, cutterPick, both, ctx, options, nullptr);
}

LC_TrimResult RS_Modification::trimStoredCachePrepared(RS_Document& document, RS_Entity& target,
    const RS_Vector& targetPick, RS_Entity& cutter, const RS_Vector& cutterPick, const bool both,
    LC_DocumentModificationBatch& ctx, const LC_StoredCacheTrimCompletions& completed,
    const LC_StoredCacheTrimOptions& options) {
    return trimStoredCacheImpl(document, target, targetPick, cutter, cutterPick, both, ctx, options, &completed);
}

LC_TrimResult RS_Modification::trimStoredCacheImpl(RS_Document& document, RS_Entity& target,
    const RS_Vector& targetPick, RS_Entity& cutter, const RS_Vector& cutterPick, const bool both,
    LC_DocumentModificationBatch& ctx, const LC_StoredCacheTrimOptions& options,
    const LC_StoredCacheTrimCompletions* completed) {
    namespace trim = LC_OffsetCacheTrim;
    LC_TrimResult result;
    result.cacheRefusal = trim::Refusal::InvalidInput;
    result.intersection1 = result.intersection2 = RS_Vector{false};
    const auto admittedUndoEpoch = document.undoStateEpoch();
    const auto checkCancel = [&] {
        return options.cancellation.requested && options.cancellation.requested->load(std::memory_order_relaxed);
    };
    const auto refuse = [&](trim::Refusal reason) {
        // A late refusal must not expose pointers into privately owned outputs.
        result.trimmed1 = result.trimmed2 = nullptr;
        result.intersection1 = result.intersection2 = RS_Vector{false};
        result.nativeDemotions = 0;
        result.result = false; result.cacheRefusal = reason; return result;
    };
    constexpr std::size_t memberCap = std::size_t{1} << 20;
    const auto outputCap = std::min<std::size_t>(options.maxOutputEntities, 4);
    const auto displayCap = std::min(options.maxDisplayEntities, kDefaultOffsetDeepEntitiesPerSource);
    // Membership is checked by POINTER before dereferencing a replacement
    // source. No countDeep(), recursive findEntity(), native clone or C call.
    const auto member = [&](const RS_Entity* source) {
        return document.size() <= memberCap && document.isOwner() &&
            std::find(document.begin(), document.end(), source) != document.end() &&
            source->getParent() == &document && !source->isDeleted() && source->isVisible();
    };
    const auto live = [&](const RS_Entity* source) { return member(source) && !source->isLocked(); };
    const auto nativeReady = [&](const LC_OffsetCurve& source) {
        if (!source.isOwner() || source.size() > kDefaultOffsetDeepEntitiesPerSource || !source.hasEditableDefinition()) return false;
        for (const auto* child : source) {
            if (!child || child->getParent() != &source || child->rtti() != RS2::EntityLine) return false;
        }
        return true;
    };
    const auto conic = [](const RS_Entity* source) {
        return (source->rtti() == RS2::EntityCircle && dynamic_cast<const RS_Circle*>(source)) ||
            (source->rtti() == RS2::EntityArc && dynamic_cast<const RS_Arc*>(source)) ||
            (source->rtti() == RS2::EntityEllipse && dynamic_cast<const RS_Ellipse*>(source));
    };
    const auto eligible = [&](const RS_Entity* source) {
        if (!live(source)) return false;
        if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(source)) return nativeReady(*native);
        if (conic(source)) return true;
        if (source->rtti() == RS2::EntitySpline) {
            const auto* spline = dynamic_cast<const RS_Spline*>(source);
            if (!spline || !spline->isOwner() || spline->size() > kDefaultOffsetDeepEntitiesPerSource) return false;
            const auto& data = spline->getData(); const trim::Limits hard;
            if (data.degree != 1 && data.degree != 3) return false;
            const auto layout = LC_EllipseOffsetGeometry::cacheLayoutCountsFromControls(data.degree, data.controlPoints.size());
            if (!layout || layout->spans > hard.maxSpansPerCurve) return false;
            std::size_t records = 0;
            for (const auto n : {data.controlPoints.size(), data.weights.size(), data.knotslist.size(), data.fitPoints.size(),
                    data.savedOpenKnots.size(), layout->spans, layout->spans}) {
                if (n > hard.maxInputRecords - records) return false;
                records += n;
            }
            for (const auto* child : *spline) if (!child || child->getParent() != spline || child->rtti() != RS2::EntityLine) return false;
            return true; // Raw counts/layout/finite checks precede copying in the pure kernel.
        }
        return source->rtti() == RS2::EntityLine && dynamic_cast<const RS_Line*>(source);
    };
    try {
        if (checkCancel()) return refuse(trim::Refusal::Cancelled);
        if (completed) {
            if (completed->publicationUndoEpoch != admittedUndoEpoch ||
                bool(completed->secondRequest) != bool(completed->second) ||
                !trimRequest::admitted(completed->firstRequest)) return refuse(trim::Refusal::InvalidInput);
            const auto& first = completed->firstRequest.token;
            const auto submittedEpoch = static_cast<std::uint64_t>(first.undoEpoch);
            if (admittedUndoEpoch != submittedEpoch && admittedUndoEpoch != submittedEpoch + 1)
                return refuse(trim::Refusal::InvalidInput);
            if (completed->secondRequest) {
                const auto& second = completed->secondRequest->token;
                if (second.requestId <= first.requestId ||
                    std::tie(second.generation, second.sourceRevision, second.undoEpoch) !=
                        std::tie(first.generation, first.sourceRevision, first.undoEpoch))
                    return refuse(trim::Refusal::InvalidInput);
            }
        }
        if (&target == &cutter || !outputCap || !displayCap || document.size() > memberCap ||
            ctx.entitiesToAdd.size() > qsizetype(memberCap) || ctx.entitiesToDelete.size() > qsizetype(memberCap))
            return refuse(trim::Refusal::LimitExceeded);
        const auto* nativeA = dynamic_cast<const LC_OffsetCurve*>(&target);
        const auto* nativeB = dynamic_cast<const LC_OffsetCurve*>(&cutter);
        if (!nativeA && !nativeB) return refuse(trim::Refusal::UnsupportedCutter);
        // Capture EVERY native operand before replacement-source getter reads.
        // Native admission/snapshot getters are bounded, shared value reads;
        // an ordinary operand's virtual endpoints must not precede this baseline.
        struct NativeInput {
            const LC_OffsetCurve* address = nullptr;
            unsigned long long id = 0;
            bool replacement = false;
            std::unique_ptr<LC_OffsetCurveEditSnapshot> snapshot;
        };
        std::array<NativeInput, 2> nativeInputs;
        nativeInputs[0].address = nativeA; nativeInputs[0].replacement = true;
        nativeInputs[1].address = nativeB; nativeInputs[1].replacement = both;
        for (auto& input : nativeInputs) if (input.address) {
            const auto reason = input.replacement ? trim::Refusal::UnsupportedLayout : trim::Refusal::UnsupportedCutter;
            if (!member(input.address) || (input.replacement && input.address->isLocked()) || !nativeReady(*input.address))
                return refuse(reason);
            input.id = input.address->getId();
            input.snapshot = LC_OffsetCurveEditSnapshot::capture(*input.address);
            if (!input.snapshot) return refuse(trim::Refusal::LimitExceeded);
        }
        if (!eligible(&target) || (both && !eligible(&cutter))) return refuse(trim::Refusal::UnsupportedLayout);
        if (nativeB && !nativeReady(*nativeB)) return refuse(trim::Refusal::UnsupportedCutter);
        // A one-sided limit is read-only, but a native limit must still be a
        // live top-level source, not a borrowed/generated/block clone.
        if (nativeB && !member(&cutter)) return refuse(trim::Refusal::UnsupportedCutter);
        const auto supportedLimit = [](const RS_Entity& e) {
            return dynamic_cast<const RS_Line*>(&e) || dynamic_cast<const RS_Arc*>(&e) ||
                dynamic_cast<const RS_Circle*>(&e) || dynamic_cast<const RS_Ellipse*>(&e) ||
                dynamic_cast<const RS_Spline*>(&e);
        };
        if (!supportedLimit(cutter)) return refuse(trim::Refusal::UnsupportedCutter);
        std::array<RS_Entity*, 2> sources{{&target, both ? &cutter : nullptr}};
        std::array<std::uint64_t, 2> stamps{};
        const auto stamp = [](const RS_Entity& e) {
            if (const auto* spline = dynamic_cast<const RS_Spline*>(&e)) return RS_Spline::geometryFingerprint(spline->getData());
            RS_SplineData data{1, false};
            if (const auto* circle = dynamic_cast<const RS_Circle*>(&e)) {
                data.controlPoints = {circle->getCenter()}; data.weights = {circle->getRadius()};
            } else if (const auto* arc = dynamic_cast<const RS_Arc*>(&e)) {
                const auto& d = arc->getData(); data.controlPoints = {d.center};
                data.weights = {d.radius, d.angle1, d.angle2, d.reversed ? 1. : 0.};
            } else if (const auto* ellipse = dynamic_cast<const RS_Ellipse*>(&e)) {
                const auto& d = ellipse->getData(); data.controlPoints = {d.center, d.majorP};
                data.weights = {d.ratio, d.angle1, d.angle2, d.reversed ? 1. : 0., d.isArc ? 1. : 0.,
                    d.angleDegrees, d.startAngleDegrees, d.otherAngleDegrees, d.angularLength};
            } else data.controlPoints = {e.getStartpoint(), e.getEndpoint()};
            return RS_Spline::geometryFingerprint(data);
        };
        // Attribute copies use the same RS_Entity base copy as E-tools. Admit
        // opaque maps/vectors first; the schema parser never mutates variants.
        std::array<LC_OffsetCurveXData::XData, 2> foreign;
        for (std::size_t i = 0; i < sources.size() && sources[i]; ++i) {
            if (!sources[i]->cloneMetadataFits(1024, std::size_t{1} << 20, 4096)) return refuse(trim::Refusal::LimitExceeded);
            auto parsed = LC_OffsetCurveXData::parseAndStrip(sources[i]->getDrwExtData());
            if (!parsed.remaining) return refuse(trim::Refusal::LimitExceeded);
            foreign[i] = std::move(*parsed.remaining); stamps[i] = stamp(*sources[i]);
        }
        auto remaining = lowerStoredTrimLimits(options.kernel);
        const auto debit = [&](const trim::Work& work) {
            return debitStoredTrimWork(remaining, result.cacheWork, work);
        };
        std::array<trim::Result, 2> prepared;
        const bool conicReplacement = conic(&target) || (both && conic(&cutter));
        if (completed && completed->second && (conicReplacement || !both))
            return refuse(trim::Refusal::InvalidInput);
        if (conicReplacement) {
            const std::size_t ci = conic(&target) ? 0 : 1, ni = 1 - ci;
            const auto* cache = ci == 0 ? nativeB : nativeA;
            if (!cache) return refuse(trim::Refusal::UnsupportedLayout);
            const trim::Pick cp{ci == 0 ? targetPick : cutterPick};
            std::optional<trim::Pick> np;
            if (both) np = trim::Pick{ni == 0 ? targetPick : cutterPick};
            trim::Result pair;
            if (completed) {
                auto received = consumeStoredTrimConic(completed->firstRequest, completed->first,
                    cache->getData(), *sources[ci], cp, np, remaining);
                if (!received) return refuse(trim::Refusal::InvalidInput);
                pair = std::move(*received);
            } else pair = trim::prepareConic(cache->getData(), *sources[ci], cp, np, remaining, options.cancellation);
            if (!debit(pair.work)) return refuse(trim::Refusal::LimitExceeded);
            if (pair.refusal != trim::Refusal::None) return refuse(pair.refusal);
            if (pair.conicFragments.empty() || (both && pair.fragments.empty())) return refuse(trim::Refusal::Arithmetic);
            prepared[ni] = std::move(pair);
            prepared[ci].conicFragments.swap(prepared[ni].conicFragments);
        } else {
            for (std::size_t i = 0; i < sources.size() && sources[i]; ++i) {
                if (checkCancel()) return refuse(trim::Refusal::Cancelled);
                RS_SplineData line{1, false}; const RS_SplineData* raw = nullptr;
                if (const auto* spline = dynamic_cast<const RS_Spline*>(sources[i])) raw = &spline->getData();
                else {
                    if (sources[i]->rtti() != RS2::EntityLine) return refuse(trim::Refusal::UnsupportedLayout);
                    line.controlPoints = {sources[i]->getStartpoint(), sources[i]->getEndpoint()};
                    line.knotslist = {0, 0, 1, 1}; line.weights = {1, 1}; raw = &line;
                }
                if (completed) {
                    const auto* retained = i == 0 ? &completed->firstRequest : completed->secondRequest;
                    const auto* receipt = i == 0 ? &completed->first : completed->second;
                    if (!retained || !receipt) return refuse(trim::Refusal::InvalidInput);
                    auto received = consumeStoredTrimCache(*retained, *receipt, *raw,
                        i == 0 ? cutter : target, {i == 0 ? targetPick : cutterPick}, remaining);
                    if (!received) return refuse(trim::Refusal::InvalidInput);
                    prepared[i] = std::move(*received);
                } else prepared[i] = trim::prepare(*raw, i == 0 ? cutter : target,
                    {i == 0 ? targetPick : cutterPick}, remaining, options.cancellation);
                if (!debit(prepared[i].work)) return refuse(trim::Refusal::LimitExceeded);
                if (prepared[i].refusal != trim::Refusal::None) return refuse(prepared[i].refusal);
                if (prepared[i].fragments.empty()) return refuse(trim::Refusal::Arithmetic);
            }
        }
        std::array<std::size_t, 2> sideCount{};
        std::size_t count = 0, display = 0;
        for (std::size_t i = 0; i < sources.size() && sources[i]; ++i) {
            sideCount[i] = prepared[i].fragments.size() + prepared[i].conicFragments.size();
            if (!sideCount[i] || sideCount[i] > outputCap - count) return refuse(trim::Refusal::LimitExceeded);
            count += sideCount[i];
            if (conic(sources[i])) {
                if (!prepared[i].fragments.empty() || sideCount[i] > displayCap - display) return refuse(trim::Refusal::LimitExceeded);
                display += sideCount[i];
            } else for (const auto& fragment : prepared[i].fragments) {
                const auto upper = sources[i]->rtti() == RS2::EntityLine ? std::optional<std::size_t>{1} :
                    RS_Spline::displayEntityUpperBound(fragment.data);
                if (!upper || *upper > displayCap - display) return refuse(trim::Refusal::LimitExceeded);
                display += *upper;
            }
        }
        // BOTH geometry preparations and ALL count/attribute/display admissions
        // precede the first output constructor. No original has been changed.
        std::vector<std::unique_ptr<RS_Entity>> owned; owned.reserve(count);
        for (std::size_t i = 0; i < sources.size() && sources[i]; ++i) {
            if (dynamic_cast<const LC_OffsetCurve*>(sources[i])) ++result.nativeDemotions;
            for (std::size_t j = 0; j < sideCount[i]; ++j) {
                if (checkCancel()) return refuse(trim::Refusal::Cancelled);
                std::unique_ptr<RS_Entity> output;
                if (conic(sources[i])) {
                    const auto& data = prepared[i].conicFragments[j].data;
                    // Owned typed restrictions of the original primitive, never
                    // endpoint chords, display intersections or fitted splines.
                    if (sources[i]->rtti() == RS2::EntityEllipse) {
                        auto clone = std::make_unique<RS_Ellipse>(*static_cast<const RS_Ellipse*>(sources[i]));
                        clone->setParent(nullptr);
                        // Keep at least one nonzero angle installed while the
                        // ordinary API recalculates its fuzzy whole-ellipse flag.
                        if (std::abs(data.angle1) >= std::abs(data.angle2)) {
                            clone->setAngle1(data.angle1); clone->setAngle2(data.angle2);
                        } else { clone->setAngle2(data.angle2); clone->setAngle1(data.angle1); }
                        if (clone->getAngle1() != data.angle1 || clone->getAngle2() != data.angle2 || !clone->isEllipticArc())
                            return refuse(trim::Refusal::Arithmetic);
                        output = std::move(clone);
                    } else if (sources[i]->rtti() == RS2::EntityArc) {
                        auto clone = std::make_unique<RS_Arc>(*static_cast<const RS_Arc*>(sources[i])); clone->setParent(nullptr);
                        clone->setData(RS_ArcData{data.center, data.majorP.x, data.angle1, data.angle2, data.reversed});
                        output = std::move(clone);
                    } else output = std::make_unique<RS_Arc>(nullptr,
                        RS_ArcData{data.center, data.majorP.x, data.angle1, data.angle2, data.reversed});
                    if (!(output->getLength() > 0) || !std::isfinite(output->getLength())) return refuse(trim::Refusal::Arithmetic);
                } else if (sources[i]->rtti() == RS2::EntityLine) {
                    const auto& fragment = prepared[i].fragments[j];
                    if (fragment.data.degree != 1 || fragment.data.controlPoints.size() != 2) return refuse(trim::Refusal::UnsupportedLayout);
                    output = std::make_unique<RS_Line>(nullptr, fragment.data.controlPoints.front(), fragment.data.controlPoints.back());
                } else {
                    const auto& fragment = prepared[i].fragments[j];
                    auto spline = std::make_unique<RS_Spline>(nullptr, fragment.data);
                    if (!spline->validate() || !spline->size() ||
                        RS_Spline::geometryFingerprint(spline->getData()) != RS_Spline::geometryFingerprint(fragment.data))
                        return refuse(trim::Refusal::Arithmetic);
                    output = std::move(spline);
                }
                output->RS_Entity::operator=(*sources[i]);
                output->setDrwExtData(foreign[i]);
                output->clearDwgProvenance(RS_Entity::Identity);
                output->setOwnerLineage(0); output->setLineage(RS_Entity::newLineage());
                output->setParent(&document); output->calculateBorders();
                if (!LC_EllipseOffsetGeometry::isFinite2D(output->getMin()) ||
                    !LC_EllipseOffsetGeometry::isFinite2D(output->getMax())) return refuse(trim::Refusal::Arithmetic);
                owned.push_back(std::move(output));
            }
        }
        std::vector<const RS_Entity*> roots; roots.reserve(owned.size());
        for (const auto& e : owned) roots.push_back(e.get());
        if (measureOffsetOutput(roots, displayCap).status != LC_OffsetTreeStatus::Ok) return refuse(trim::Refusal::LimitExceeded);
        if (checkCancel()) return refuse(trim::Refusal::Cancelled);
        for (std::size_t i = 0; i < sources.size() && sources[i]; ++i) {
            if (!eligible(sources[i]) || stamp(*sources[i]) != stamps[i] || ctx.entitiesToDelete.contains(sources[i]))
                return refuse(trim::Refusal::UnsupportedLayout);
        }
        if (count > memberCap - std::size_t(ctx.entitiesToAdd.size()) ||
            (both ? 2u : 1u) > memberCap - std::size_t(ctx.entitiesToDelete.size())) return refuse(trim::Refusal::LimitExceeded);
        auto additions = ctx.entitiesToAdd, deletions = ctx.entitiesToDelete;
        additions.reserve(additions.size() + qsizetype(count)); deletions.reserve(deletions.size() + (both ? 2 : 1));
        for (const auto& e : owned) additions.append(e.get());
        deletions.append(&target); if (both) deletions.append(&cutter);
        result.trimmed1 = owned.front().get();
        if (both) result.trimmed2 = owned[sideCount[0]].get();
        std::optional<std::size_t> firstRoot;
        for (const auto& fragment : prepared[0].fragments) {
            const std::array<std::pair<const trim::Endpoint*, RS_Vector>, 2> ends{{
                {&fragment.start, fragment.data.controlPoints.front()}, {&fragment.end, fragment.data.controlPoints.back()}}};
            for (const auto& [end, point] : ends) if (end->rootId) {
                if (!firstRoot) { firstRoot = end->rootId; result.intersection1 = point; }
                else if (end->rootId != firstRoot) result.intersection2 = point;
            }
        }
        // No allocation or user callback after the first swap. The standard
        for (std::size_t j = 0; j < prepared[0].conicFragments.size(); ++j) {
            const auto& fragment = prepared[0].conicFragments[j];
            if (fragment.startRoot) result.intersection1 = owned[j]->getStartpoint();
            if (fragment.endRoot) result.intersection2 = owned[j]->getEndpoint();
        }
        // All typed geometry, attributes and BOTH output sets are complete.
        // document batch is the only publication; no compensating rollback.
        // This is after the final source stamps and typed endpoint getters.
        // Revalidate the complete native root set, not only the last operand.
        // Pointer membership precedes ID/type/value reads through each address.
        for (const auto& input : nativeInputs) if (input.address) {
            const auto reason = input.replacement ? trim::Refusal::UnsupportedLayout : trim::Refusal::UnsupportedCutter;
            if (!member(input.address) || input.address->getId() != input.id ||
                (input.replacement && input.address->isLocked()) || !nativeReady(*input.address) ||
                !input.snapshot->matches(*input.address)) return refuse(reason);
        }
        std::vector<LC_OffsetCurveFinalInput::Input> finalInputs;
        finalInputs.reserve(2);
        for (const auto& input : nativeInputs) if (input.address)
            finalInputs.push_back({input.address, input.snapshot.get(), input.replacement});
        if (document.undoStateEpoch() != admittedUndoEpoch)
            return refuse(trim::Refusal::UnsupportedLayout);
        LC_OffsetCurveFinalInput::validate(document, finalInputs);
        for (const auto& input : finalInputs) if (!input.matches)
            return refuse(input.requireUnlocked ? trim::Refusal::UnsupportedLayout : trim::Refusal::UnsupportedCutter);
        // The atomic token read cannot invalidate the stored seal. Keep this
        // final probe after all validation allocations, immediately before handoff.
        if (checkCancel()) return refuse(trim::Refusal::Cancelled);
        // No operand callback or notification follows.
        ctx.entitiesToAdd.swap(additions); ctx.entitiesToDelete.swap(deletions); ctx.dontSetActiveLayerAndPen();
        for (auto& e : owned) e.release();
        result.result = true; result.cacheRefusal.reset();
        return result;
    } catch (const std::bad_alloc&) { return refuse(trim::Refusal::AllocationFailure); }
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
    if (isNativeCacheChild(*entityToTrim)) return nullptr;

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
    if (isNativeCacheChild(*cutEntity)) return false;

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
            // cut() returns a freshly allocated spline, never the receiver,
            // so the clone it is called on has to be owned here.
            const std::unique_ptr<RS_Entity> spline{cutEntity->clone()};
            cut2 = static_cast<LC_SplinePoints*>(spline.get())->cut(cutCoord);
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
            if (entity1IndexInPolyline == 0 && static_cast<unsigned int>(entity2IndexInPolyline) == polylineSegmentsCount - 1 && polylineSegmentsCount > 2) {
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

    // create 2 tmp parallels; the fillet intersects and trims each as one
    // atomic curve, so a spline's offset, an RS_Spline drawn as lines, counts as none
    auto singleParallel = [&coord, &data](RS_AtomicEntity* entity) {
        QList<RS_Entity*> parallels;
        RS_Creation::createParallel(coord, data.radius, 1, entity, false, parallels);
        if (parallels.size() != 1 || parallels.front()->isContainer()) {
            qDeleteAll(parallels);
            return std::unique_ptr<RS_Entity>{};
        }
        return std::unique_ptr<RS_Entity>{parallels.front()};
    };
    const std::unique_ptr<RS_Entity> par1 = singleParallel(entity1);
    const std::unique_ptr<RS_Entity> par2 = singleParallel(entity2);

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
                bool adjacentSegments = (delta == 1 ) || (polylineIsClosedOrEndpointsAreTheSame && static_cast<unsigned int>(delta) == polylineSegmentsCount - 1);

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
            if (entity1IndexInPolyline == 0 && static_cast<unsigned int>(entity2IndexInPolyline) == polylineSegmentsCount - 1 && polylineSegmentsCount > 2) {
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
        if (const auto* native = dynamic_cast<const LC_OffsetCurve*>(e)) {
            auto plain = native->toPlainSpline();
            ctx += plain.release();
            ctx -= e;
            continue;
        }
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
                    if (containerType == RS2::EntityInsert) {
                        // FlagVisible on an insert's expansion children is derived
                        // from the frozen-layer/frozen-block state at the time the
                        // insert was last updated (see hasEffectiveInsertVisibility()
                        // in rs_insert.cpp), not the child's own code-60 visibility.
                        // Exploding must not freeze that derived snapshot in: the
                        // clone becomes a top-level entity with no insert left to
                        // recompute it, so a frozen layer at explode time would
                        // otherwise leave it permanently invisible.
                        clone->setVisible(true);
                    }
                    // FlagHatchChild marks a pattern line's role inside its parent
                    // hatch (see RS_Hatch::updatePatternHatch()); it does not
                    // describe the exploded, now-independent copy.
                    clone->delFlag(RS2::FlagHatchChild);
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
