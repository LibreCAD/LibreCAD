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

#include "rs_pattern.h"
#include "lc_offsetoutputbudget.h"

#include <QFileInfo>
#include <array>
#include <cmath>
#include <unordered_set>

#include "rs_debug.h"
#include "rs_fileio.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_pen.h"
#include "rs_system.h"

namespace {
enum class NativeContent { None, Present, Unknown };

NativeContent nativeContent(const RS_Entity& root, std::size_t& visits) {
    struct Frame { const RS_EntityContainer* container; std::size_t next; };
    std::array<Frame, 256> parents{};
    std::size_t depth = 0;
    const RS_Entity* entity = &root;
    for (;;) {
        if (++visits > kDefaultOffsetDeepEntitiesPerSource) return NativeContent::Unknown;
        if (entity && entity->rtti() == RS2::EntityOffsetCurve) return NativeContent::Present;
        if (const auto* container = dynamic_cast<const RS_EntityContainer*>(entity); container && !container->empty()) {
            if (depth == parents.size() || container->count() > kDefaultOffsetDeepEntitiesPerSource - visits)
                return NativeContent::Unknown;
            parents[depth++] = {container, 1};
            entity = container->first();
            continue;
        }
        while (depth && parents[depth - 1].next == parents[depth - 1].container->count()) --depth;
        if (!depth) return NativeContent::None;
        auto& parent = parents[depth - 1];
        entity = parent.container->entityAt(parent.next++);
    }
}

bool ownedTree(const RS_Entity& root, std::size_t& visits, std::unordered_set<const RS_Entity*>& seen) {
    struct Frame { const RS_EntityContainer* container; std::size_t next; };
    std::array<Frame, 256> parents{};
    std::size_t depth = 0;
    const RS_Entity* entity = &root;
    for (;;) {
        if (++visits > kDefaultOffsetDeepEntitiesPerSource || !entity || !seen.insert(entity).second) return false;
        if (const auto* container = dynamic_cast<const RS_EntityContainer*>(entity)) {
            if (!container->isOwner() || container->count() > kDefaultOffsetDeepEntitiesPerSource - visits) return false;
            if (!container->empty()) {
                if (depth == parents.size()) return false;
                parents[depth++] = {container, 1};
                entity = container->first();
                continue;
            }
        }
        while (depth && parents[depth - 1].next == parents[depth - 1].container->count()) --depth;
        if (!depth) return true;
        auto& parent = parents[depth - 1];
        entity = parent.container->entityAt(parent.next++);
    }
}

bool boundedNativeCount(const RS_EntityContainer& source) {
    std::size_t count = 0;
    for (const auto* entity : source) {
        if (entity && entity->rtti() == RS2::EntityOffsetCurve) {
            const auto edges = static_cast<const RS_EntityContainer*>(entity)->count();
            if (edges > kDefaultOffsetDeepEntitiesPerSource - count) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Pattern: Aggregate offset display cache limit reached");
                return false;
            }
            count += edges;
        }
    }
    return true;
}

bool appendOffsetCache(RS_Pattern& pattern, const RS_Entity& entity) {
    const auto& cache = static_cast<const RS_EntityContainer&>(entity);
    if (entity.isDeleted() || cache.empty() || cache.count() > kDefaultOffsetDeepEntitiesPerSource) {
        RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Pattern: Offset display cache limit reached");
        return false;
    }
    for (const auto* edge : cache) {
        if (!edge || edge->isDeleted() || edge->rtti() != RS2::EntityLine) {
            RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Pattern: Unsupported offset display cache");
            return false;
        }
        for (const auto& p : {edge->getStartpoint(), edge->getEndpoint()}) {
            if (!p.valid || !std::isfinite(p.x) || !std::isfinite(p.y) || p.z != 0) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Pattern: Non-2D offset display cache");
                return false;
            }
        }
    }
    for (const auto* edge : cache) {
        auto clone = std::unique_ptr<RS_Entity>{edge->clone()};
        clone->setPen(entity.getPen(true));
        clone->setLayer(nullptr);
        clone->reparent(&pattern);
        pattern.addEntity(clone.release());
    }
    return true;
}
}

/**
 * Constructor.
 *
 * @param fileName File name of a DXF file defining the pattern
 */
RS_Pattern::RS_Pattern(const QString& fileName)
    : RS_EntityContainer(nullptr), m_fileName(fileName) {
    RS_DEBUG->print("RS_Pattern::RS_Pattern() ");
}

