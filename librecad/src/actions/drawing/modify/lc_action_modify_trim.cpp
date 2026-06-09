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

#include "lc_action_modify_trim.h"

#include "lc_actioninfomessagebuilder.h"
#include "rs_arc.h"
#include "rs_atomicentity.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_modification.h"

struct LC_ActionModifyTrim::TrimActionData {
    RS_Vector limitCoord;
    RS_Vector trimCoord;
};

/**
 * @param actionContext
 * @param both Trim both entities.
 */
LC_ActionModifyTrim::LC_ActionModifyTrim(LC_ActionContext* actionContext, const bool both)
    : LC_UndoableDocumentModificationAction(both ? "ActionModifyTrim2" : "ActionModifyTrim", actionContext, both ? RS2::ActionModifyTrim2 : RS2::ActionModifyTrim),
      m_actionData(std::make_unique<TrimActionData>()), m_both{both} {
}

LC_ActionModifyTrim::~LC_ActionModifyTrim() = default;

void LC_ActionModifyTrim::init(const int status) {
    m_snapMode.clear();
    m_snapMode.restriction = RS2::RestrictNothing;
    RS_PreviewActionInterface::init(status);
}

void LC_ActionModifyTrim::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    if (isAtomic(contextEntity)) {
        m_trimEntity = static_cast<RS_AtomicEntity*>(contextEntity);
        m_actionData->trimCoord = clickPos;
    }
}

void LC_ActionModifyTrim::finish() {
    RS_PreviewActionInterface::finish();
}

bool LC_ActionModifyTrim::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (isAtomic(m_trimEntity) && m_limitEntity != nullptr /* && limitEntity->isAtomic()*/) {
        const LC_TrimResult trimResult = RS_Modification::trim(m_actionData->trimCoord, m_trimEntity, m_actionData->limitCoord,
                                                               m_limitEntity, m_both, ctx);

        if (trimResult.trimmed1 != nullptr) {
            trimResult.trimmed1->setPen(m_trimEntity->getPen(false));
            trimResult.trimmed1->setLayer(m_trimEntity->getLayer(false));

            if (m_both) {
                if (trimResult.trimmed2 != nullptr) {
                    trimResult.trimmed2->setPen(m_limitEntity->getPen(false));
                    trimResult.trimmed2->setLayer(m_limitEntity->getLayer(false));
                }
            }
            ctx.dontSetActiveLayerAndPen();
            return trimResult.result;
        }
        return false;
    }
    return false;
}

void LC_ActionModifyTrim::doTriggerCompletion(const bool success) {
    if (success) {
        m_trimEntity = nullptr;
        if (m_both) {
            m_limitEntity = nullptr;
            setStatus(ChooseLimitEntity);
        }
        else {
            setStatus(ChooseTrimEntity);
        }
    }
}

void LC_ActionModifyTrim::previewTrim(RS_Entity* entityToTrimCandidate, RS_Entity* limitingEntity, const RS_Vector& trimCoordinates,
                                      const RS_Vector& limitCoordinates, bool& trimInvalid) const {
    if (entityToTrimCandidate != nullptr && entityToTrimCandidate != limitingEntity) {
        if (entityToTrimCandidate->isAtomic()) {
            auto* atomicTrimCandidate = dynamic_cast<RS_AtomicEntity*>(entityToTrimCandidate);

            LC_DocumentModificationBatch ctx;
            const LC_TrimResult trimResult = RS_Modification::trim(trimCoordinates, atomicTrimCandidate, limitCoordinates, limitingEntity,
                                                                   m_both, ctx);
            if (trimResult.result) {
                trimInvalid = false;
                highlightHover(entityToTrimCandidate);
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(trimResult.intersection1);
                    previewRefTrimmedEntity(trimResult.trimmed1, entityToTrimCandidate);
                    if (trimResult.intersection2.valid) {
                        previewRefPoint(trimResult.intersection2);
                    }
                    if (m_both) {
                        if (trimResult.trimmed2 != nullptr) {
                            previewRefTrimmedEntity(trimResult.trimmed2, limitingEntity);
                        }
                    }
                    if (isInfoCursorForModificationEnabled()) {
                        const auto msg = rtti() == RS2::ActionModifyTrim2 ? tr("Trim Two") : tr("Trim");
                        auto builder = msgStart().string(msg).vector(tr("Intersection:"), trimResult.intersection1);
                        if (trimResult.intersection2.valid) {
                            builder.vector(tr("Intersection 2:"), trimResult.intersection2);
                        }
                        builder.toInfoCursorZone2(false);
                    }
                }
            }
        }
    }
}

