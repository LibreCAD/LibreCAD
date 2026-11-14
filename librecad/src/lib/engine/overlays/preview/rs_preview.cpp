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

#include "rs_preview.h"

#include <array>

#include "lc_graphicviewport.h"
#include "lc_offsetcurvetransform.h"
#include "lc_offsetoutputbudget.h"
#include "rs_color.h"
#include "rs_debug.h"
#include "rs_line.h"
#include "rs_pen.h"
#include "rs_settings.h"

namespace {
enum class PreviewRoute { Ordinary, Native, Refuse };

PreviewRoute previewRoute(const RS_Entity& root) {
    struct Frame { const RS_EntityContainer* container; std::size_t next; };
    std::array<Frame, 256> parents{};
    std::size_t depth = 0, visits = 0;
    bool native = false;
    const RS_Entity* entity = &root;
    for (;;) {
        if (++visits > kDefaultOffsetDeepEntitiesPerSource) return PreviewRoute::Refuse;
        if (entity && entity->rtti() == RS2::EntityOffsetCurve) {
            const auto* cache = static_cast<const RS_EntityContainer*>(entity);
            if (cache->count() > kDefaultOffsetDeepEntitiesPerSource - visits) return PreviewRoute::Refuse;
            visits += cache->count();
            native = true; // typed preparation validates its data and children
        } else if (entity && RS2::isTextEntity(entity->rtti())) {
            // Text/MText own generated glyph caches. Their legacy proxies use
            // baselines, not user geometry; do not walk the glyph expansion.
        } else if (const auto* container = dynamic_cast<const RS_EntityContainer*>(entity); container && !container->empty()) {
            if (depth == parents.size() || container->count() > kDefaultOffsetDeepEntitiesPerSource - visits)
                return PreviewRoute::Refuse;
            parents[depth++] = {container, 1};
            entity = container->first();
            continue;
        }
        while (depth && parents[depth - 1].next == parents[depth - 1].container->count()) --depth;
        if (!depth) return native ? PreviewRoute::Native : PreviewRoute::Ordinary;
        auto& parent = parents[depth - 1];
        entity = parent.container->entityAt(parent.next++);
    }
}

std::unique_ptr<RS_Entity> nativePreviewCopy(const RS_Entity& entity) {
    if (entity.rtti() == RS2::EntityOffsetCurve || entity.rtti() == RS2::EntityContainer) {
        auto prepared = LC_OffsetCurveTransform::prepare(entity, {}, true);
        if (prepared) return std::move(prepared.entity);
    }
    RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Preview: Unsupported native preview tree");
    return {};
}

std::unique_ptr<RS_Entity> previewCopy(const RS_Entity& entity) {
    if (entity.isDeleted()) return {};
    switch (previewRoute(entity)) {
    case PreviewRoute::Ordinary:
        return std::unique_ptr<RS_Entity>{entity.cloneProxy()};
    case PreviewRoute::Native:
        return nativePreviewCopy(entity);
    case PreviewRoute::Refuse:
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Preview: Native routing scan limit reached");
        return {};
    }
    return {};
}

std::unique_ptr<RS_Entity> admittedPreviewEntity(const RS_Entity* entity) {
    if (entity->isDeleted()) {
        delete entity;
        return {};
    }
    const auto route = previewRoute(*entity);
    if (route == PreviewRoute::Ordinary)
        return std::unique_ptr<RS_Entity>{const_cast<RS_Entity*>(entity)};
    auto copy = route == PreviewRoute::Native ? nativePreviewCopy(*entity) : nullptr;
    delete entity;
    if (route == PreviewRoute::Refuse)
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Preview: Native routing scan limit reached");
    return copy;
}
}

/**
 * Constructor.
 */
RS_Preview::RS_Preview(RS_EntityContainer* parent, LC_GraphicViewport* viewport)
        : LC_PreviewDocument(parent), m_viewport{viewport}{

// fixme - sand - ucs - check when preview is created and whether this may be delegated to actio init?

    m_maxEntities = LC_GET_ONE_INT("Appearance", "MaxPreview", 100);
    const auto highLight = RS_Color(LC_GET_ONE_STR("Colors", "highlight", RS_Settings::HIGHLIGHT));
    setPen(RS_Pen(highLight, RS2::Width00, RS2::SolidLine));
}

