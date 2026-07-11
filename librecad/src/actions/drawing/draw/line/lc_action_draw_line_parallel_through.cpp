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

#include "lc_action_draw_line_parallel_through.h"

#include "lc_line_parallel_through_options_filler.h"
#include "lc_line_parallel_through_options_widget.h"
#include "rs_creation.h"
#include "rs_document.h"
#include "rs_polyline.h"
#include "rs_preview.h"
// fixme - sand - consider relaxing existing restrictions, if any - and use no-restrictions mode for this action.

namespace {
    //this holds a list of entity types which supports tangent
    const auto g_supportedEntityTypes = EntityTypeList{
                {RS2::EntityArc, RS2::EntityCircle, RS2::EntityLine,  RS2::EntityParabola/*, RS2::EntitySplinePoints*/}
    };
}

LC_ActionDrawLineParallelThrough::LC_ActionDrawLineParallelThrough(LC_ActionContext* actionContext)
    : LC_UndoableDocumentModificationAction("ActionDrawLineParallelThrough", actionContext, RS2::ActionDrawLineParallelThrough), m_coord(new RS_Vector{}) {
}

LC_ActionDrawLineParallelThrough::~LC_ActionDrawLineParallelThrough() = default;

void LC_ActionDrawLineParallelThrough::doSaveOptions() {
    save("Number", m_numberToCreate);
    save("Symmetric", m_symmetric);
    save("Within", m_distributeWithin);
}

void LC_ActionDrawLineParallelThrough::doLoadOptions() {
    m_numberToCreate = loadInt("Number", 1);
    m_symmetric = loadBool("Symmetric", false);
    m_distributeWithin = loadBool("Within", false);
}

bool LC_ActionDrawLineParallelThrough::isInVisualSnapStatus(int status) {
    return (status == SetPos);
}

void LC_ActionDrawLineParallelThrough::finish() {
    if (m_entity != nullptr) {
        m_entity->setHighlighted(false);
        m_entity = nullptr;
    }
    RS_PreviewActionInterface::finish();
}

void LC_ActionDrawLineParallelThrough::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
    auto entity = contextEntity;
    if (isPolyline(contextEntity)) {
        const auto polyline = static_cast<RS_Polyline*>(contextEntity);
        entity = polyline->getNearestEntity(clickPos);
    }
    const RS2::EntityType rtti = entity->rtti();
    if (g_supportedEntityTypes.contains(rtti)) {
        m_entity = entity;
        setStatus(SetPos);
    }
}

bool LC_ActionDrawLineParallelThrough::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    if (m_entity != nullptr) {
        RS_Creation::createParallelThrough(*m_coord, m_numberToCreate, m_entity, m_symmetric, m_distributeWithin, ctx.entitiesToAdd);
        return true;
    }
    return false;
}

void LC_ActionDrawLineParallelThrough::doTriggerCompletion([[maybe_unused]] bool success) {
}

