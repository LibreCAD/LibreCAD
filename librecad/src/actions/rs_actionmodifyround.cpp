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

#include <QList>
#include <QMouseEvent>

#include "rs_actionmodifyround.h"
#include "rs_commandevent.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_information.h"
#include "rs_math.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "qg_roundoptions.h"

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
        entity.getNearestEndpoint(point, &distance);
        return distance < RS_TOLERANCE;
    }

    bool atEndPoint(RS_Entity &entity1, RS_Entity &entity2, const RS_Vector &point){
        return atEndPoint(entity1, point) || atEndPoint(entity2, point);
    }
}
struct RS_ActionModifyRound::Points {
    RS_Vector coord1;
    RS_Vector coord2;
    RS_RoundData data{};
};

// fixme - review cases for rounding circles and arcs, it's weird enough
// fixme - potentially, it's better to support more fine grained trim mode that will control which entities should be trimmed (first, second, both)?

RS_ActionModifyRound::RS_ActionModifyRound(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Round Entities",
                               container, graphicView), pPoints(std::make_unique<Points>()), lastStatus(SetEntity1){
    setActionType(RS2::ActionModifyRound);
}

RS_ActionModifyRound::~RS_ActionModifyRound() = default;


void RS_ActionModifyRound::init(int status){
    RS_PreviewActionInterface::init(status);

    snapMode.clear();
    snapMode.restriction = RS2::RestrictNothing;
}

void RS_ActionModifyRound::finish(bool updateTB){
    RS_PreviewActionInterface::finish(updateTB);
}

/*
    Removes the old fillet, if it exists.

    - by Melwyn Francis Carlo.
*/
bool RS_ActionModifyRound::removeOldFillet(RS_Entity *e, const bool &isPolyline){
    if (!isArc(e) || entity1 == nullptr || entity2 == nullptr)
        return false;

    auto isChained = [this](const RS_Vector &point){
        return atEndPoint(*entity1, *entity2, point);
    };
    std::vector<RS_Vector> endPoints = {e->getStartpoint(), e->getEndpoint()};
    bool chained = std::all_of(endPoints.begin(), endPoints.end(), isChained);
    if (!chained)
        return false;

    if (!isPolyline)
        container->removeEntity(e);

    return true;
}

void RS_ActionModifyRound::trigger(){

    RS_DEBUG->print("RS_ActionModifyRound::trigger()");

    if (entity1 && entity1->isAtomic() &&
        entity2 && entity2->isAtomic()){

        deletePreview();

        bool foundPolyline = false;

        if ((entity1->getParent() != nullptr) && (entity2->getParent() != nullptr)){
            if (isPolyline(entity1->getParent()) &&
                isPolyline(entity2->getParent()) &&
                (entity1->getParent() == entity2->getParent())){
                foundPolyline = true;

                for (auto *e: entity1->getParent()->getEntityList()) {
                    if ((e != entity1) && (e != entity2)){
                        if (removeOldFillet(e, foundPolyline)){
                            entity1->getParent()->removeEntity(e);
                            break;
                        }
                    }
                }
            }

            if (!foundPolyline){
                for (auto *e: graphicView->getContainer()->getEntityList()) {
                    if ((e != entity1) && (e != entity2)){
                        if (removeOldFillet(e, foundPolyline))
                            break;
                    }
                }
            }
        }

        RS_Modification m(*container, graphicView);
        m.round(pPoints->coord2,
                pPoints->coord1,
                (RS_AtomicEntity *) entity1,
                pPoints->coord2,
                (RS_AtomicEntity *) entity2,
                pPoints->data);

        //coord = RS_Vector(false);
        pPoints->coord1 = RS_Vector(false);
        entity1 = nullptr;
        pPoints->coord2 = RS_Vector(false);
        entity2 = nullptr;
        // fixme - decide to which state go after trigger - probably it's more convenient to say in SetEntity2?
        setStatus(SetEntity1);
        graphicView->redraw();
        updateSelectionWidget();
    }
}

