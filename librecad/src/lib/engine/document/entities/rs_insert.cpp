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

#include "rs_insert.h"
#include "lc_insert_transform.h"
#include "lc_linemath.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_color.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_pen.h"

namespace {

const QString& layerZeroName() {
    static const QString name = QStringLiteral("0");
    return name;
}

// Cheap, unvalidated name check -- only safe for a layer pointer known to be
// live (e.g. straight from RS_Entity::getLayer() on a real, still-owned
// entity). NOT a substitute for RS_Entity::layerNameEquals(), which validates
// the pointer first: block clones can retain dangling layer pointers (see
// RS_Entity::validatedLayer's doc comment), and calling this on one would be
// a use-after-free. Named distinctly from the RS_Entity member of a similar
// name so an unqualified call from inside an RS_Insert member function can't
// silently resolve to the wrong one via C++ name lookup.
bool layerIsNamed(RS_Layer *layer, const QString &name) {
    return layer != nullptr && layer->getName().compare(name, Qt::CaseInsensitive) == 0;
}

bool usesInheritedLayer(const RS_Entity& entity) {
    return layerIsNamed(entity.getLayer(), layerZeroName());
}

// Expanded INSERTs are flattened, so the derived child must retain the
// visibility contributed by every block-reference ancestor. Layer 0 is the
// exception: within a block definition it inherits that ancestor's layer.
bool hasEffectiveInsertVisibility(const RS_Entity& entity, bool inheritedVisibility) {
    if (!inheritedVisibility || entity.isDeleted()
        || !entity.getFlag(RS2::FlagVisible)) {
        return false;
    }
    if (usesInheritedLayer(entity))
        return true;

    // getLayer() validates imported layer pointers once an entity belongs to a
    // graphic. Avoid RS_Entity::isVisible() here because its explicit-layer
    // fast path intentionally assumes a live pointer.
    const RS_Layer* layer = entity.getLayer();
    return layer == nullptr || !layer->isFrozen();
}

enum class InsertTransformCapability {
    NativeOrthogonal,
    CircleToEllipse,
    ArcToEllipse,
    NestedInsert,
    Unsupported
};

InsertTransformCapability transformCapability(const RS_Entity& entity) {
    switch (entity.rtti()) {
    case RS2::EntityCircle:
        return InsertTransformCapability::CircleToEllipse;
    case RS2::EntityArc:
        return InsertTransformCapability::ArcToEllipse;
    case RS2::EntityInsert:
        return InsertTransformCapability::NestedInsert;
    // These entity families currently own a two-dimensional scale/rotate/move
    // implementation. Each remains explicitly listed so a new EntityType is
    // rejected until its INSERT-transform semantics are reviewed.
    case RS2::EntityPoint:
    case RS2::EntityLine:
    case RS2::EntityPolyline:
    case RS2::EntityEllipse:
    case RS2::EntityHyperbola:
    case RS2::EntitySolid:
    case RS2::EntityConstructionLine:
    case RS2::EntityMText:
    case RS2::EntityText:
    case RS2::EntityDimAligned:
    case RS2::EntityDimLinear:
    case RS2::EntityDimRadial:
    case RS2::EntityDimDiametric:
    case RS2::EntityDimAngular:
    case RS2::EntityDimArc:
    case RS2::EntityDimOrdinate:
    case RS2::EntityTolerance:
    case RS2::EntityDimLeader:
    case RS2::EntityHatch:
    case RS2::EntityImage:
    case RS2::EntityWipeout:
    case RS2::EntityMLeader:
    case RS2::EntitySpline:
    case RS2::EntitySplinePoints:
    case RS2::EntityParabola:
        return InsertTransformCapability::NativeOrthogonal;
    case RS2::EntityUnknown:
    case RS2::EntityContainer:
    case RS2::EntityBlock:
    case RS2::EntityFontChar:
    case RS2::EntityGraphic:
    case RS2::EntityVertex:
    case RS2::EntityOverlayBox:
    case RS2::EntityPreview:
    case RS2::EntityPattern:
    case RS2::EntityOverlayLine:
    case RS2::EntityRefPoint:
    case RS2::EntityRefLine:
    case RS2::EntityRefConstructionLine:
    case RS2::EntityRefArc:
    case RS2::EntityRefCircle:
    case RS2::EntityRefEllipse:
    case RS2::EntitySnapMark:
    case RS2::EntitySnapLine:
    case RS2::EntitySnapArc:
    case RS2::EntitySnapCircle:
    case RS2::EntitySnapConstructionLine:
    case RS2::EntityDimArrowBlock:
        return InsertTransformCapability::Unsupported;
    }
    return InsertTransformCapability::Unsupported;
}

bool isUniformScale(const LC_InsertTransformParts& parts) {
    const double tolerance = std::numeric_limits<double>::epsilon()
                             * std::max({LC_InsertTransform::IdentityScale,
                                         std::abs(parts.scaleX),
                                         std::abs(parts.scaleY)});
    return std::abs(std::abs(parts.scaleX) - std::abs(parts.scaleY)) <= tolerance;
}

std::unique_ptr<RS_Entity> cloneForTransform(const RS_Entity& source,
                                              const LC_InsertTransformParts& parts) {
    switch (transformCapability(source)) {
    case InsertTransformCapability::CircleToEllipse:
        if (!isUniformScale(parts)) {
            const auto& circle = static_cast<const RS_Circle&>(source);
            return std::make_unique<RS_Ellipse>(nullptr,
                RS_EllipseData{circle.getCenter(),
                               RS_Vector(circle.getRadius(), LC_InsertTransform::Zero),
                               LC_InsertTransform::IdentityScale,
                               LC_InsertTransform::Zero,
                               lcInsertTransformFullTurnRadians(), false});
        }
        break;
    case InsertTransformCapability::ArcToEllipse:
        if (!isUniformScale(parts)) {
            const auto& arc = static_cast<const RS_Arc&>(source);
            return std::make_unique<RS_Ellipse>(nullptr,
                RS_EllipseData{arc.getCenter(),
                               RS_Vector(arc.getRadius(), LC_InsertTransform::Zero),
                               LC_InsertTransform::IdentityScale,
                               arc.getAngle1(), arc.getAngle2(),
                               arc.isReversed()});
        }
        break;
    case InsertTransformCapability::NestedInsert:
    case InsertTransformCapability::Unsupported:
        return nullptr;
    case InsertTransformCapability::NativeOrthogonal:
        break;
    }
    return std::unique_ptr<RS_Entity>(source.clone());
}

void copyExpansionProperties(const RS_Entity& source, RS_Entity& target) {
    target.setPen(source.getPen(false));
    target.setLayer(source.getLayer());
    target.setVisible(source.getFlag(RS2::FlagVisible));
}

class ScopedEntityUpdateState final {
public:
    explicit ScopedEntityUpdateState(RS_Entity& entity)
        : m_entity(entity), m_previous(entity.isUpdateEnabled()) {
        m_entity.setUpdateEnabled(false);
    }

