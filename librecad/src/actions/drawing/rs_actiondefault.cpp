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
#include <algorithm>

#include<QMouseEvent>

#include "qc_applicationwindow.h"
#include "rs_actiondefault.h"
#include "rs_commandevent.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_overlaybox.h"
#include "rs_preview.h"
#include "rs_selection.h"
#include "rs_units.h"
#include "rs_actioninterface.h"

struct RS_ActionDefault::Points {
    RS_Vector v1;
    RS_Vector v2;
    RS_Entity *highlightedEntity = nullptr;
    RS_Entity* refMovingEntity = nullptr;
};

namespace {

// Glowing effects on Mouse hover
    constexpr double minimumHoverTolerance = 3.0;
    constexpr double hoverToleranceFactor1 = 1.0;
    constexpr double hoverToleranceFactor2 = 10.0;

// whether the entity supports glowing effects on mouse hovering
    bool allowMouseOverGlowing(const RS_Entity *entity){
        if (entity == nullptr)
            return false;
        switch (entity->rtti()) {
            case RS2::EntityHatch:
            case RS2::EntityImage:
            case RS2::EntitySolid:
            case RS2::EntityUnknown:
            case RS2::EntityPattern:
                return false;
            default:
                return true;
        }
    }
}

/**
 * Constructor.
 */
RS_ActionDefault::RS_ActionDefault(
    RS_EntityContainer &container,
    RS_GraphicView &graphicView)
    :RS_PreviewActionInterface("Default",
                               container, graphicView), pPoints(std::make_unique<Points>()), snapRestriction(RS2::RestrictNothing){

    RS_DEBUG->print("RS_ActionDefault::RS_ActionDefault");
    setActionType(RS2::ActionDefault);
    typeToSelect = graphicView.getTypeToSelect();
    RS_DEBUG->print("RS_ActionDefault::RS_ActionDefault: OK");
}

RS_ActionDefault::~RS_ActionDefault() = default;


void RS_ActionDefault::init(int status){
    RS_DEBUG->print("RS_ActionDefault::init");
    if (status >= 0){
        checkSupportOfQuickEntityInfo();
    }
    if (status == Neutral){
        deletePreview();
        deleteSnapper();
    }
    RS_PreviewActionInterface::init(status);
    pPoints->v1 = pPoints->v2 = {};
    //    snapMode.clear();
    //    snapMode.restriction = RS2::RestrictNothing;
    //    restrBak = RS2::RestrictNothing;
    //        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);

    RS_DEBUG->print("RS_ActionDefault::init: OK");
}

void RS_ActionDefault::checkSupportOfQuickEntityInfo(){
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        this->allowEntityQuickInfoAuto = entityInfoWidget->isAutoSelectEntitiesInDefaultAction();
        this->allowEntityQuickInfoForCTRL = entityInfoWidget->isSelectEntitiesInDefaultActionWithCTRL();
    }
}

void RS_ActionDefault::keyPressEvent(QKeyEvent *e){
    //        std::cout<<"RS_ActionDefault::keyPressEvent(): begin"<<std::endl;
    switch (e->key()) {
        case Qt::Key_Shift:
            snapRestriction = snapMode.restriction;
            setSnapRestriction(RS2::RestrictOrthogonal);
            e->accept();
            break; //avoid clearing command line at shift key
            //cleanup default action, issue#285
        case Qt::Key_Escape:
            //        std::cout<<"RS_ActionDefault::keyPressEvent(): Qt::Key_Escape"<<std::endl;
            deletePreview();
            deleteSnapper();
            setStatus(Neutral);
            e->accept();
            break;
        default:
            e->ignore();
    }

}

void RS_ActionDefault::keyReleaseEvent(QKeyEvent *e){
    if (e->key() == Qt::Key_Shift){
        setSnapRestriction(snapRestriction);
        e->accept();
    }
}