void RS_ActionModifyRound::mouseMoveEvent(QMouseEvent *e){
    RS_DEBUG->print("RS_ActionModifyRound::mouseMoveEvent begin");

    RS_Vector mouse = toGraph(e);
    RS_Entity *se = catchEntity(e, eType, RS2::ResolveAllButTextImage);
    deleteHighlights();
    deletePreview();

    switch (getStatus()) {
        case SetEntity1: {
            if (se != nullptr){
                if (RS_Information::isTrimmable(se)){
                    highlightHover(se);
                }
            }
            break;
        }
        case SetEntity2: {
            highlightSelected(entity1);
            if (se != nullptr){
                if (entity1 != se && RS_Information::isTrimmable(se) && se->isAtomic()){

                    RS_Vector coord2 = se->getNearestPointOnEntity(mouse, true);
                    RS_Entity *tmp1 = entity1->clone();
                    RS_Entity *tmp2 = se->clone();
                    tmp1->reparent(preview.get());
                    tmp2->reparent(preview.get());
                    previewEntity(tmp1);
                    previewEntity(tmp2);

                    bool trim = pPoints->data.trim;
//                    pPoints->data.trim = false;
                    RS_Modification m(*preview, nullptr, false);
                    LC_RoundResult *roundResult = m.round(mouse,
                                                          pPoints->coord1,
                                                          (RS_AtomicEntity *) tmp1,
                                                          coord2,
                                                          (RS_AtomicEntity *) tmp2,
                                                          pPoints->data);

                    if (roundResult != nullptr && roundResult->error == LC_RoundResult::OK){
                        highlightHover(se);
                        auto *arc = roundResult->round;
                        if (arc != nullptr){

                            RS_Vector arcStartPoint = arc->getStartpoint();
                            RS_Vector arcEndPoint = arc->getEndpoint();
                            if (showRefEntitiesOnPreview) {
                                previewRefPoint(arcStartPoint);
                                previewRefPoint(arcEndPoint);
                                previewRefPoint(pPoints->coord1);
                                previewRefSelectablePoint(coord2);
                                previewRefPoint(mouse);
                                previewRefLine(mouse, coord2);
                                if (trim){
                                    previewEntityModifications(entity1, roundResult->trimmed1, arcStartPoint, roundResult->trim1Mode);
                                    previewEntityModifications(se, roundResult->trimmed2, arcEndPoint, roundResult->trim2Mode);
                                }
                            }
                            if (trim){
                                preview->removeEntity(roundResult->trimmed1);
                                preview->removeEntity(roundResult->trimmed2);
                            }
                        }
                    }

                    pPoints->data.trim = trim;

                    preview->removeEntity(tmp1);
                    preview->removeEntity(tmp2);

                }
            }
            break;
        }
        default:
            break;
    }
    drawPreview();
    drawHighlights();

    RS_DEBUG->print("RS_ActionModifyRound::mouseMoveEvent end");
}

void RS_ActionModifyRound::previewEntityModifications(const RS_Entity *original, RS_Entity *modified, RS_Vector& roundPoint, int mode){
    bool decreased = modified->getLength() < original->getLength();
    if (isLine(modified)){
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
                if (showRefEntitiesOnPreview) {
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
    else if (this->isArc(modified)){
            auto* newArc = dynamic_cast<RS_Arc*>(modified);
            RS_ArcData arcData = newArc->getData();
            std::swap(arcData.angle1, arcData.angle2);
            previewRefArc(arcData);
    }
}

void RS_ActionModifyRound::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_Vector mouse = toGraph(e);
    RS_Entity *se = catchEntity(e, eType, RS2::ResolveAll);
    switch (status) {
        case SetEntity1: {
            if (se && se->isAtomic() &&
                RS_Information::isTrimmable(se)){
                entity1 = se;
                pPoints->coord1 = se->getNearestPointOnEntity(mouse, true);
                setStatus(SetEntity2);
            }
            break;
        }
        case SetEntity2: {
            if (se && se->isAtomic() &&
                RS_Information::isTrimmable(entity1, se)){
                entity2 = se;
                pPoints->coord2 = mouse;/* se->getNearestPointOnEntity(mouse, true);*/
                //setStatus(ChooseRounding);
                trigger();
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyRound::onMouseRightButtonRelease(int status, [[maybe_unused]]QMouseEvent *e) {
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
                lastStatus = (Status) status;
                setStatus(SetRadius);
                accept = true;
            } else if (checkCommand("trim", c)){
                deletePreview();
                lastStatus = (Status) status;
                setStatus(SetTrim);
                pPoints->data.trim = !pPoints->data.trim;
                updateOptions();
                accept = true;
            } else {
                bool ok;
                double r = RS_Math::eval(c, &ok);
                if (ok && r > 1.0e-10){
                    accept = true;
                    pPoints->data.radius = r;

                } else {
                    commandMessage(tr("Not a valid expression"));
                }
                // fixme - should we allow change status for invalid input?
                updateOptions();
                setStatus(lastStatus);
            }
            break;
        }
        case SetRadius: {
            bool ok;
            double r = RS_Math::eval(c, &ok);
            if (ok){
                accept = true;
                pPoints->data.radius = r;
            } else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            setStatus(lastStatus);
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
    pPoints->data.radius = r;
}

double RS_ActionModifyRound::getRadius() const{
    return pPoints->data.radius;
}

void RS_ActionModifyRound::setTrim(bool t){
    pPoints->data.trim = t;
}

bool RS_ActionModifyRound::isTrimOn() const{
    return pPoints->data.trim;
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
