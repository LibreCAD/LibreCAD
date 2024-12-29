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
#include "lc_linemath.h"
#include "rs_snapper.h"

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
    clearMouseWidgetIcon();
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
            deleteSnapper();
            goToNeutralStatus();
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

    bool controlPressed = isControl(event);

    bool shouldShowQuickInfoWidget = allowEntityQuickInfoAuto || (controlPressed && allowEntityQuickInfoForCTRL);
    bool showHighlightEntity = highlightEntitiesOnHover || shouldShowQuickInfoWidget;
    bool showEntityDescriptions = isShowEntityDescriptionOnHighlight();

    if (!showHighlightEntity && !showEntityDescriptions)
        return;

    RS_Entity *entity = nullptr;

    if (controlPressed){
        entity = catchEntity(event, RS2::ResolveAll);
    }
    else{
        entity = catchEntity(event);
    }
    if (entity == nullptr) {
        infoCursorOverlayData.setZone2("");
        return;
    }
    if (!entity->isVisible()){
        return;
    }

    if (showEntityDescriptions){
        QString entityInfoStr = obtainEntityDescriptionForInfoCursor(entity, RS2::EntityDescriptionLevel::DescriptionLong);
        if (!entityInfoStr.isEmpty()) {
            infoCursorOverlayData.setZone2(entityInfoStr);
            forceUpdateInfoCursor(event);
        }
    }

    if ((entity->isLocked() && !shouldShowQuickInfoWidget)){
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

void RS_ActionDefault::forceUpdateInfoCursor(const QMouseEvent *event) {
    const RS_Vector pos = toGraph(event);
    RS_Snapper::forceUpdateInfoCursor(pos);
}

bool RS_ActionDefault::isShowEntityDescriptionOnHighlight(){
    return graphicView->isShowEntityDescriptionOnHover() && (infoCursorOverlayPrefs != nullptr && infoCursorOverlayPrefs->enabled);
}

void RS_ActionDefault::mouseMoveEvent(QMouseEvent *e){
    deletePreview();
    deleteHighlights();

    RS_Vector mouse = toGraph(e);

    updateCoordinateWidgetByRelZero(mouse);

    // clear any existing hovering
    clearHighLighting();

    switch (getStatus()) {
        case Neutral: {
            deleteSnapper();
            highlightHoveredEntities(e);
            if (infoCursorOverlayPrefs->enabled){
                if (!isShowEntityDescriptionOnHighlight()) {
                    infoCursorOverlayData.setZone2("");
                }
                RS_Snapper::forceUpdateInfoCursor(mouse);
            }
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
                        moveRelativeZero(pPoints->v1);
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

            // additional processing for moving endpoint of lines and arcs
            bool ctrlPressed = isControl(e);
            bool shiftPressed = isShift(e);
            bool addClone = true;

            RS_Entity *refMovingEntity = pPoints->refMovingEntity;
            RS2::EntityType type = refMovingEntity->rtti();

            // FIXME - SAND - UPDATE ENTITIES PROPERTIES ON MOVE (like chord len, radius of arc etc!) where it's applicable

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
                    if (isInfoCursorForModificationEnabled()) {
                        createEditedLineDescription(nullptr, ctrlPressed, shiftPressed);
                    }
                    break;
                }
                case RS2::EntityArc: {
                    auto *refMovingArc = dynamic_cast<RS_Arc *>(refMovingEntity);
                    auto *clone = dynamic_cast<RS_Arc *>(refMovingArc->cloneProxy(graphicView));

                    const RS_Vector &arcCenter = refMovingArc->getCenter();
                    const RS_Vector &arcMiddle = refMovingArc->getMiddlePoint();
                    const RS_Vector &arcEnd = refMovingArc->getEndpoint();
                    const RS_Vector &arcStart = refMovingArc->getStartpoint();
                    if (ctrlPressed){ // for arc, we just correct angle of endpoint without changing the center and radius - if we move endpoint ref
                        mouse = getSnapAngleAwarePoint(e, arcCenter, mouse, true);
                        if (arcStart == pPoints->v1){
                            clone->trimStartpoint(mouse);
                            pPoints->v2 = clone->getStartpoint();
                            preview->addEntity(clone);
                            if (showRefEntitiesOnPreview) {
                                previewRefPoint(clone->getCenter());
                            }
                            addClone = false;
                        } else if (arcEnd == pPoints->v1){
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
                            clone->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);
                            preview->addEntity(clone);
                            if (showRefEntitiesOnPreview) {
                                previewRefLine(pPoints->v2, pPoints->v1);
                            }
                            addClone = false;
                        }
                        if (!addClone){
                            if (showRefEntitiesOnPreview) {
                                previewRefLine(arcCenter, pPoints->v2);
                                previewRefPoint(arcCenter);
                            }
                        }
                    }
                    else{
                        bool referenceMoved = false;                        
                        if (arcMiddle == pPoints->v1){ // middle point processing
                            mouse = LC_LineMath::getNearestPointOnInfiniteLine(mouse, arcCenter, arcMiddle);
                            if (shiftPressed) { // do scaling
                                double fromMouse = arcCenter.distanceTo(mouse);
                                double radius = refMovingArc->getRadius();
                                double scaleFactor = fromMouse / radius;
                                double fromMiddle = arcMiddle.distanceTo(mouse);
                                if (fromMiddle > radius){
                                    if (arcCenter.angleTo(arcMiddle) !=
                                        arcCenter.angleTo(mouse)) { // checking direction of mouse relating to center (own for counter-part of circle)
                                        scaleFactor = -scaleFactor;
                                    }
                                }

                                clone->scale(arcCenter, RS_Vector(scaleFactor, scaleFactor));
                                preview->addEntity(clone);
                                referenceMoved = true;
                            }
                            else { // change radius
                                if (showRefEntitiesOnPreview) {
                                    previewRefSelectablePoint(mouse);
                                    previewRefLine(arcCenter, clone->getMiddlePoint());
                                }
                            }
                        }
                        else {
                            if (shiftPressed) {
                                if (arcEnd == pPoints->v1 ||
                                    arcStart == pPoints->v1) { // change chord length
                                    mouse = LC_LineMath::getNearestPointOnInfiniteLine(mouse, arcStart, arcEnd);
                                }
                            } else {
                                mouse = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                            }
                        }
                        pPoints->v2 = mouse;
                        if (!referenceMoved) {
                            clone->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);
                            preview->addEntity(clone);
                        }

                        if (showRefEntitiesOnPreview) {
                            previewRefLine(pPoints->v2, pPoints->v1);
                            previewRefPoint(clone->getCenter());
                        }
                        addClone = false;
                    }

                    if (isInfoCursorForModificationEnabled()) {
                        createEditedArcDescription(clone, ctrlPressed, shiftPressed);
                    }
                    break;
                }
                case RS2::EntityPolyline:{
                    if (shiftPressed || ctrlPressed){
                        auto* polyline = static_cast<RS_Polyline *>(refMovingEntity);
                        RS_Vector directionPoint = polyline->getRefPointAdjacentDirection(shiftPressed, pPoints->v1);
                        if (directionPoint.valid) {
                            pPoints->v2 = LC_LineMath::getNearestPointOnInfiniteLine(mouse, pPoints->v1, directionPoint);
                        }
                        else{
                            pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                        }
                    }
                    else{
                        pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                    }
                    if (showRefEntitiesOnPreview) {
                        previewRefLine(pPoints->v2, pPoints->v1);
                    }
                    break;
                }
                case RS2::EntityCircle:{
                    // fixme -sand - add morphing of circle to ellipse
                    auto *refMovingCircle = dynamic_cast<RS_Circle *>(refMovingEntity);
                    auto *clone = dynamic_cast<RS_Circle *>(refMovingCircle->cloneProxy(graphicView));
                    pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);

                    if (showRefEntitiesOnPreview) {
                        previewRefLine(pPoints->v2, pPoints->v1);
                    }
                    clone->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);
                    preview->addEntity(clone);
                    if (isInfoCursorForModificationEnabled()) {
                        createEditedCircleDescription(clone, ctrlPressed, shiftPressed);
                    }
                    addClone = false;
                    break;
                }
                case RS2::EntityEllipse:{
                    // fixme - sand - add rotation of major axis
                    pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                    if (showRefEntitiesOnPreview) {
                        previewRefLine(pPoints->v2, pPoints->v1);
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
                const RS_Vector &offset = pPoints->v2 - pPoints->v1;
                clone->moveRef(pPoints->v1, offset);
                preview->addEntity(clone);

                if (isInfoCursorForModificationEnabled()) {
                    LC_InfoMessageBuilder msg(tr("Offset"));
                    msg.add(formatRelative(offset));
                    msg.add(formatRelativePolar(offset));
                    msg.add(tr("New Position"), formatVector(offset));
                    appendInfoCursorZoneMessage(msg.toString(), 2, false);
                }
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
            mouse = snapPoint(e);
            pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
            updateCoordinateWidgetByRelZero(pPoints->v2);

            preview->addSelectionFrom(*container,graphicView);
            const RS_Vector &offset = pPoints->v2 - pPoints->v1;
            preview->move(offset);

            auto *line = new RS_Line(pPoints->v1, pPoints->v2);
            preview->addEntity(line);
            if (showRefEntitiesOnPreview) {
                previewRefLine(pPoints->v1, pPoints->v2);
                previewRefPoint(pPoints->v1);
                previewRefSelectablePoint(pPoints->v2);
            }
            line->setSelected(true);
            if (isInfoCursorForModificationEnabled()){
                QString type;
                if (isControl(e)) {
                   type = tr("Copy Offset");
                } else {
                    type = tr("Move Offset");
                }
                LC_InfoMessageBuilder msg(type);
                msg.add(formatRelative(offset));
                msg.add(formatRelativePolar(offset));
                appendInfoCursorZoneMessage(msg.toString(), 2, false);
            }
            break;
        }
        case SetCorner2: {
            if (pPoints->v1.valid){
                pPoints->v2 = mouse;
                auto ob = new RS_OverlayBox(preview.get(),
                                            RS_OverlayBoxData(pPoints->v1, pPoints->v2));
                previewEntity(ob);

                if (isInfoCursorForModificationEnabled()) {
                    bool cross = (pPoints->v1.x > pPoints->v2.x);
                    bool deselect = isShift(e);
                    QString msg = deselect ? tr("De-Selecting") : tr("Selecting");
                    msg.append(tr(" entities "));
                    msg.append(cross? tr("that intersect with box") : tr("that are within box"));
                    infoCursorOverlayData.setZone2(msg);
                    forceUpdateInfoCursor(e);
                }
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
    drawPreview();
    drawHighlights();
}

void RS_ActionDefault::createEditedLineDescription([[maybe_unused]]RS_Line* clone, [[maybe_unused]]bool ctrlPressed,  [[maybe_unused]]bool shiftPressed) {
    LC_InfoMessageBuilder msg(tr("Line"));
    msg.add(tr("Length: "), formatLinear(pPoints->v1.distanceTo(pPoints->v2)));
    msg.add(tr("Angle: "), formatAngle(pPoints->v1.angleTo(pPoints->v2)));
    appendInfoCursorZoneMessage(msg.toString(), 2, true);
}

void RS_ActionDefault::createEditedArcDescription(RS_Arc* clone,  [[maybe_unused]]bool ctrlPressed, [[maybe_unused]] bool shiftPressed) {
    RS_Vector center = clone->getCenter();
    RS_Line tmpLine = RS_Line(clone->getStartpoint(), clone->getEndpoint());
    double height = tmpLine.getDistanceToPoint(clone->getMiddlePoint());

    LC_InfoMessageBuilder msg(tr("Arc"));
    msg.add(tr("Radius:"),formatLinear(clone->getRadius()));
    msg.add(tr("Center:"),formatVector(center));
    msg.add(tr("Angle Length:"), formatAngle(clone->getAngleLength()));
    msg.add(tr("Chord Length:"), formatLinear(clone->getStartpoint().distanceTo(clone->getEndpoint())));
    msg.add(tr("Height:"), formatLinear(height));
    appendInfoCursorZoneMessage(msg.toString(), 2, true);
}

void RS_ActionDefault::createEditedCircleDescription(RS_Circle* clone,  [[maybe_unused]]bool ctrlPressed,  [[maybe_unused]]bool shiftPressed) {
    RS_Vector center = clone->getCenter();
    LC_InfoMessageBuilder msg(tr("Circle"));
    msg.add(tr("Radius:"),formatLinear(clone->getRadius()));
    msg.add(tr("Center:"),formatVector(center));
    appendInfoCursorZoneMessage(msg.toString(), 2, true);
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
                data.keepOriginals = isControl(e);
                data.offset = pPoints->v2 - pPoints->v1;
                m.move(data);
                goToNeutralStatus();
                updateSelectionWidget();
                deleteSnapper();
                break;
            }
            case MovingRef: {             
                RS_Vector mouse = snapPoint(e);

                deletePreview();

                // additional processing for moving endpoint of lines and arcs
                bool ctrlPressed = isControl(e);
                bool shiftPressed = isShift(e);
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

                        const RS_Vector &arcCenter = refMovingArc->getCenter();
                        const RS_Vector &arcStart = refMovingArc->getStartpoint();
                        const RS_Vector &arcEnd = refMovingArc->getEndpoint();
                        if (ctrlPressed) {
                            // for arc, we just correct angle of enpoint without changing the center and radius - if we move endpoint ref
                            mouse = getSnapAngleAwarePoint(e, arcCenter, mouse, false);
                            if (arcStart == pPoints->v1) {
                                arcClone->trimStartpoint(mouse);
                                pPoints->v2 = arcClone->getStartpoint();

                                moveRefOnClone = false;
                            } else if (arcEnd == pPoints->v1) {
                                arcClone->trimEndpoint(mouse);
                                pPoints->v2 = arcClone->getEndpoint();
                                moveRefOnClone = false;
                            } else {
                                // center
                                pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                            }
                        } else {
                            bool referenceMoved = false;
                            const RS_Vector &arcMiddle = refMovingArc->getMiddlePoint();
                            if (arcMiddle == pPoints->v1) {
                                mouse = LC_LineMath::getNearestPointOnInfiniteLine(mouse, arcCenter, arcMiddle);
                                if (shiftPressed) {
                                    double fromMouse = arcCenter.distanceTo(mouse);
                                    double radius = refMovingArc->getRadius();
                                    double scaleFactor = fromMouse / radius;
                                    double fromMiddle = arcMiddle.distanceTo(mouse);
                                    if (fromMiddle > radius) {
                                        // checking direction of mouse relating to center (own for counter-part of circle)
                                        if (arcCenter.angleTo(arcMiddle) !=
                                            arcCenter.angleTo(mouse)) {
                                            scaleFactor = -scaleFactor;
                                        }
                                    }
                                    arcClone->scale(arcCenter, RS_Vector(scaleFactor, scaleFactor));
                                    referenceMoved = true;
                                }
                                else{
                                    // change radius, default processing my move ref
                                }
                            }
                            else { // one of endpoint
                                if (shiftPressed){ // changing chord
                                    if (arcEnd == pPoints->v1 ||
                                        arcStart == pPoints->v1) { // change chord length
                                        mouse = LC_LineMath::getNearestPointOnInfiniteLine(mouse, arcStart, arcEnd);
                                    }
                                }
                                else{ // free change of endpoint position
                                    mouse = getSnapAngleAwarePoint(e, pPoints->v1, mouse, false);
                                }
                            }
                            if (!referenceMoved) {
                                pPoints->v2 = mouse;
                                arcClone->moveRef(pPoints->v1, pPoints->v2 - pPoints->v1);
                            }
                            moveRefOnClone = false;
                        }
                        break;
                    }
                    case RS2::EntityPolyline:{
                        if (shiftPressed || ctrlPressed){
                            auto* polyline = static_cast<RS_Polyline *>(refMovingEntity);
                            RS_Vector directionPoint = polyline->getRefPointAdjacentDirection(shiftPressed, pPoints->v1);
                            if (directionPoint.valid) {
                                pPoints->v2 = LC_LineMath::getNearestPointOnInfiniteLine(mouse, pPoints->v1, directionPoint);
                            }
                            else{
                                pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
                            }
                        }
                        else{
                            pPoints->v2 = getSnapAngleAwarePoint(e, pPoints->v1, mouse, true);
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
                    clone->setSelected(true);
                    clone->setLayer(refMovingEntity->getLayer());
                    clone->setPen(refMovingEntity->getPen(false));
                    container->addEntity(clone);

                    // delete and add this into undo
                    undoCycleReplace(refMovingEntity, clone);
                }
                goToNeutralStatus();
                updateSelectionWidget();
                graphicView->redraw();
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
                if (isShift(e)) {
                    s.selectContour(en);
                }
                else {
                    s.selectSingle(en);
                }
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
    deletePreview();
    deleteHighlights();
    deleteInfoCursor();
    drawPreview();
    drawHighlights();
    setStatus(Neutral);
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
            updateMouseWidgetTRCancel(tr("Set new position"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Create a copy")));
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
                    modifiers =  MOD_SHIFT_AND_CTRL(tr("Lengthen/Scale"),tr("Lengthen Chord"));
                    break;
                case RS2::EntityPolyline:
                    modifiers =  MOD_SHIFT_AND_CTRL(tr("Move in Previous segment direction"),tr("Move in Next segment direction"));
                    break;
                default:
                    modifiers = MOD_SHIFT_ANGLE_SNAP;
            }
            updateMouseWidget(tr("Set new ref position"), "", modifiers);
            break;
        }
        case Neutral: {
            updateMouseWidget(tr("Zoom, pan or select entity"), "", MOD_SHIFT_AND_CTRL(tr("Scroll Horizontally / Select Contour"), tr("Scroll Vertically / Select Child entities")));
            break;
        }
        case SetCorner2: {
            updateMouseWidgetTRBack(tr("Choose second edge"), MOD_SHIFT_LC(tr("De-Select entities")));
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
    clearMouseWidgetIcon();
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