void LC_ActionDrawLineParallelThrough::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    const RS_Vector& snap = e->snapPoint;
    switch (status) {
        case SetEntity: {
            const auto entity = catchAndDescribe(e, RS2::ResolveAll);
            if (entity != nullptr && g_supportedEntityTypes.contains(entity->rtti())) {
                m_entity = entity;
                highlightHover(m_entity);
                if (m_showRefEntitiesOnPreview) {
                    const RS_Vector nearest = m_entity->getNearestPointOnEntity(*m_coord, false);
                    previewRefPoint(nearest);
                }
            }
            break;
        }
        case SetPos: {
            *m_coord = getFreeSnapAwarePoint(e, snap);
            highlightSelected(m_entity);
            QList<RS_Entity*> parallels;
            RS_Creation::createParallelThrough(*m_coord, m_numberToCreate, m_entity, m_symmetric, m_distributeWithin, parallels);
            if (!parallels.empty()) {
                auto en = parallels.front();
                m_preview->addAllFromList(parallels);
                const RS_Vector nearest = m_entity->getNearestPointOnEntity(*m_coord, false);
                moveRelativeZero(nearest); // fixme - should we restore original relzero?
                if (m_numberToCreate == 1 && !m_symmetric) {
                    prepareEntityDescription(en, RS2::EntityDescriptionLevel::DescriptionCreating);
                }
                else {
                    const int creatingNumber = m_numberToCreate * (m_symmetric ? 2 : 1);
                    appendInfoCursorEntityCreationMessage(QString::number(creatingNumber) + tr(" entities will be created"));
                }
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(nearest);
                    previewRefLine(nearest, *m_coord);

                    if (m_symmetric&& isLine(m_entity)) {
                        // fixme - support of polyline
                        RS_Vector otherPoint = *m_coord;
                        otherPoint.mirror(m_entity->getStartpoint(), m_entity->getEndpoint());
                        previewRefPoint(otherPoint);
                        previewRefLine(nearest, otherPoint);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLineParallelThrough::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    switch (status) {
        case SetEntity: {
            const auto entity = catchEntityByEvent(e, RS2::ResolveAll);
            if (entity != nullptr && g_supportedEntityTypes.contains(entity->rtti())) {
                m_entity = entity;
                setStatus(SetPos);
            }
            break;
        }
        case SetPos: {
            fireCoordinateEventForSnap(e);
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLineParallelThrough::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    if (m_entity != nullptr) {
        m_entity = nullptr;
    }
    initPrevious(status);
}

void LC_ActionDrawLineParallelThrough::onCoordinateEvent(const int status, [[maybe_unused]] bool isZero, const RS_Vector& coord) {
    switch (status) {
        case SetPos: {
            *m_coord = coord;
            addSnappedPointToVisualSnap(coord);
            trigger();
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawLineParallelThrough::updateActionPrompt() {
    switch (getStatus()) {
        case SetEntity:
            updatePromptTRCancel(tr("Select entity"));
            break;
        case SetPos:
            updatePromptTRBack(tr("Specify through point"), MOD_SHIFT_FREE_SNAP);
            break;
        case SetNumber:
            updatePromptTRBack(tr("Number:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

bool LC_ActionDrawLineParallelThrough::doProcessCommand(const int status, const QString& command) {
    bool accept = false;

    switch (status) {
        case SetEntity:
        case SetPos: {
            if (checkCommand("number", command)) {
                deletePreview();
                m_lastStatus = static_cast<Status>(getStatus());
                setStatus(SetNumber);
                accept = true;
            }
            break;
        }
        case SetNumber: {
            bool ok = false;
            const int n = command.toInt(&ok);
            if (ok) {
                accept = true;
                if (n > 0 && n < 100) {
                    m_numberToCreate = n;
                }
                else {
                    commandMessage(tr("Not a valid number. Try 1..99"));
                }
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(m_lastStatus);
            break;
        }
        default:
            break;
    }
    return accept;
}

QStringList LC_ActionDrawLineParallelThrough::getAvailableCommands() {
    QStringList cmd;
    switch (getStatus()) {
        case SetEntity:
            cmd += command("number");
            break;
        default:
            break;
    }
    return cmd;
}

RS2::CursorType LC_ActionDrawLineParallelThrough::doGetMouseCursor([[maybe_unused]] const int status) {
    switch (status) {
        case SetEntity:
            return RS2::SelectCursor;
        case SetNumber:
        case SetPos:
            return RS2::CadCursor;
        default:
            return RS2::NoCursorChange;
    }
}

int LC_ActionDrawLineParallelThrough::getNumber() const {
    return m_numberToCreate;
}

void LC_ActionDrawLineParallelThrough::setNumber(const int n) {
    m_numberToCreate = n;
}

LC_ActionOptionsWidget* LC_ActionDrawLineParallelThrough::createOptionsWidget() {
    return new LC_LineParallelThroughOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawLineParallelThrough::createOptionsFiller() {
    return new LC_LineParallelThroughOptionsFiller();
}