/**
 * Adds an entity to this preview and removes any attributes / layer
 * connections before that.
 */
void RS_Preview::addEntity(const RS_Entity* entity) {
    if (entity == nullptr) {
        return;
    }

    auto admitted = admittedPreviewEntity(entity);
    if (!admitted) return;
    entity = admitted.release();

    // only border preview for complex entities:

    const int rtti = entity->rtti();

    bool addBorder = false;
    bool refEntity = false;

    switch (rtti) {
        //        case RS2::EntityImage:
        case RS2::EntityHatch:
            addBorder = true;
            break;
        /*case RS2::EntityInsert: {
            addBorder = true;
            break;
        }*/
        case RS2::EntityRefPoint:
        case RS2::EntitySnapMark:
        case RS2::EntityRefLine:
        case RS2::EntitySnapLine:
        case RS2::EntitySnapCircle:
        case RS2::EntitySnapArc:
        case RS2::EntityRefConstructionLine:
        case RS2::EntitySnapConstructionLine:
        case RS2::EntityRefCircle:
        case RS2::EntityRefEllipse:
        case RS2::EntityRefArc: {
            refEntity = true;
            break;
        }
        // todo - sand - hm... spline and texts are not included into limit of preview entities?
        case RS2::EntityPolyline:
            break;
        case RS2::EntitySpline:
            break;
        case RS2::EntityMText:
            break;
        case RS2::EntityText:
            break;
        case RS2::EntityPoint:
            break;
        default: {
            if (entity->isContainer()) {
                if (entity->countDeep() > m_maxEntities - countDeep()) {
                    addBorder = true;
                }
            }
        }
    }

    if (addBorder) {
        RS_Line *l1, *l2, *l3, *l4;

        RS_Vector min = entity->getMin();
        RS_Vector max = entity->getMax();

        if (m_viewport->hasUCS()) {
            RS_Vector c2, c4;
            calcRectCorners(min, max, c2, c4);
            l1 = new RS_Line(this, {min.x, min.y}, {c2.x, c2.y});
            l2 = new RS_Line(this, {c2.x, c2.y}, {max.x, max.y});
            l3 = new RS_Line(this, {max.x, max.y}, {c4.x, c4.y});
            l4 = new RS_Line(this, {c4.x, c4.y}, {min.x, min.y});
        }
        else {
            l1 = new RS_Line(this, {min.x, min.y}, {max.x, min.y});
            l2 = new RS_Line(this, {max.x, min.y}, {max.x, max.y});
            l3 = new RS_Line(this, {max.x, max.y}, {min.x, max.y});
            l4 = new RS_Line(this, {min.x, max.y}, {min.x, min.y});
        }

        RS_EntityContainer::addEntity(l1);
        RS_EntityContainer::addEntity(l2);
        RS_EntityContainer::addEntity(l3);
        RS_EntityContainer::addEntity(l4);

        delete entity;
    }
    else {
        const auto ent = const_cast<RS_Entity*>(entity);
        ent->setLayer(nullptr);
        ent->clearSelectionFlag();
        ent->reparent(this);
        // Don't set this pen, let drawing routines decide entity->setPenToActive();
        if (refEntity) {
            m_referenceEntities.append(ent);
            if (getAutoUpdateBorders()) {
                adjustBorders(entity);
            }
        }
        else {
            RS_EntityContainer::addEntity(entity);
        }
    }
}

void RS_Preview::calcRectCorners(const RS_Vector &worldCorner1, const RS_Vector &worldCorner3, RS_Vector &worldCorner2, RS_Vector &worldCorner4) const {
    const RS_Vector ucsCorner1 = m_viewport->toUCS(worldCorner1);
    const RS_Vector ucsCorner3 = m_viewport->toUCS(worldCorner3);
    const auto ucsCorner2 = RS_Vector(ucsCorner1.x, ucsCorner3.y);
    const auto ucsCorner4 = RS_Vector(ucsCorner3.x, ucsCorner1.y);
    worldCorner2 =  m_viewport->toWorld(ucsCorner2);
    worldCorner4 =  m_viewport->toWorld(ucsCorner4);
}

void RS_Preview::clear() {
    if (isOwner()) {
        while (!m_referenceEntities.isEmpty()) {
            delete m_referenceEntities.takeFirst();
        }
    } else {
        m_referenceEntities.clear();
    }
    RS_EntityContainer::clear();
}


