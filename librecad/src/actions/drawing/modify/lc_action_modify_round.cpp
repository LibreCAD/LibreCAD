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

#include "lc_action_modify_round.h"

#include "lc_actioninfomessagebuilder.h"
#include "lc_round_options_filler.h"
#include "lc_round_options_widget.h"
#include "rs_arc.h"
#include "rs_document.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_graphic.h"
#include "rs_information.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"

namespace {
// supported entity types for fillet
    const EntityTypeList SUPPORTED_ENTITY_TYPES = {{RS2::EntityLine,
//                             RS2::EntityPolyline, // not atomic
                             RS2::EntityArc,
                             RS2::EntityCircle,
                             RS2::EntityEllipse,
                             RS2::EntitySpline}};

// Whether the point is on an endPoint of the entity
    bool atEndPoint(const RS_Entity &entity, const RS_Vector &point){
        double distance = 1.;
        const RS_Vector nearestPoint = entity.getNearestEndpoint(point, nullptr, &distance);
        return nearestPoint.valid && distance < RS_TOLERANCE;
    }

    bool atEndPoint(const RS_Entity &entity1, const RS_Entity &entity2, const RS_Vector &point){
        return atEndPoint(entity1, point) || atEndPoint(entity2, point);
    }
}

struct LC_ActionModifyRound::RoundActionData {
    RS_Vector coord1;
    RS_Vector coord2;
    RS_RoundData data{};
    LC_DocumentModificationBatch triggerContext;
    LC_RoundResult roundResult;
};

// fixme - review cases for rounding circles and arcs, it's weird enough
// fixme - potentially, it's better to support more fine grained trim mode that will control which entities should be trimmed (first, second, both)?

LC_ActionModifyRound::LC_ActionModifyRound(LC_ActionContext *actionContext)
    :LC_UndoableDocumentModificationAction("ActionModifyRound", actionContext, RS2::ActionModifyRound),
    m_actionData(std::make_unique<RoundActionData>()){
}

LC_ActionModifyRound::~LC_ActionModifyRound() = default;

void LC_ActionModifyRound::doSaveOptions() {
    save("Radius", getRadius());
    save("Trim", isTrimOn());
}

void LC_ActionModifyRound::doLoadOptions() {
    const double radius = loadDouble("Radius", 10.0);
    setRadius(radius);
    const bool trimOn = loadBool("Trim", true);
    setTrim(trimOn);
}

void LC_ActionModifyRound::init(const int status){
    m_snapMode.clear();
    m_snapMode.restriction = RS2::RestrictNothing;
    RS_PreviewActionInterface::init(status);
}

void LC_ActionModifyRound::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
     if (isAtomic(contextEntity)) {
         m_entity1 = static_cast<RS_AtomicEntity*>(contextEntity);
         m_actionData->coord1 = contextEntity->getNearestPointOnEntity(clickPos, true);
         setStatus(SetEntity2);
     }
}

void LC_ActionModifyRound::finish(){
    RS_PreviewActionInterface::finish();
}

/*
    Removes the old fillet, if it exists.

    - by Melwyn Francis Carlo.
*/
bool LC_ActionModifyRound::removeOldFillet(RS_Entity *e, const bool isPolyline) const {
    if (!isArc(e) || m_entity1 == nullptr || m_entity2 == nullptr) {
        return false;
    }

    auto isChained = [this](const RS_Vector &point){
        return atEndPoint(*m_entity1, *m_entity2, point);
    };
    std::vector<RS_Vector> endPoints = {e->getStartpoint(), e->getEndpoint()};
    const bool chained = std::all_of(endPoints.begin(), endPoints.end(), isChained);
    if (!chained) {
        return false;
    }

    if (!isPolyline) {
        m_document->removeEntity(e);
    }

    return true;
}

void LC_ActionModifyRound::drawSnapper() {
    // disable snapper for action
}