    ~ScopedEntityUpdateState() {
        m_entity.setUpdateEnabled(m_previous);
    }

private:
    RS_Entity& m_entity;
    bool m_previous;
};

void applyTransform(RS_Entity& entity, const LC_InsertTransform& transform,
                    const LC_InsertTransformParts& parts) {
    ScopedEntityUpdateState updateState(entity);
    // The decomposition carries the determinant sign in sy.  Route that
    // reflection through the entity's mirror contract rather than hoping each
    // scale() implementation updates directional state (arc sweeps, hatch
    // angles, text alignment, image frames, and similar metadata) for a
    // negative component scale.
    entity.scale(lcInsertTransformOrigin(),
                 RS_Vector(parts.scaleX, std::abs(parts.scaleY)));
    if (parts.reversesOrientation)
        entity.mirror(lcInsertTransformOrigin(), lcInsertTransformReflectionAxisPoint());
    entity.rotate(lcInsertTransformOrigin(), parts.angle);
    entity.move(RS_Vector(transform.tx, transform.ty));
}

// update the entity pen according to the blockPen
RS_Pen updatePen(RS_Pen pen, const RS_Pen& blockPen) {
    // color from block (free floating):
    if (pen.getColor() == RS_Color(RS2::FlagByBlock)) {
        pen.setColor(blockPen.getColor());
    }

    // line width from block (free floating):
    if (pen.getWidth() == RS2::WidthByBlock) {
        pen.setWidth(blockPen.getWidth());
    }

    // line type from block (free floating):
    if (pen.getLineType() == RS2::LineByBlock) {
        pen.setLineType(blockPen.getLineType());
    }

    return pen;
}

enum class LC_InsertExpansionWorkKind {
    ExpandArrayCell,
    VisitBlock,
    ProcessEntity
};

// The stack models recursive INSERT expansion without consuming the C++ call
// stack.  A VisitBlock item resumes at one source entity at a time, keeping
// auxiliary memory proportional to nesting rather than block entity count.
struct LC_InsertExpansionWork {
    LC_InsertExpansionWorkKind kind = LC_InsertExpansionWorkKind::ExpandArrayCell;
    const RS_Block* block = nullptr;
    const RS_Entity* entity = nullptr;
    LC_InsertTransform transform;
    RS_Pen blockPen;
    RS_Layer* inheritedLayer = nullptr;
    bool inheritedVisibility = true;
    RS_InsertData insertData;
    int column = 0;
    int row = 0;
    std::size_t entityIndex = 0U;
};

}
RS_InsertData::RS_InsertData(const QString& _name,
							 RS_Vector _insertionPoint,
							 RS_Vector _scaleFactor,
							 double _angle,
							 int _cols, int _rows, RS_Vector _spacing,
							 RS_BlockList* _blockSource ,
							 RS2::UpdateMode _updateMode ):
	name(_name)
  ,insertionPoint(_insertionPoint)
  // LibreCAD is 2D, so callers routinely build the scale from a two component
  // RS_Vector, whose z defaults to 0. See usableScale().
  ,scaleFactor(usableScale(_scaleFactor))
  ,angle(_angle)
  ,cols(_cols)
  ,rows(_rows)
  ,spacing(_spacing)
  ,blockSource(_blockSource)
  ,updateMode(_updateMode){
}