/**
 * Adds all entities from 'container' to the preview (unselected).
 */
void RS_Preview::addAllFrom(RS_EntityContainer& container, [[maybe_unused]]LC_GraphicViewport* view) {
    unsigned int c=0;
    for(const auto e: container){
        if (c > m_maxEntities) {
            break;
        }
        auto copy = previewCopy(*e);
        if (!copy) {
            continue;
        }
        RS_Entity* clone = copy.release();
        clone->clearSelectionFlag();
        clone->reparent(this);

        c += clone->countDeep();
        addEntity(clone);
        // clone might be nullptr after this point
    }
}

void RS_Preview::addClonesFromList(const QList<RS_Entity*>& list) {
    unsigned int c = 0;
    for (const auto e: list) {
        if (c > m_maxEntities) {
            break;
        }
        if (e != nullptr) {
            auto copy = previewCopy(*e);
            if (!copy) {
                continue;
            }
            const RS_Entity* clone = copy.release();
            c += clone->countDeep();
            addEntity(clone);
        }
    }
}

void RS_Preview::addAllFromList(const QList<RS_Entity*>& list) {
    unsigned int c = 0;
    for (const auto e: list) {
        if (e == nullptr) {
            continue;
        }
        if (c > m_maxEntities) {
            delete e;                       // adopted or released, never dropped
            continue;
        }
        auto admitted = admittedPreviewEntity(e);
        if (!admitted) continue;
        c += admitted->countDeep();
        addEntity(admitted.release());
    }
}

void RS_Preview::addAllFromList(const std::list<RS_Entity*>& list) {
    unsigned int c = 0;
    for (const auto e: list) {
        if (e == nullptr) {
            continue;
        }
        if (c > m_maxEntities) {
            delete e;
            continue;
        }
        auto admitted = admittedPreviewEntity(e);
        if (!admitted) continue;
        c += admitted->countDeep();
        addEntity(admitted.release());
    }
}

/**
 * Adds all entities in the given range and those which have endpoints
 * in the given range to the preview.
 */
void RS_Preview::addStretchablesFrom(RS_EntityContainer& container, [[maybe_unused]]LC_GraphicViewport* view,
                                     const RS_Vector& v1, const RS_Vector& v2) {
    unsigned int c=0;

    for (const auto e: container) {
        if (!e || e->rtti() == RS2::EntityHatch || c >= m_maxEntities) continue;
        const auto route = previewRoute(*e);
        if (route == PreviewRoute::Refuse) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Preview: Native routing scan limit reached");
            continue;
        }
        if (!e->isVisible()) continue;
        auto copy = route == PreviewRoute::Native ? nativePreviewCopy(*e) : nullptr;
        if (route == PreviewRoute::Native && !copy) continue;
        const auto* candidate = copy ? copy.get() : e;
        if (candidate->isInWindow(v1, v2) || candidate->hasEndpointsWithinWindow(v1, v2)) {
            if (!copy) copy.reset(e->cloneProxy());
            if (!copy) {
                continue;
            }
            RS_Entity *clone = copy.release();
            //clone->setSelected(false);
            clone->reparent(this);

            c += clone->countDeep();
            addEntity(clone);
            // clone might be nullptr after this point
        }
    }
}

void RS_Preview::draw(RS_Painter* painter) {
//    bool drawTextsAsDraftsForPreview = view->isDrawTextsAsDraftForPreview();
// fixme - ucs - achieve view - store as field? This temporary for compilation...
    const bool drawTextsAsDraftsForPreview = false;

    for (const auto e: std::as_const(*this)) {
        const int type = e->rtti();
        switch (type) {
            case RS2::EntityMText:
            case RS2::EntityText: {
                if (drawTextsAsDraftsForPreview){
                    e->drawDraft(painter);
                }
                e->draw(painter);
                break;
            }
            case RS2::EntityImage: {
                e->drawDraft(painter);
                break;
            }
            default:
                e->draw(painter);
        }
    }
}

void RS_Preview::addReferenceEntitiesToContainer(RS_EntityContainer *container){
    for (const auto en: std::as_const(m_referenceEntities)){
        container->addEntity(en);
    }
}

int RS_Preview::getMaxAllowedEntities() const {
    return m_maxEntities;
}