/*
   Highlights hovered entities that are visible and not locked.

   - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
void RS_ActionDefault::highlightHoveredEntities(QMouseEvent *event){
//    clearHighLighting();

    bool shouldShowQuickInfoWidget = allowEntityQuickInfoAuto || (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier) && allowEntityQuickInfoForCTRL);
    bool showHighlightEntity = highlightEntitiesOnHover || shouldShowQuickInfoWidget;
    if (!showHighlightEntity)
        return;

    RS_Entity *entity = catchEntity(event);
    if (entity == nullptr)
        return;
    if (!entity->isVisible() || (entity->isLocked() && !shouldShowQuickInfoWidget)){
        return;
    }

    const double hoverToleranceFactor = (entity->rtti() == RS2::EntityEllipse)
                                        ? hoverToleranceFactor1
                                        : hoverToleranceFactor2;

    const double hoverTolerance{hoverToleranceFactor / graphicView->getFactor().magnitude()};

    double hoverTolerance_adjusted = ((entity->rtti() != RS2::EntityEllipse) && (hoverTolerance < minimumHoverTolerance))
                                     ? minimumHoverTolerance
                                     : hoverTolerance;

    double screenTolerance = graphicView->toGraphDX((int)(0.01 * std::min(graphicView->getWidth(), graphicView->getHeight())));
    hoverTolerance_adjusted = std::min(hoverTolerance_adjusted, screenTolerance);
    bool isPointOnEntity = false;
    RS_Vector currentMousePosition = toGraph(event);
    if (((entity->rtti() >= RS2::EntityDimAligned) && (entity->rtti() <= RS2::EntityDimLeader))
        || (entity->rtti() == RS2::EntityText) || (entity->rtti() == RS2::EntityMText)){
        double nearestDistanceTo_pointOnEntity = 0.;

        entity->getNearestPointOnEntity(currentMousePosition, true, &nearestDistanceTo_pointOnEntity);

        if (nearestDistanceTo_pointOnEntity <= hoverTolerance_adjusted) isPointOnEntity = true;
    } else {
        isPointOnEntity = entity->isPointOnEntity(currentMousePosition, hoverTolerance_adjusted);
    }

    // Glowing effect on mouse hovering
    if (isPointOnEntity){
        highlightEntity(entity);
        if (shouldShowQuickInfoWidget){
            updateQuickInfoWidget(entity);
        }
    }
}

void RS_ActionDefault::mouseMoveEvent(QMouseEvent *e){

    RS_Vector mouse = toGraph(e);

    updateCoordinateWidgetByRelZero(mouse);

    // clear any existing hovering
    clearHighLighting();

    switch (getStatus()) {
        case Neutral: {
            deleteSnapper();
            highlightHoveredEntities(e);
            drawHighlights();
            break;
        }
        case Dragging:{
            pPoints->v2 = mouse;

            if (graphicView->toGuiDX(pPoints->v1.distanceTo(pPoints->v2)) > 10){
                // look for reference points to drag:
                double dist;
                RS_EntityContainer::RefInfo refInfo = container->getNearestSelectedRefInfo(pPoints->v1, &dist);
                RS_Vector ref = refInfo.ref;
                if (ref.valid == true && graphicView->toGuiDX(dist) < 8){
                    RS_DEBUG->print("RS_ActionDefault::mouseMoveEvent: moving reference point");
                    pPoints->refMovingEntity = refInfo.entity;
                    pPoints->v1 = ref;
                    moveRelativeZero(pPoints->v1);
                    setStatus(MovingRef);
                } else {
                    // test for an entity to drag:
                    RS_Entity *en = catchEntity(pPoints->v1);
                    if (en && en->isSelected()){
                        RS_DEBUG->print("RS_ActionDefault::mouseMoveEvent: moving entity");
                        RS_Vector vp = en->getNearestRef(pPoints->v1);
                        if (vp.valid) {
                            pPoints->v1 = vp;
                        }
                        graphicView->moveRelativeZero(pPoints->v1);
                        setStatus(Moving);
                    }
                    // no entity found. start area selection:
                    else {
                        setStatus(SetCorner2);
                    }
                }
            }
            break;
        }
        case MovingRef: {
            mouse = snapPoint(e);

            deletePreview();
            deleteHighlights();

            // additional processing for moving endpoint of lines and arcs
            bool ctrlPressed = isControl(e);
            bool addClone = true;

            RS_Entity *refMovingEntity = pPoints->refMovingEntity;
            RS2::EntityType type = refMovingEntity->rtti();
            switch (type) {
                case RS2::EntityLine: {
                    auto *refMovingLine = dynamic_cast<RS_Line *>(refMovingEntity);
                    RS_Vector basePoint;
                    if (refMovingLine->getStartpoint() == pPoints->v1){
                        basePoint = refMovingLine->getEndpoint();
                    }
                    else {
                        basePoint = refMovingLine->getStartpoint();
                    }
                    mouse = getSnapAngleAwarePoint(e, basePoint, mouse, true);
                    if (ctrlPressed){ // - if CTRL pressed, move endpoint with saving current angle of line
                        RS_ConstructionLine constructionLine = RS_ConstructionLine(nullptr,
                            RS_ConstructionLineData(refMovingLine->getStartpoint(),
                                refMovingLine->getEndpoint()));
                        RS_Vector newEndpoint = constructionLine.getNearestPointOnEntity(mouse, false);
                        pPoints->v2 = newEndpoint;
                    }
                    else{
                        pPoints->v2 = mouse;
                        previewRefLine(pPoints->v2, pPoints->v1);
                    }
                    break;
                }
                case RS2::EntityArc: {
                    auto *refMovingArc = dynamic_cast<RS_Arc *>(refMovingEntity);
                    auto *clone = dynamic_cast<RS_Arc *>(refMovingArc->clone());

                    if (ctrlPressed){ // for arc, we just correct angle of enpoint without changing the center and radius - if we move endpoint ref
                        mouse = getSnapAngleAwarePoint(e, refMovingArc->getCenter(), mouse, true);
                        if (refMovingArc->getStartpoint() == pPoints->v1){
                            clone->trimStartpoint(mouse);
                            pPoints->v2 = clone->getStartpoint();
                            preview->addEntity(clone);
                            if (showRefEntitiesOnPreview) {
                                previewRefPoint(clone->getCenter());
                            }
                            addClone = false;
                        } else if (refMovingArc->getEndpoint() == pPoints->v1){
                            clone->trimEndpoint(mouse);
                            pPoints->v2 = clone->getEndpoint();
                            preview->addEntity(clone);
                            if (showRefEntitiesOnPreview) {
                                previewRefPoint(clone->getCenter());
                            }
                            addClone = false;
                        }
                        else{ // center
                            pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                            if (showRefEntitiesOnPreview) {
                                previewRefLine(pPoints->v2, pPoints->v1);
                            }
                        }
                        if (!addClone){
                            if (showRefEntitiesOnPreview) {
                                previewRefLine(refMovingArc->getCenter(), pPoints->v2);
                                previewRefPoint(refMovingArc->getCenter());
                            }
                        }
                    }
                    else{
                        mouse = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                        pPoints->v2 = mouse;
                        clone->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);
                        if (showRefEntitiesOnPreview) {
                            previewRefLine(pPoints->v2, pPoints->v1);
                            previewRefPoint(clone->getCenter());
                        }
                        preview->addEntity(clone);
                        addClone = false;
                    }
                    break;
                }
                // FIXME - add additional processing for dimensions to ensure snapping of dimension lines (same as for creation of dims)
                default: {
                    pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                    if (showRefEntitiesOnPreview) {
                        previewRefLine(pPoints->v2, pPoints->v1);
                    }
                    break;
                }
            }

            updateCoordinateWidgetByRelZero(pPoints->v2);

            if (addClone){
                RS_Entity* clone = refMovingEntity->clone();
                clone->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);
                preview->addEntity(clone);
            }

            if (showRefEntitiesOnPreview) {
                previewRefSelectablePoint(pPoints->v2);
                previewRefPoint(pPoints->v1);
            }

            drawPreview();
            drawHighlights();
            break;
        }
        case Moving: {
            deletePreview();
            mouse = snapPoint(e);
            pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
            updateCoordinateWidgetByRelZero(pPoints->v2);

            preview->addSelectionFrom(*container);
            preview->move(pPoints->v2 - pPoints->v1);

            auto *line = new RS_Line(pPoints->v1, pPoints->v2);
            preview->addEntity(line);
            if (showRefEntitiesOnPreview) {
                previewRefLine(pPoints->v1, pPoints->v2);
                previewRefPoint(pPoints->v1);
                previewRefSelectablePoint(pPoints->v2);
            }
            line->setSelected(true);

            drawPreview();
            break;
        }
        case SetCorner2: {
            if (pPoints->v1.valid){
                pPoints->v2 = mouse;

                deletePreview();

                auto ob = new RS_OverlayBox(preview.get(),
                                            RS_OverlayBoxData(pPoints->v1, pPoints->v2));
                preview->addEntity(ob);

                drawPreview();
            }
            break;
        }
        case Panning: {
            RS_Vector const vTarget{e->position()};
            RS_Vector const v01 = vTarget - pPoints->v1;
            if (v01.squared() >= 64.){
                graphicView->zoomPan((int) v01.x, (int) v01.y);
                pPoints->v1 = vTarget;
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDefault::mousePressEvent(QMouseEvent *e){
    if (e->button() == Qt::LeftButton){
        switch (getStatus()) {
            case Neutral: {
                auto const m = e->modifiers();
                if (m & (Qt::ControlModifier | Qt::MetaModifier)){
                    pPoints->v1 = RS_Vector{e->position()};
                    setStatus(Panning);
                } else {
                    pPoints->v1 = toGraph(e);
                    setStatus(Dragging);
                }
                break;
            }
            case Moving: {
                pPoints->v2 = snapPoint(e);
                pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, pPoints->v2);
                deletePreview();
                RS_Modification m(*container, graphicView);
                RS_MoveData data;
                data.number = 0;
                data.useCurrentLayer = false;
                data.useCurrentAttributes = false;
                data.offset = pPoints->v2 - pPoints->v1;
                m.move(data);
                goToNeutralStatus();
                updateSelectionWidget();
                deleteSnapper();
                break;
            }
            case MovingRef: {
                /*
                pPoints->v2 = snapPoint(e);
                if (e->modifiers() & Qt::ShiftModifier){
                    pPoints->v2 = snapToAngle(pPoints->v2, pPoints->v1);
                }
                deletePreview();
                RS_Modification m(*container, graphicView);
                RS_MoveRefData data;
                data.ref = pPoints->v1;
                data.offset = pPoints->v2 - pPoints->v1;
                m.moveRef(data);
                */

                RS_Vector mouse = snapPoint(e);

                deletePreview();

                // additional processing for moving endpoint of lines and arcs
                bool ctrlPressed = isControl(e);
                bool moveRefOnClone = true;

                RS_Entity *refMovingEntity = pPoints->refMovingEntity;
                RS_Entity *clone = refMovingEntity->clone();
                RS2::EntityType type = refMovingEntity->rtti();
                switch (type) {
                    case RS2::EntityLine: {
                        auto *refMovingLine = dynamic_cast<RS_Line *>(refMovingEntity);
                        RS_Vector basePoint;
                        if (refMovingLine->getStartpoint() == pPoints->v1) {
                            basePoint = refMovingLine->getEndpoint();
                        } else {
                            basePoint = refMovingLine->getStartpoint();
                        }
                        mouse = getSnapAngleAwarePoint(e, basePoint, mouse);
                        if (ctrlPressed) {
                            // - if CTRL pressed, move endpoint with saving current angle of line
                            RS_ConstructionLine constructionLine = RS_ConstructionLine(nullptr,
                                RS_ConstructionLineData(refMovingLine->getStartpoint(),
                                                        refMovingLine->getEndpoint()));
                            RS_Vector newEndpoint = constructionLine.getNearestPointOnEntity(mouse, false);
                            pPoints->v2 = newEndpoint;
                        } else {
                            pPoints->v2 = mouse;
                        }
                        break;
                    }
                    case RS2::EntityArc: {
                        auto *refMovingArc = dynamic_cast<RS_Arc *>(refMovingEntity);
                        auto *arcClone = dynamic_cast<RS_Arc *>(clone);

                        if (ctrlPressed) {
                            // for arc, we just correct angle of enpoint without changing the center and radius - if we move endpoint ref
                            mouse = getSnapAngleAwarePoint(e, refMovingArc->getCenter(), mouse, false);
                            if (refMovingArc->getStartpoint() == pPoints->v1) {
                                arcClone->trimStartpoint(mouse);
                                pPoints->v2 = arcClone->getStartpoint();

                                moveRefOnClone = false;
                            } else if (refMovingArc->getEndpoint() == pPoints->v1) {
                                arcClone->trimEndpoint(mouse);
                                pPoints->v2 = arcClone->getEndpoint();
                                moveRefOnClone = false;
                            } else {
                                // center
                                pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                            }
                        } else {
                            mouse = getSnapAngleAwarePoint(e, pPoints->v1, mouse, false);
                            pPoints->v2 = mouse;
                            arcClone->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);
                            moveRefOnClone = false;
                        }
                        break;
                    }
                    default: {
                        pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, false);
                        break;
                    }
                }

                if (moveRefOnClone) {
                    clone->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);
                }

                if (document) {
                    document->startUndoCycle();

                    clone->setSelected(true);

                    clone->setLayer(refMovingEntity->getLayer());
                    clone->setPen(refMovingEntity->getPen());
                    container->addEntity(clone);
                    document->addUndoable(clone);

                    // delete and add this into undo
                    deleteEntityUndoable(refMovingEntity);

                    document->endUndoCycle();
                }
                graphicView->redraw();

                goToNeutralStatus();
                updateSelectionWidget();
                break;
            }
            default:
                break;
        }
    } else if (e->button() == Qt::RightButton){
        //cleanup
        goToNeutralStatus();
        e->accept();
    }
}

