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

#include "rs_actionmodifytrim.h"

#include "lc_actioninfomessagebuilder.h"
#include "rs_arc.h"
#include "rs_atomicentity.h"
#include "rs_debug.h"
#include "rs_ellipse.h"
#include "rs_modification.h"

struct RS_ActionModifyTrim::TrimActionData {
    RS_Vector limitCoord;
    RS_Vector trimCoord;
};

/**
 * @param both Trim both entities.
 */
RS_ActionModifyTrim::RS_ActionModifyTrim(LC_ActionContext *actionContext, bool both)
    : RS_PreviewActionInterface("Trim Entity",actionContext, both ? RS2::ActionModifyTrim2 : RS2::ActionModifyTrim)
    , m_trimEntity{nullptr}, m_limitEntity{nullptr}
    , m_actionData(std::make_unique<TrimActionData>()), m_both{both} {
}

RS_ActionModifyTrim::~RS_ActionModifyTrim() = default;

void RS_ActionModifyTrim::init(int status) {
    m_snapMode.clear();
    m_snapMode.restriction = RS2::RestrictNothing;
    RS_PreviewActionInterface::init(status);
}

void RS_ActionModifyTrim::finish(bool updateTB) {
    RS_PreviewActionInterface::finish(updateTB);
}

void RS_ActionModifyTrim::doTrigger() {
    RS_DEBUG->print("RS_ActionModifyTrim::trigger()");

    if (m_trimEntity && m_trimEntity->isAtomic() &&
        m_limitEntity /* && limitEntity->isAtomic()*/) {

        RS_Modification m(*m_container, m_viewport);
        [[maybe_unused]] LC_TrimResult trimResult =  m.trim(m_actionData->trimCoord,  m_trimEntity,
               m_actionData->limitCoord, /*(RS_AtomicEntity*)*/m_limitEntity,
               m_both);

        m_trimEntity = nullptr;
        if (m_both) {
            m_limitEntity = nullptr;
            setStatus(ChooseLimitEntity);
        } else {
            setStatus(ChooseTrimEntity);
        }
    }
}

// todo - check trim both mode - it seems that limiting entity should be atomic too...
void RS_ActionModifyTrim::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;
    switch (status) {
        case ChooseLimitEntity: {
            RS_Entity *se = catchAndDescribe(e, RS2::ResolveAllButTextImage);
            if (se != nullptr) {
                highlightHover(se);
            }
            break;
        }
        case ChooseTrimEntity: {
            RS_Entity *se = catchAndDescribe(e, RS2::ResolveNone);
            bool trimInvalid = true;

            if (se != nullptr && se != m_limitEntity) {
                if (se->isAtomic()) {

                    auto *atomicTrimCandidate = dynamic_cast<RS_AtomicEntity *>(se);

                    RS_Modification m(*m_container, m_viewport);
                    LC_TrimResult trimResult = m.trim(mouse, atomicTrimCandidate,
                                                      m_actionData->limitCoord, m_limitEntity,
                                                      m_both, true);
                    if (trimResult.result) {
                        trimInvalid = false;
                        highlightHover(se);
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(trimResult.intersection1);
                            previewRefTrimmedEntity(trimResult.trimmed1, se);
                            if (trimResult.intersection2.valid) {
                                previewRefPoint(trimResult.intersection2);
                            }
                            if (m_both) {
                                if (trimResult.trimmed2 != nullptr) {
                                    previewRefTrimmedEntity(trimResult.trimmed2, m_limitEntity);
                                }
                            }
                            if (isInfoCursorForModificationEnabled()){
                                auto builder = msg(tr("Trim"))
                                    .vector(tr("Intersection:"), trimResult.intersection1);
                                if (trimResult.intersection2.valid) {
                                    builder.vector(tr("Intersection 2:"), trimResult.intersection2);
                                }
                                builder.toInfoCursorZone2(false);
                            }
                        }
                    }
                }
            }
            if (trimInvalid) {
                highlightSelected(m_limitEntity);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyTrim::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;
    switch (status) {
        case ChooseLimitEntity: {
            RS_Entity *se = catchEntityByEvent(e, RS2::ResolveAllButTextImage);
            if (se != nullptr) {
                m_limitEntity = se;
                if (m_limitEntity->rtti() != RS2::EntityPolyline/*&& limitEntity->isAtomic()*/) {
                    m_actionData->limitCoord = mouse;
                    setStatus(ChooseTrimEntity);
                }
            }
            break;
        }
        case ChooseTrimEntity: {
            RS_Entity *se = catchEntityByEvent(e, RS2::ResolveNone);
            if (se != nullptr) {
                if (se->isAtomic() && se != m_limitEntity) {
                    m_actionData->trimCoord = mouse;
                    m_trimEntity = dynamic_cast<RS_AtomicEntity *>(se);
                    trigger();
                }
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyTrim::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void RS_ActionModifyTrim::updateMouseButtonHints() {
    switch (getStatus()) {
        case ChooseLimitEntity:
            if (m_both) {
                updateMouseWidgetTRCancel(tr("Select first trim entity"));
            } else {
                updateMouseWidgetTRBack(tr("Select limiting entity"));
            }
            break;
        case ChooseTrimEntity:
            if (m_both) {
                updateMouseWidgetTRCancel(tr("Select second trim entity"));
            } else {
                updateMouseWidgetTRBack(tr("Select entity to trim"));
            }
            break;
        default:
            updateMouseWidget();
            break;
    }
}
RS2::CursorType RS_ActionModifyTrim::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}

void RS_ActionModifyTrim::previewRefTrimmedEntity(RS_Entity *trimmed, RS_Entity* original) {
    int rtti = trimmed->rtti();
    switch (rtti){
        case RS2::EntityLine:{
            RS_Vector start = original->getStartpoint();
            RS_Vector startTrimmed = trimmed->getStartpoint();
            RS_Vector end = original->getEndpoint();
            RS_Vector endTrimmed = trimmed->getEndpoint();
            bool sameStart = start == startTrimmed;
            bool sameEnd = end == endTrimmed;

            if (!sameStart){
                end = startTrimmed;
            }
            if (!sameEnd){
                start = endTrimmed;
            }
            previewRefLine(start, end);
            break;
        }
        case RS2::EntityArc:{
            auto* arc = dynamic_cast<RS_Arc *>(trimmed);
            RS_ArcData data = arc->getData();
            data.reversed = !data.reversed;
            previewRefArc(data);
            break;
        }
        case RS2::EntityCircle:{
            // that's really strange case - not trimmed circle??? 
//            auto* circle = dynamic_cast<RS_Circle*>(trimmed);
//            previewRefCircle(circle->getCenter(), circle->getRadius());
            break;
        }
        case RS2::EntityEllipse:{
            auto* ellipse = dynamic_cast<RS_Ellipse *>(trimmed);
            auto data = ellipse->getData();
            data.reversed = !data.reversed;
            previewRefEllipse(data);
            break;
        }
        case RS2::EntityParabola:{
            // fixme - check trimming of parabola and drawing part that will be trimmed
        }
        default:{
            previewEntity(trimmed);
            RS_DEBUG->print("RS_ActionModifyTrim::unhandled trimmed entity type");
        }
    }
}