double RS_InsertData::usableScale(const double factor) {
    return std::isfinite(factor) && LC_LineMath::isMeaningful(factor) ? factor : 1.0;
}

RS_Vector RS_InsertData::usableScale(const RS_Vector& scale) {
    return {usableScale(scale.x), usableScale(scale.y), usableScale(scale.z)};
}

RS_InsertData::RS_InsertData(const RS_InsertData &other):
   name(other.name)
  ,insertionPoint(other.insertionPoint)
  ,scaleFactor(other.scaleFactor)
  ,extrusion(other.extrusion)
  ,angle(other.angle)
  ,cols(other.cols)
  ,rows(other.rows)
  ,spacing(other.spacing)
  ,blockSource(other.blockSource)
  ,updateMode(other.updateMode){
}

std::ostream& operator <<(std::ostream& os,
                          const RS_InsertData& d) {
    os << "(" << d.name.toLatin1().data() << ")";
    return os;
}

/**
 * @param parent The graphic this m_block belongs to.
 */
RS_Insert::RS_Insert(RS_EntityContainer* parent,
                     const RS_InsertData& d)
    : RS_EntityContainer(parent)
      , m_data(d) {
    if (m_data.updateMode != RS2::NoUpdate) {
        RS_Insert::update();
        //calculateBorders();
    }
}

RS_Entity* RS_Insert::clone() const{
	auto i = new RS_Insert(*this);
	i->setOwner(isOwner());
	i->detach();
	return i;
}