// todo - check trim both mode - it seems that limiting entity should be atomic too...
void LC_ActionModifyTrim::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->graphPoint;
    bool trimInvalid = true;
    switch (status) {
        case ChooseLimitEntity: {
            RS_Entity* se = catchAndDescribe(e, RS2::ResolveAllButTextImage);
            if (m_trimEntity != nullptr && se != nullptr) {
                previewTrim(m_trimEntity, se, m_actionData->trimCoord, mouse, trimInvalid);
                if (trimInvalid) {
                    highlightSelected(m_trimEntity);
                }
            }
            else if (se != nullptr) {
                highlightHover(se);
            }

            break;
        }
        case ChooseTrimEntity: {
            RS_Entity* entityToTrimCandidate = catchAndDescribe(e, RS2::ResolveNone);
            previewTrim(entityToTrimCandidate, m_limitEntity, mouse, m_actionData->limitCoord, trimInvalid);
            if (trimInvalid) {
                highlightSelected(m_limitEntity);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyTrim::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->graphPoint;
    switch (status) {
        case ChooseLimitEntity: {
            RS_Entity* se = catchEntityByEvent(e, RS2::ResolveAllButTextImage);
            if (se != nullptr) {
                m_limitEntity = se;
                if (m_limitEntity->rtti() != RS2::EntityPolyline/*&& limitEntity->isAtomic()*/) {
                    m_actionData->limitCoord = mouse;
                    if (m_trimEntity != nullptr) {
                        trigger();
                    }
                    else {
                        setStatus(ChooseTrimEntity);
                    }
                }
            }
            break;
        }
        case ChooseTrimEntity: {
            RS_Entity* se = catchEntityByEvent(e, RS2::ResolveNone);
            if (isAtomic(se) && se != m_limitEntity) {
                m_actionData->trimCoord = mouse;
                m_trimEntity = static_cast<RS_AtomicEntity*>(se);
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyTrim::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionModifyTrim::updateActionPrompt() {
    switch (getStatus()) {
        case ChooseLimitEntity:
            if (m_both) {
                updatePromptTRCancel(tr("Select first trim entity"));
            }
            else {
                updatePromptTRBack(tr("Select limiting entity"));
            }
            break;
        case ChooseTrimEntity:
            if (m_both) {
                updatePromptTRCancel(tr("Select second trim entity"));
            }
            else {
                updatePromptTRBack(tr("Select entity to trim"));
            }
            break;
        default:
            updatePrompt();
            break;
    }
}

RS2::CursorType LC_ActionModifyTrim::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

void LC_ActionModifyTrim::previewRefTrimmedEntity(RS_Entity* trimmed, const RS_Entity* original) const {
    const int rtti = trimmed->rtti();
    switch (rtti) {
        case RS2::EntityLine: {
            RS_Vector start = original->getStartpoint();
            const RS_Vector startTrimmed = trimmed->getStartpoint();
            RS_Vector end = original->getEndpoint();
            const RS_Vector endTrimmed = trimmed->getEndpoint();
            const bool sameStart = start == startTrimmed;
            const bool sameEnd = end == endTrimmed;

            if (!sameStart) {
                end = startTrimmed;
            }
            if (!sameEnd) {
                start = endTrimmed;
            }
            previewRefLine(start, end);
            break;
        }
        case RS2::EntityArc: {
            auto* arc = static_cast<RS_Arc*>(trimmed);
            if (arc == nullptr) {
                break;
            }
            RS_ArcData data = arc->getData();
            data.reversed = !data.reversed;
            previewRefArc(data);
            break;
        }
        case RS2::EntityCircle: {
            // that's really strange case - not trimmed circle???
            //            auto* circle = dynamic_cast<RS_Circle*>(trimmed);
            //            previewRefCircle(circle->getCenter(), circle->getRadius());
            break;
        }
        case RS2::EntityEllipse: {
            const auto* ellipse = static_cast<RS_Ellipse*>(trimmed);
            if (ellipse == nullptr) {
                break;
            }
            auto data = ellipse->getData();
            data.reversed = !data.reversed;
            previewRefEllipse(data);
            break;
        }
        case RS2::EntityParabola:{
          // Parabola has no reversed-direction toggle; just preview the
          // resulting (already-trimmed) entity.
          previewEntity(trimmed);
          break;
        }
        default: {
            previewEntity(trimmed);
            RS_DEBUG->print("RS_ActionModifyTrim::unhandled trimmed entity type");
        }
    }
}
