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

#include "rs_actionmodifyround.h"

#include "lc_actioninfomessagebuilder.h"
#include "qg_roundoptions.h"
#include "rs_arc.h"
#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_information.h"
#include "rs_modification.h"
#include "rs_preview.h"

namespace {
// supported entity types for fillet
    EntityTypeList eType = {{RS2::EntityLine,
//                             RS2::EntityPolyline, // not atomic
                             RS2::EntityArc,
                             RS2::EntityCircle,
                             RS2::EntityEllipse,
                             RS2::EntitySpline}};

// Whether the point is on an endPoint of the entity
    bool atEndPoint(RS_Entity &entity, const RS_Vector &point){
        double distance = 1.;
        RS_Vector nearestPoint = entity.getNearestEndpoint(point, &distance);
        return nearestPoint.valid && distance < RS_TOLERANCE;
    }

    bool atEndPoint(RS_Entity &entity1, RS_Entity &entity2, const RS_Vector &point){
        return atEndPoint(entity1, point) || atEndPoint(entity2, point);
    }
}

struct RS_ActionModifyRound::RoundActionData {
    RS_Vector coord1;
    RS_Vector coord2;
    RS_RoundData data{};
};

// fixme - review cases for rounding circles and arcs, it's weird enough
// fixme - potentially, it's better to support more fine grained trim mode that will control which entities should be trimmed (first, second, both)?

RS_ActionModifyRound::RS_ActionModifyRound(LC_ActionContext *actionContext)
    :RS_PreviewActionInterface("Round Entities", actionContext, RS2::ActionModifyRound),
    m_actionData(std::make_unique<RoundActionData>()), m_lastStatus(SetEntity1){
}

RS_ActionModifyRound::~RS_ActionModifyRound() = default;


void RS_ActionModifyRound::init(int status){
    m_snapMode.clear();
    m_snapMode.restriction = RS2::RestrictNothing;
    RS_PreviewActionInterface::init(status);
}

void RS_ActionModifyRound::doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) {
     if (isAtomic(contextEntity)) {
         m_entity1 = static_cast<RS_AtomicEntity*>(contextEntity);
         m_actionData->coord1 = contextEntity->getNearestPointOnEntity(clickPos, true);
         setStatus(SetEntity2);
     }
}

void RS_ActionModifyRound::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
}

/*
    Removes the old fillet, if it exists.

    - by Melwyn Francis Carlo.
*/
bool RS_ActionModifyRound::removeOldFillet(RS_Entity *e, const bool &isPolyline){
    if (!isArc(e) || m_entity1 == nullptr || m_entity2 == nullptr) {
        return false;
    }

    auto isChained = [this](const RS_Vector &point){
        return atEndPoint(*m_entity1, *m_entity2, point);
    };
    std::vector<RS_Vector> endPoints = {e->getStartpoint(), e->getEndpoint()};
    bool chained = std::all_of(endPoints.begin(), endPoints.end(), isChained);
    if (!chained) {
        return false;
    }

    if (!isPolyline) {
        m_container->removeEntity(e);
    }

    return true;
}

void RS_ActionModifyRound::drawSnapper() {
    // disable snapper for action   
}

void RS_ActionModifyRound::doTrigger() {
    RS_DEBUG->print("RS_ActionModifyRound::trigger()");

    bool foundPolyline = false;

    if ((m_entity1->getParent() != nullptr) && (m_entity2->getParent() != nullptr)) {
        if (isPolyline(m_entity1->getParent()) &&
            isPolyline(m_entity2->getParent()) &&
            (m_entity1->getParent() == m_entity2->getParent())) {
            foundPolyline = true;

            for (auto* e : m_entity1->getParent()->getEntityList()) {
                if ((e != m_entity1) && (e != m_entity2)) {
                    if (removeOldFillet(e, foundPolyline)) {
                        m_entity1->getParent()->removeEntity(e);
                        break;
                    }
                }
            }
        }

        if (!foundPolyline) {
            for (auto* e : m_container->getEntityList()) {
                if ((e != m_entity1) && (e != m_entity2)) {
                    if (removeOldFillet(e, foundPolyline))
                        break;
                }
            }
        }
    }

    RS_Modification m(*m_container, m_viewport);
    m.round(m_actionData->coord2, m_actionData->coord1, m_entity1,
            m_actionData->coord2, m_entity2, m_actionData->data);

    //coord = RS_Vector(false);
    m_actionData->coord1 = RS_Vector(false);
    m_entity1 = nullptr;
    m_actionData->coord2 = RS_Vector(false);
    m_entity2 = nullptr;
    // fixme - decide to which state go after trigger - probably it's more convenient to say in SetEntity2?
    setStatus(SetEntity1);
}