/**
 * Updates the entity buffer of this insert entity. This method
 * needs to be called whenever the block this insert is based on changes.
 */
void RS_Insert::calculateBorders() {
    RS_EntityContainer::calculateBorders();
    if (count() == 0) {
        m_minV = RS_Vector(false);
        m_maxV = RS_Vector(false);
    }
}

void RS_Insert::update() {
    update(RS_InsertExpansionBudget{});
}

void RS_Insert::update(const RS_InsertExpansionBudget& budget) {
    RS_DEBUG->print("RS_Insert::update");
    RS_DEBUG->print("RS_Insert::update: name: %s", m_data.name.toLatin1().data());
    if (!m_updateEnabled) {
        return;
    }
    RS_Block* blk = getBlockForInsert();
    if (blk == nullptr) {
        RS_DEBUG->print("RS_Insert::update: Block is nullptr");
        clear();
        calculateBorders();
        return;
    }
    if (isDeleted()) {
        RS_DEBUG->print("RS_Insert::update: Insert is in undo list");
        clear();
        calculateBorders();
        return;
    }
    const bool fontLetterInsert = blk->rtti() == RS2::EntityFontChar
                                  || m_data.blockSource != nullptr;
    RS_Pen expansionPen = fontLetterInsert ? getPen(false) : getPen(true);
    if (fontLetterInsert && !expansionPen.isValid() && m_parent != nullptr)
        expansionPen = m_parent->getPen(false);

    std::vector<std::unique_ptr<RS_Entity>> expanded;
    std::vector<const RS_Block*> activeBlocks;
    std::vector<LC_InsertExpansionWork> work;
    const auto arrayFitsBudget = [&budget](const RS_InsertData& data) {
        if (data.cols <= 0 || data.rows <= 0)
            return false;
        const auto columns = static_cast<std::size_t>(data.cols);
        const auto rows = static_cast<std::size_t>(data.rows);
        // Check before multiplication so an untrusted MINSERT grid cannot
        // overflow and bypass the traversal limit.
        return columns <= budget.maxArrayInstances / rows;
    };
    const auto appendLeaf = [&](const RS_Entity& source,
                                const LC_InsertTransform& transform,
                                const RS_Pen& blockPen,
                                RS_Layer* inheritedLayer,
                                bool inheritedVisibility) -> bool {
        if (expanded.size() >= budget.maxDerivedEntities)
            return false;
        LC_InsertTransformParts parts;
        const auto decompositionStatus = transform.decompose(parts);
        if (decompositionStatus != LC_InsertTransformDecompositionStatus::Ok) {
            RS_DEBUG->print(RS_Debug::D_ERROR,
                            "RS_Insert::update: derived transform rejected: %s",
                            lcInsertTransformDecompositionStatusName(decompositionStatus));
            return false;
        }
        auto clone = cloneForTransform(source, parts);
        if (clone)
            copyExpansionProperties(source, *clone);
        if (!clone) {
            RS_DEBUG->print(RS_Debug::D_ERROR,
                            "RS_Insert::update: unsupported derived entity transform");
            return false;
        }
        applyTransform(*clone, transform, parts);
        RS_Layer* layer = clone->getLayer();
        // Validate once and reuse -- layer can be a dangling pointer left
        // over from the block clone (see RS_Entity::validatedLayer), so this
        // check cannot be skipped or replaced with the cheap layerIsNamed().
        RS_Layer* validated = layer != nullptr ? this->validatedLayer(layer) : nullptr;
        if (validated != nullptr && layerIsNamed(validated, layerZeroName()))
            clone->setLayer(inheritedLayer);
        else if (layer != nullptr && validated == nullptr)
            clone->setLayer(nullptr);
        clone->setParent(this);
        clone->setVisible(hasEffectiveInsertVisibility(source, inheritedVisibility));
        clone->setSelectionFlag(false);
        clone->setPen(updatePen(clone->getPen(false), blockPen));
        if (m_data.updateMode != RS2::PreviewUpdate)
            clone->update();
        expanded.push_back(std::move(clone));
        return true;
    };

    const auto queueArrayCell = [&](const RS_Block& sourceBlock,
                                    const RS_InsertData& data,
                                    const LC_InsertTransform& parentTransform,
                                    const RS_Pen& blockPen,
                                    RS_Layer* inheritedLayer,
                                    bool inheritedVisibility,
                                    int column, int row) {
        LC_InsertExpansionWork item;
        item.kind = LC_InsertExpansionWorkKind::ExpandArrayCell;
        item.block = &sourceBlock;
        item.transform = parentTransform;
        item.blockPen = blockPen;
        item.inheritedLayer = inheritedLayer;
        item.inheritedVisibility = inheritedVisibility;
        item.insertData = data;
        item.column = column;
        item.row = row;
        work.push_back(std::move(item));
    };

    std::size_t arrayInstances = 0U;
    bool success = budget.isValid() && arrayFitsBudget(m_data);
    if (success) {
        queueArrayCell(*blk, m_data, LC_InsertTransform{}, expansionPen, getLayer(),
                       isVisible(), 0, 0);
    }
    while (success && !work.empty()) {
        LC_InsertExpansionWork item = std::move(work.back());
        work.pop_back();

        switch (item.kind) {
        case LC_InsertExpansionWorkKind::ExpandArrayCell: {
            LC_InsertTransform child;
            if (item.block == nullptr || !arrayFitsBudget(item.insertData)
                || item.column < 0 || item.row < 0
                || item.column >= item.insertData.cols || item.row >= item.insertData.rows) {
                success = false;
                break;
            }
            if (arrayInstances >= budget.maxArrayInstances) {
                RS_DEBUG->print(RS_Debug::D_ERROR,
                                "RS_Insert::update: array instance limit exceeded");
                success = false;
                break;
            }
            ++arrayInstances;
            const LC_InsertTransformStatus transformStatus =
                LC_InsertTransform::fromInsert(item.insertData, item.block->getBasePoint(),
                                                item.column, item.row, child);
            if (transformStatus != LC_InsertTransformStatus::Ok) {
                RS_DEBUG->print(RS_Debug::D_ERROR,
                                "RS_Insert::update: transform rejected: %s",
                                lcInsertTransformStatusName(transformStatus));
                success = false;
                break;
            }

            int nextColumn = item.column;
            int nextRow = item.row + 1;
            if (nextRow >= item.insertData.rows) {
                nextRow = 0;
                ++nextColumn;
            }
            if (nextColumn < item.insertData.cols) {
                queueArrayCell(*item.block, item.insertData, item.transform,
                               item.blockPen, item.inheritedLayer,
                               item.inheritedVisibility, nextColumn, nextRow);
            }

            LC_InsertExpansionWork enterBlock;
            enterBlock.kind = LC_InsertExpansionWorkKind::VisitBlock;
            enterBlock.block = item.block;
            if (!LC_InsertTransform::compose(item.transform, child,
                                              enterBlock.transform)) {
                RS_DEBUG->print(RS_Debug::D_ERROR,
                                "RS_Insert::update: transform composition overflow");
                success = false;
                break;
            }
            enterBlock.blockPen = item.blockPen;
            enterBlock.inheritedLayer = item.inheritedLayer;
            enterBlock.inheritedVisibility = item.inheritedVisibility;
            work.push_back(std::move(enterBlock));
            break;
        }
        case LC_InsertExpansionWorkKind::VisitBlock: {
            if (item.block == nullptr) {
                success = false;
                break;
            }
            if (item.entityIndex == 0) {
                if (std::find(activeBlocks.cbegin(), activeBlocks.cend(), item.block)
                    != activeBlocks.cend()) {
                    RS_DEBUG->print(RS_Debug::D_ERROR,
                                    "RS_Insert::update: recursive block reference rejected");
                    success = false;
                    break;
                }
                if (activeBlocks.size() >= budget.maxNestingDepth) {
                    RS_DEBUG->print(RS_Debug::D_ERROR,
                                    "RS_Insert::update: nesting depth exceeds expansion limit");
                    success = false;
                    break;
                }
                activeBlocks.push_back(item.block);
            }
            const std::size_t entityCount = item.block->count();
            if (item.entityIndex >= entityCount) {
                if (activeBlocks.empty() || activeBlocks.back() != item.block) {
                    success = false;
                } else {
                    activeBlocks.pop_back();
                }
                break;
            }

            LC_InsertExpansionWork continueBlock = item;
            ++continueBlock.entityIndex;
            work.push_back(std::move(continueBlock));

            LC_InsertExpansionWork processEntity;
            processEntity.kind = LC_InsertExpansionWorkKind::ProcessEntity;
            if (item.entityIndex > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
                success = false;
                break;
            }
            processEntity.entity = item.block->entityAt(static_cast<int>(item.entityIndex));
            processEntity.transform = item.transform;
            processEntity.blockPen = item.blockPen;
            processEntity.inheritedLayer = item.inheritedLayer;
            processEntity.inheritedVisibility = item.inheritedVisibility;
            work.push_back(std::move(processEntity));
            break;
        }
        case LC_InsertExpansionWorkKind::ProcessEntity:
            if (item.entity == nullptr || item.entity->isDeleted())
                break;
            if (item.entity->rtti() != RS2::EntityInsert) {
                success = appendLeaf(*item.entity, item.transform, item.blockPen,
                                     item.inheritedLayer, item.inheritedVisibility);
                break;
            }

            {
                const auto& nested = static_cast<const RS_Insert&>(*item.entity);
                RS_Block* nestedBlock = nested.getBlockForInsert();
                const RS_InsertData nestedData = nested.getData();
                if (nestedBlock == nullptr || !arrayFitsBudget(nestedData)) {
                    success = false;
                    break;
                }
                RS_Layer* nestedLayer = nested.getLayer();
                // Validating member call (nestedLayer can be dangling, as in
                // appendLeaf above) -- kept explicit so it can't be mistaken
                // for the cheap free-function layerIsNamed().
                if (this->layerNameEquals(nestedLayer, layerZeroName()))
                    nestedLayer = item.inheritedLayer;
                const bool nestedVisibility =
                    hasEffectiveInsertVisibility(nested, item.inheritedVisibility)
                    && !nestedBlock->isFrozen();
                queueArrayCell(*nestedBlock, nestedData, item.transform,
                               updatePen(nested.getPen(false), item.blockPen), nestedLayer,
                               nestedVisibility, 0, 0);
            }
            break;
        }
    }

    // Commit only a fully expanded tree.  Failed cycles, unsupported non-planar
    // OCS, and nonrepresentable shear leave this INSERT empty, never partial.
    clear();
    if (!success) {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "RS_Insert::update: expansion rejected without partial children");
        calculateBorders();
        return;
    }
    const bool oldAutoUpdateBorders = getAutoUpdateBorders();
    setAutoUpdateBorders(false);
    for (auto& entity : expanded)
        appendEntity(entity.release());
    setAutoUpdateBorders(oldAutoUpdateBorders);
    calculateBorders();
    RS_DEBUG->print("RS_Insert::update: OK");
}