void RS_ActionDefault::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionDefault::mouseReleaseEvent()");
    pPoints->v2 = toGraph(e);
    switch (status) {
        case Dragging: {
            // select single entity:
            RS_Entity *en = catchEntity(e);

            if (en != nullptr){
                deletePreview();

                RS_Selection s(*container, graphicView);

                s.selectSingle(en);

                updateSelectionWidget();

                e->accept();

                goToNeutralStatus();
            } else {
                setStatus(SetCorner2);
            }
            break;
        }
        case SetCorner2: {
            //v2 = snapPoint(e);
            pPoints->v2 = toGraph(e);

            // select window:
            //if (graphicView->toGuiDX(v1.distanceTo(v2))>20) {
            deletePreview();

            bool cross = (pPoints->v1.x > pPoints->v2.x);
            RS_Selection s(*container, graphicView);
            bool select = (e->modifiers() & Qt::ShiftModifier) == 0;
            s.selectWindow(typeToSelect, pPoints->v1, pPoints->v2, select, cross);

            updateSelectionWidget();

            goToNeutralStatus();
            e->accept();
            //}
            break;
        }
        case Panning:
            goToNeutralStatus();
            break;
        default:
            break;
    }
}

void RS_ActionDefault::onMouseRightButtonRelease([[maybe_unused]]int status, QMouseEvent *e) {
    RS_DEBUG->print("RS_ActionDefault::mouseReleaseEvent()");
    //cleanup
    goToNeutralStatus();
    e->accept();
}