/**
 * Clone
 *
 * @author{Dongxu Li}
 */
RS_Entity* RS_Pattern::clone() const {
    auto cloned = std::make_unique<RS_Pattern>(m_fileName);
    cloned->m_loaded = m_loaded;
    if (m_loaded) {
        bool hasNative = false;
        std::size_t scanVisits = 0;
        if (!boundedNativeCount(*this)) {
            cloned->m_loaded = false;
            return cloned.release();
        }
        for (const auto* entity : *this) {
            if (!entity) continue;
            if (entity->rtti() == RS2::EntityOffsetCurve) hasNative = true;
            else if (nativeContent(*entity, scanVisits) != NativeContent::None) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Pattern: Unsupported nested native pattern or scan limit");
                cloned->m_loaded = false;
                return cloned.release();
            }
        }
        if (!hasNative) {
            cloned->setOwner(isOwner());
            for (const auto* entity : *this)
                if (entity) cloned->addEntity(isOwner() ? entity->clone() : entity);
            return cloned.release();
        }
        std::size_t ownedVisits = 0;
        std::unordered_set<const RS_Entity*> seen;
        for (const auto* entity : *this) {
            if (!entity || !ownedTree(*entity, ownedVisits, seen)) {
                RS_DEBUG->print(RS_Debug::D_WARNING, "RS_Pattern: Unsafe descendant ownership in native pattern");
                cloned->m_loaded = false;
                return cloned.release();
            }
        }
        for (const auto* entity : *this) {
            if (!entity) continue;
            if (entity->rtti() == RS2::EntityOffsetCurve) {
                if (!appendOffsetCache(*cloned, *entity)) {
                    cloned->clear();
                    cloned->m_loaded = false;
                    break;
                }
                continue;
            }
            // Even a borrowed pattern must produce independently owned copies.
            auto copy = std::unique_ptr<RS_Entity>{entity->clone()};
            copy->setPen(entity->getPen(true));
            copy->setLayer(nullptr);
            copy->reparent(cloned.get());
            cloned->addEntity(copy.release());
        }
    }
    return cloned.release();
}

/**
 * Loads the given pattern file into this pattern.
 * Lines, arcs and ellipses are retained; native curves use displayed lines.
 * Other ordinary entity types keep their existing unsupported status.
 *
 */
bool RS_Pattern::loadPattern() {
    if (m_loaded) {
        return true;
    }

    RS_DEBUG->print("RS_Pattern::loadPattern");

    // Search for the appropriate pattern if we have only the name of the pattern:
    QString path;

    if (!m_fileName.endsWith(".dxf", Qt::CaseInsensitive)) {
        foreach(const QString & path0, RS_SYSTEM->getPatternList())
        {
            if (QFileInfo(path0).baseName().toLower() == m_fileName.toLower()) {
                path = path0;
                RS_DEBUG->print("Pattern found: %s", path.toLatin1().data());
                break;
            }
        }
        if (path.isEmpty()) {
            RS_DEBUG->print("Pattern not found: %s", m_fileName.toLatin1().data());
        }
    }

    // We have the full path of the pattern:
    else {
        path = m_fileName;
    }

    // No pattern paths found:
    if (path.isEmpty()) {
        RS_DEBUG->print("No pattern \"%s\"available.", m_fileName.toLatin1().data());
        return false;
    }

    RS_Graphic gr;
    RS_FileIO::instance()->fileImport(gr, path);
    if (!boundedNativeCount(gr)) return false;
    for (const auto* e : gr) {
        if (e != nullptr && e->rtti() == RS2::EntityOffsetCurve) {
            if (!appendOffsetCache(*this, *e)) {
                clear();
                return false;
            }
            continue;
        }
        if (e != nullptr && (e->rtti() == RS2::EntityLine || e->rtti() == RS2::EntityArc || e->rtti() == RS2::EntityEllipse)) {
            const RS_Layer* layer = e->getLayer();
            RS_Entity* clone = e->clone();
            clone->reparent(this);
            if (layer != nullptr) {
                clone->setLayer(layer->getName());
            }
            addEntity(clone);
        }
    }

    m_loaded = true;
    RS_DEBUG->print("RS_Pattern::loadPattern: OK");

    return true;
}

QString RS_Pattern::getFileName() const {
    return m_fileName;
}