bool LC_ActionModifyRound::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    const auto roundResult = m_actionData->roundResult;
    if (roundResult.isPolyline) {
        const auto polyline = m_entity1->getParent();
        roundResult.polyline->setPen(polyline->getPen(false));
        roundResult.polyline->setLayer(polyline->getLayer(false));
    }
    else {
        if (roundResult.round != nullptr) {
            roundResult.round->setPen(m_document->getActivePen());
            roundResult.round->setLayer(m_graphic->getActiveLayer());
        }
    }
    ctx.dontSetActiveLayerAndPen();
    auto tmpContext = m_actionData->triggerContext;
    ctx += tmpContext.entitiesToAdd;
    ctx -= tmpContext.entitiesToDelete;
    return true;
}

void LC_ActionModifyRound::doTriggerCompletion([[maybe_unused]]bool success) {
    m_actionData->coord1 = {};
    m_actionData->coord2 = {};
    m_entity1            = nullptr;
    m_entity2            = nullptr;
    m_actionData->triggerContext.clear();
    // fixme - decide to which state go after trigger - probably it's more convenient to say in SetEntity2?
    setStatus(SetEntity1);
}


void LC_ActionModifyRound::onMouseMoveEvent(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->graphPoint;
    RS_Entity *se = catchAndDescribe(e, SUPPORTED_ENTITY_TYPES, RS2::ResolveAll);
    switch (status) {
        case SetEntity1: {
            if (se != nullptr){
                if (RS_Information::isTrimmable(se)){
                    highlightHover(se);
                }
            }
            break;
        }
        case SetEntity2: {
            highlightSelected(m_entity1);
            if (se != nullptr){
                if (m_entity1 != se && se->isAtomic()){
                    const RS_Vector coord2 = se->getNearestPointOnEntity(mouse, true);


                    const bool trim = m_actionData->data.trim;
                    LC_DocumentModificationBatch ctx;
                    const LC_RoundResult roundResult = RS_Modification::round(mouse, m_actionData->coord1, m_entity1, coord2,
                                                                        static_cast<RS_AtomicEntity*>(se), m_actionData->data, ctx);

                    if (roundResult.error == LC_RoundResult::OK){
                        highlightHover(se);
                        if (m_showRefEntitiesOnPreview) {
                            previewRefPoint(m_actionData->coord1);
                            previewRefSelectablePoint(coord2);
                            previewRefPoint(mouse);
                            previewRefLine(mouse, coord2);
                            previewRefPoint( roundResult.trimmingPoint1);
                            previewRefPoint(roundResult.trimmingPoint2);
                            if (m_actionData->data.trim){
                                previewEntityModifications(m_entity1, roundResult.trimmed1, roundResult.trimmingPoint1, roundResult.trim1Mode);
                                previewEntityModifications(se, roundResult.trimmed2, roundResult.trimmingPoint2, roundResult.trim2Mode);
                            }
                        }
                        if (m_actionData->data.trim && !roundResult.isPolyline){
                            m_preview->removeEntity(roundResult.trimmed1);
                            m_preview->removeEntity(roundResult.trimmed2);
                        }

                        const auto *arc = roundResult.round;
                        if (arc != nullptr){
                            if (m_showRefEntitiesOnPreview) {
                                if (!roundResult.isPolyline) {
                                    previewEntity(arc);
                                }
                            }
                            if (isInfoCursorForModificationEnabled()){
                                const RS_Vector arcEndPoint = arc->getEndpoint();
                                const RS_Vector arcStartPoint = arc->getStartpoint();
                                msg(tr("Round"))
                                    .vector(tr("Point 1:"), arcStartPoint)
                                    .vector(tr("Point 2:"), arcEndPoint)
                                    .toInfoCursorZone2(false);
                            }
                        }
                        if (roundResult.isPolyline) {
                            previewEntity(roundResult.polyline);
                        }
                    }

                    m_actionData->data.trim = trim;
                }
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionModifyRound::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

void LC_ActionModifyRound::previewEntityModifications(const RS_Entity *original, RS_Entity *modified, const RS_Vector& roundPoint, const int mode) const {
    const bool decreased = modified->getLength() < original->getLength();
    if (isLine(modified)){ // fixme - support of polyline
        if (decreased){
            if (mode == LC_RoundResult::TRIM_START){
                previewRefLine(modified->getStartpoint(), original->getStartpoint());
            }
            else{
                previewRefLine(modified->getEndpoint(), original->getEndpoint());
            }
        }
        else{
            if (mode == LC_RoundResult::TRIM_START){
                if (m_showRefEntitiesOnPreview) {
                    previewRefPoint(original->getStartpoint());
                }
                previewLine(original->getStartpoint(), roundPoint);
            }
            else{
                previewRefPoint(original->getEndpoint());
                previewLine(original->getEndpoint(), roundPoint);
            }
        }
    }
    else if (isArc(modified)){
            auto* newArc = static_cast<RS_Arc*>(modified);
            RS_ArcData arcData = newArc->getData();
            std::swap(arcData.angle1, arcData.angle2);
            previewRefArc(arcData);
    }
}

void LC_ActionModifyRound::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->graphPoint;
    RS_Entity *se = catchEntityByEvent(e, SUPPORTED_ENTITY_TYPES, RS2::ResolveAll);
    switch (status) {
        case SetEntity1: {
            if (isAtomic(se) && RS_Information::isTrimmable(se)){
                m_entity1 = static_cast<RS_AtomicEntity*>(se);
                m_actionData->coord1 = se->getNearestPointOnEntity(mouse, true);
                setStatus(SetEntity2);
            }
            break;
        }
        case SetEntity2: {
            if (isAtomic(se) ){
                m_entity2 = static_cast<RS_AtomicEntity*>(se);
                m_actionData->coord2 = mouse;
                const auto roundResult = RS_Modification::round(m_actionData->coord2, m_actionData->coord1, m_entity1,
                                                                m_actionData->coord2, m_entity2, m_actionData->data, m_actionData->triggerContext);

                if (roundResult.error == LC_RoundResult::OK) {
                    m_actionData->roundResult = roundResult;
                    trigger();
                }
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionModifyRound::onMouseRightButtonRelease(const int status, [[maybe_unused]] const LC_MouseEvent* e) {
    deletePreview();
    initPrevious(status);
}

bool LC_ActionModifyRound::doProcessCommand(int status, const QString &command) {
    bool accept = false;

    switch (status) {
        case SetEntity1:
        case SetEntity2: {
            if (checkCommand("radius", command)){
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetRadius);
                accept = true;
            } else if (checkCommand("trim", command)){
                deletePreview();
                m_lastStatus = static_cast<Status>(status);
                setStatus(SetTrim);
                m_actionData->data.trim = !m_actionData->data.trim;
                updateOptions();
                accept = true;
            } else {
                bool ok = false;
                const double r = RS_Math::eval(command, &ok);
                if (ok && r > 1.0e-10){
                    accept = true;
                    m_actionData->data.radius = r;

                } else {
                    commandMessage(tr("Not a valid expression"));
                }
                // fixme - should we allow change status for invalid input?
                updateOptions();
                setStatus(m_lastStatus);
            }
            break;
        }
        case SetRadius: {
            bool ok = false;
            const double r = RS_Math::eval(command, &ok);
            if (ok){
                accept = true;
                m_actionData->data.radius = r;
            } else {
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

QStringList LC_ActionModifyRound::getAvailableCommands(){
    QStringList cmd;
    switch (getStatus()) {
        case SetEntity1:
        case SetEntity2:
            cmd += command("radius");
            cmd += command("trim");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionModifyRound::setRadius(const double r) const {
    m_actionData->data.radius = r;
}

double LC_ActionModifyRound::getRadius() const{
    return m_actionData->data.radius;
}

void LC_ActionModifyRound::setTrim(const bool t) const {
    m_actionData->data.trim = t;
}

bool LC_ActionModifyRound::isTrimOn() const{
    return m_actionData->data.trim;
}

void LC_ActionModifyRound::updateActionPrompt(){
    switch (getStatus()) {
        case SetEntity1:
            updatePromptTRBack(tr("Specify first entity or enter radius <%1>").arg(getRadius()));
            break;
        case SetEntity2:
            updatePromptTRBack(tr("Specify second entity"));
            break;
        case SetRadius:
            updatePromptTRCancel(tr("Enter radius:"));
            break;
        default:
            updatePrompt();
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionModifyRound::createOptionsWidget(){
    return new LC_RoundOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyRound::createOptionsFiller() {
    return new LC_RoundOptionsFiller();
}

RS2::CursorType LC_ActionModifyRound::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