void RS_ActionDefault::goToNeutralStatus(){
    this->deletePreview();
    this->deleteHighlights();
    this->drawPreview();
    this->drawHighlights();
    this->setStatus(Neutral);
}

// fixme - review and cleanup
void RS_ActionDefault::commandEvent( [[maybe_unused]] RS_CommandEvent *e){
//    QString c = e->getCommand().toLower();

    // if the current action can't deal with the command,
    //   it might be intended to launch a new command
    //if (!e.isAccepted()) {
    // command for new action:
    //RS2::ActionType type = RS_COMMANDS->cmdToAction(c);
    //if (type!=RS2::ActionNone) {
    //graphicView->setCurrentAction(type);
    //return true;
    //}
    //}
}

QStringList RS_ActionDefault::getAvailableCommands(){
    QStringList cmd;

    //cmd += "line";
    //cmd += "rectangle";

    return cmd;
}

void RS_ActionDefault::updateMouseButtonHints(){
    switch (getStatus()) {
        case Moving:{
            updateMouseWidgetTRCancel(tr("Set new position"), MOD_SHIFT_ANGLE_SNAP);
            break;
        }
        case MovingRef: {
            LC_ModifiersInfo modifiers;
            RS2::EntityType rtti = pPoints->refMovingEntity->rtti();
            switch (rtti){
                case RS2::EntityLine:
                    modifiers =  MOD_SHIFT_AND_CTRL_ANGLE(tr("Lengthen Line"));
                    break;
                case RS2::EntityArc:
                    modifiers =  MOD_SHIFT_AND_CTRL_ANGLE(tr("Lengthen Arc"));
                    break;
                default:
                    modifiers = MOD_SHIFT_ANGLE_SNAP;
            }
            updateMouseWidget(tr("Set new ref position"), "", modifiers);
            break;
        }
        case Neutral: {
            updateMouseWidget(tr(""), "",  MOD_SHIFT_AND_CTRL(tr("Scroll horizontally"), tr("Scroll Vertically")));
            break;
        }
        case SetCorner2: {
            updateMouseWidgetTRBack(tr("Choose second edge"));
            break;
        }
        default: {
            updateMouseWidget();
        }
    }
}