/**
 * @return Pointer to the m_block associated with this Insert or
 *   nullptr if the m_block couldn't be found. Blocks are requested
 *   from the blockSource if one was supplied and otherwise from
 *   the closest parent graphic.
 */
RS_Block* RS_Insert::getBlockForInsert() const{
    RS_BlockList* blkList = nullptr;

    if (!m_data.blockSource) {
        if (getGraphic()) {
            blkList = getGraphic()->getBlockList();
        }
    } else {
        blkList = m_data.blockSource;
    }

    if (blkList == nullptr) {
        invalidateBlockCache();
        return nullptr;
    }
    if (m_blockList == blkList && m_blockListGeneration == blkList->generation()) {
        return m_block;
    }
    m_block = blkList->find(m_data.name);
    m_blockList = blkList;
    m_blockListGeneration = blkList->generation();
    return m_block;
}

/**
 * Is this insert visible? (re-implementation from RS_Entity)
 *
 * @return true Only if the entity and the block and the layer it is on
 * are visible.
 * The Layer might also be nullptr. In that case the layer visibility
 * is ignored.
 * The Block might also be nullptr. In that case the block visibility
 * is ignored.
 */
bool RS_Insert::isVisible() const{
    RS_Block* blk = getBlockForInsert();
    if (blk != nullptr) {
        if (blk->isFrozen()) {
            return false;
        }
    }

    return RS_Entity::isVisible();
}