void RS_ActionModifyRound::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;
    RS_Entity *se = catchAndDescribe(e, eType, RS2::ResolveAllButTextImage);
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
                if (m_entity1 != se && RS_Information::isTrimmable(se) && se->isAtomic()){

                    RS_Vector coord2 = se->getNearestPointOnEntity(mouse, true);
                    RS_Entity *tmp1 = m_entity1->clone();
                    RS_Entity *tmp2 = se->clone();
                    tmp1->reparent(m_preview.get());
                    tmp2->reparent(m_preview.get());
                    previewEntity(tmp1);
                    previewEntity(tmp2);

                    bool trim = m_actionData->data.trim;
//                    pPoints->data.trim = false;
                    RS_Modification m(*m_preview, m_viewport, false);
                    LC_RoundResult *roundResult = m.round(mouse,
                                                          m_actionData->coord1,
                                                          (RS_AtomicEntity *) tmp1,
                                                          coord2,
                                                          (RS_AtomicEntity *) tmp2,
                                                          m_actionData->data);

                    if (roundResult != nullptr && roundResult->error == LC_RoundResult::OK){
                        highlightHover(se);
                        auto *arc = roundResult->round;
                        if (arc != nullptr){

                            RS_Vector arcStartPoint = arc->getStartpoint();
                            RS_Vector arcEndPoint = arc->getEndpoint();
                            if (m_showRefEntitiesOnPreview) {
                                previewRefPoint(arcStartPoint);
                                previewRefPoint(arcEndPoint);
                                previewRefPoint(m_actionData->coord1);
                                previewRefSelectablePoint(coord2);
                                previewRefPoint(mouse);
                                previewRefLine(mouse, coord2);
                                if (trim){
                                    previewEntityModifications(m_entity1, roundResult->trimmed1, arcStartPoint, roundResult->trim1Mode);
                                    previewEntityModifications(se, roundResult->trimmed2, arcEndPoint, roundResult->trim2Mode);
                                }
                            }
                            if (trim){
                                m_preview->removeEntity(roundResult->trimmed1);
                                m_preview->removeEntity(roundResult->trimmed2);
                            }

                            if (isInfoCursorForModificationEnabled()){
                                msg(tr("Round"))
                                    .vector(tr("Point 1:"), arcStartPoint)
                                    .vector(tr("Point 2:"), arcEndPoint)
                                    .toInfoCursorZone2(false);
                            }
                        }
                    }

                    m_actionData->data.trim = trim;

                    m_preview->removeEntity(tmp1);
                    m_preview->removeEntity(tmp2);
                }
            }
            break;
        }
        default:
            break;
    }
}

bool RS_ActionModifyRound::doUpdateDistanceByInteractiveInput(const QString& tag, double distance) {
    if (tag == "radius") {
        setRadius(distance);
        return true;
    }
    return false;
}

void RS_ActionModifyRound::previewEntityModifications(const RS_Entity *original, RS_Entity *modified, RS_Vector& roundPoint, int mode){
    bool decreased = modified->getLength() < original->getLength();
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
            auto* newArc = dynamic_cast<RS_Arc*>(modified);
            RS_ArcData arcData = newArc->getData();
            std::swap(arcData.angle1, arcData.angle2);
            previewRefArc(arcData);
    }
}

void RS_ActionModifyRound::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;
    RS_Entity *se = catchEntityByEvent(e, eType, RS2::ResolveAll);
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
            if (isAtomic(se) &&  RS_Information::isTrimmable(m_entity1, se)){
                m_entity2 = static_cast<RS_AtomicEntity*>(se);
                m_actionData->coord2 = mouse;/* se->getNearestPointOnEntity(mouse, true);*/
                //setStatus(ChooseRounding);
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRound::onMouseRightButtonRelease(int status, [[maybe_unused]]LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

bool RS_ActionModifyRound::doProcessCommand(int status, const QString &c) {
    bool accept = false;

    switch (status) {
        case SetEntity1:
        case SetEntity2: {
            if (checkCommand("radius", c)){
                deletePreview();
                m_lastStatus = (Status) status;
                setStatus(SetRadius);
                accept = true;
            } else if (checkCommand("trim", c)){
                deletePreview();
                m_lastStatus = (Status) status;
                setStatus(SetTrim);
                m_actionData->data.trim = !m_actionData->data.trim;
                updateOptions();
                accept = true;
            } else {
                bool ok;
                double r = RS_Math::eval(c, &ok);
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
            bool ok;
            double r = RS_Math::eval(c, &ok);
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
            /*case SetTrim: {
                if (c==cmdYes.lower() || c==cmdYes2) {
                data->trim = true;
            } else if (c==cmdNo.lower() || c==cmdNo2) {
                data->trim = false;
                        } else {
                            RS_DIALOGFACTORY->commandMessage(tr("Please enter 'Yes' "
                       "or 'No'"));
                        }
                        RS_DIALOGFACTORY->requestOptions(this, true, true);
                        setStatus(lastStatus);
                    }
                    break;*/

        default:
            break;
    }
    return accept;
}

QStringList RS_ActionModifyRound::getAvailableCommands(){
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

void RS_ActionModifyRound::setRadius(double r){
    m_actionData->data.radius = r;
}

double RS_ActionModifyRound::getRadius() const{
    return m_actionData->data.radius;
}

void RS_ActionModifyRound::setTrim(bool t){
    m_actionData->data.trim = t;
}

bool RS_ActionModifyRound::isTrimOn() const{
    return m_actionData->data.trim;
}

void RS_ActionModifyRound::updateMouseButtonHints(){
    switch (getStatus()) {
        case SetEntity1:
            updateMouseWidgetTRBack(tr("Specify first entity or enter radius <%1>").arg(getRadius()));
            break;
        case SetEntity2:
            updateMouseWidgetTRBack(tr("Specify second entity"));
            break;
        case SetRadius:
            updateMouseWidgetTRCancel(tr("Enter radius:"));
            break;
            /*case SetTrim:
                        RS_DIALOGFACTORY->updateMouseWidget(tr("Trim on? (yes/no):"),
                                                            "");
                        break;*/
        default:
            updateMouseWidget();
            break;
    }
}

LC_ActionOptionsWidget* RS_ActionModifyRound::createOptionsWidget(){
    return new QG_RoundOptions();
}

RS2::CursorType RS_ActionModifyRound::doGetMouseCursor([[maybe_unused]] int status){
    return RS2::SelectCursor;
}