RS2::CursorType RS_ActionDefault::doGetMouseCursor(int status){
    switch (status) {
        case Neutral:
            return RS2::ArrowCursor;
        case Moving:
        case MovingRef:
            return RS2::SelectCursor;
        case Panning:
            return RS2::ClosedHandCursor;
        default:
            return RS2::NoCursorChange;
    }
}

void RS_ActionDefault::clearHighLighting(){
    deleteHighlights();
    clearQuickInfoWidget();
}

void RS_ActionDefault::resume(){
    clearHighLighting();
    checkSupportOfQuickEntityInfo();
    BASE_CLASS::resume();
}

void RS_ActionDefault::suspend(){
    clearHighLighting();
    BASE_CLASS::suspend();
}

void RS_ActionDefault::highlightEntity(RS_Entity *entity){
    if (!allowMouseOverGlowing(entity))
        return;
    highlightHover(entity);
}

RS2::EntityType RS_ActionDefault::getTypeToSelect(){
    return typeToSelect;
}

void RS_ActionDefault::clearQuickInfoWidget(){
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
//        entityInfoWidget->processEntity(nullptr);
    }
}

void RS_ActionDefault::updateQuickInfoWidget(RS_Entity *pEntity){
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        entityInfoWidget->processEntity(pEntity);
    }
}