RS_VectorSolutions RS_Insert::getRefPoints() const{
    return RS_VectorSolutions{m_data.insertionPoint};
}

RS_Vector RS_Insert::doGetNearestRef(const RS_Vector& coord, double* dist) const {
    return getRefPoints().getClosest(coord, dist);
}

void RS_Insert::move(const RS_Vector& offset) {
    LC_InsertTransform edit;
    if (LC_InsertTransform::translation(offset, edit))
        applySourceEdit(edit, "move");
    else {
        setLastSourceEditStatus(LC_InsertSourceEditStatus::InvalidEdit);
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Insert::move: invalid transform");
    }
}

void RS_Insert::rotate(const RS_Vector& center, double angle) {
    LC_InsertTransform edit;
    if (LC_InsertTransform::rotation(center, angle, edit))
        applySourceEdit(edit, "rotate");
    else {
        setLastSourceEditStatus(LC_InsertSourceEditStatus::InvalidEdit);
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Insert::rotate: invalid transform");
    }
}

void RS_Insert::rotate(const RS_Vector& center, const RS_Vector& angleVector) {
    if (!angleVector.valid || !std::isfinite(angleVector.x)
        || !std::isfinite(angleVector.y)
        || (angleVector.x == LC_InsertTransform::Zero
            && angleVector.y == LC_InsertTransform::Zero)) {
        setLastSourceEditStatus(LC_InsertSourceEditStatus::InvalidEdit);
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "RS_Insert::rotate: invalid angle vector");
        return;
    }
    rotate(center, angleVector.angle());
}

void RS_Insert::scale(const RS_Vector& center, const RS_Vector& factor) {
    LC_InsertTransform edit;
    if (LC_InsertTransform::scale(center, factor, edit))
        applySourceEdit(edit, "scale");
    else {
        setLastSourceEditStatus(LC_InsertSourceEditStatus::InvalidEdit);
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Insert::scale: invalid transform");
    }
}

void RS_Insert::mirror(const RS_Vector& axisPoint1, const RS_Vector& axisPoint2) {
    LC_InsertTransform edit;
    if (LC_InsertTransform::reflection(axisPoint1, axisPoint2, edit))
        applySourceEdit(edit, "mirror");
    else {
        setLastSourceEditStatus(LC_InsertSourceEditStatus::InvalidEdit);
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Insert::mirror: invalid transform");
    }
}

void RS_Insert::applySourceEdit(const LC_InsertTransform& edit, const char* operation) {
    RS_InsertData transformed;
    const auto status = lcApplyInsertSourceEdit(m_data, edit, transformed);
    if (status != LC_InsertSourceEditStatus::Ok) {
        setLastSourceEditStatus(status);
        RS_DEBUG->print(RS_Debug::D_ERROR, "RS_Insert::%s: source transform rejected: %s",
                        operation, lcInsertSourceEditStatusName(status));
        return;
    }
    m_data = std::move(transformed);
    setLastSourceEditStatus(LC_InsertSourceEditStatus::Ok);
    update();
}

std::ostream& operator << (std::ostream& os, const RS_Insert& i) {
    os << " Insert: " << i.getData() << std::endl;
    return os;
}
