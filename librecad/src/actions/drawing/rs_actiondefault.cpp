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
#include "rs_actiondefault.h"

#include <QKeyEvent>

#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_linemath.h"
#include "lc_quickinfowidget.h"
#include "qc_applicationwindow.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_constructionline.h"
#include "rs_debug.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_line.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_selection.h"
#include "rs_settings.h"

class RS_Polyline;

struct RS_ActionDefault::ActionData {
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
//            case RS2::EntityImage:
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
RS_ActionDefault::RS_ActionDefault(LC_ActionContext *actionContext)
    :LC_OverlayBoxAction("Default",actionContext, RS2::ActionDefault), m_actionData(std::make_unique<ActionData>()), m_snapRestriction(RS2::RestrictNothing){

    RS_DEBUG->print("RS_ActionDefault::RS_ActionDefault");
    m_typeToSelect = m_graphicView->getTypeToSelect();
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
    m_actionData->v1 = m_actionData->v2 = {};
    //    snapMode.clear();
    //    snapMode.restriction = RS2::RestrictNothing;
    //    restrBak = RS2::RestrictNothing;
    //        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);

    RS_DEBUG->print("RS_ActionDefault::init: OK");
}

void RS_ActionDefault::checkSupportOfQuickEntityInfo(){
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        allowEntityQuickInfoAuto = entityInfoWidget->isAutoSelectEntitiesInDefaultAction();
        allowEntityQuickInfoForCTRL = entityInfoWidget->isSelectEntitiesInDefaultActionWithCTRL();
    }
}

void RS_ActionDefault::keyPressEvent(QKeyEvent *e){
    //        std::cout<<"RS_ActionDefault::keyPressEvent(): begin"<<std::endl;
    switch (e->key()) {
        case Qt::Key_Shift:
            m_snapRestriction = m_snapMode.restriction;
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
        setSnapRestriction(m_snapRestriction);
        e->accept();
    }
}

/*
   Highlights hovered entities that are visible and not locked.

   - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
void RS_ActionDefault::highlightHoveredEntities(LC_MouseEvent *event){
//    clearHighLighting();

    bool controlPressed = event->isControl;

    bool shouldShowQuickInfoWidget = allowEntityQuickInfoAuto || (controlPressed && allowEntityQuickInfoForCTRL);
    bool showHighlightEntity = m_highlightEntitiesOnHover || shouldShowQuickInfoWidget;
    bool showEntityDescriptions = isShowEntityDescriptionOnHighlight();

    if (!showHighlightEntity && !showEntityDescriptions)
        return;

    RS_Entity *entity = nullptr;

    RS2::ResolveLevel level = controlPressed ? RS2::ResolveAll : RS2::ResolveNone;
    entity = catchEntityByEvent(event, level);

    if (entity == nullptr) {
        m_infoCursorOverlayData->setZone2("");
        return;
    }
    if (!entity->isVisible()){
        return;
    }

    if (showEntityDescriptions){
        QString entityInfoStr = obtainEntityDescriptionForInfoCursor(entity, RS2::EntityDescriptionLevel::DescriptionLong);
        if (!entityInfoStr.isEmpty()) {
            m_infoCursorOverlayData->setZone2(entityInfoStr);
            forceUpdateInfoCursor(event);
        }
    }

    if ((entity->isLocked() && !shouldShowQuickInfoWidget)){
        return;
    }

    const double hoverToleranceFactor = (entity->rtti() == RS2::EntityEllipse)
                                        ? hoverToleranceFactor1
                                        : hoverToleranceFactor2;

    const double hoverTolerance{hoverToleranceFactor / m_viewport->getFactor().magnitude()};

    double hoverTolerance_adjusted = ((entity->rtti() != RS2::EntityEllipse) && (hoverTolerance < minimumHoverTolerance))
                                     ? minimumHoverTolerance
                                     : hoverTolerance;

    double screenTolerance = toGraphDX((int)(0.01 * std::min(m_viewport->getWidth(), m_viewport->getHeight())));
    hoverTolerance_adjusted = std::min(hoverTolerance_adjusted, screenTolerance);
    bool isPointOnEntity = false;
    RS_Vector currentMousePosition = event->graphPoint;
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

void RS_ActionDefault::forceUpdateInfoCursor(const LC_MouseEvent *event) {
    const RS_Vector pos = event->graphPoint;
    RS_Snapper::forceUpdateInfoCursor(pos);
}

bool RS_ActionDefault::isShowEntityDescriptionOnHighlight(){
    return m_graphicView->isShowEntityDescriptionOnHover() && (m_infoCursorOverlayPrefs != nullptr && m_infoCursorOverlayPrefs->enabled);
}

void RS_ActionDefault::onMouseMoveEvent([[maybe_unused]]int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->graphPoint;

    updateCoordinateWidgetByRelZero(mouse);

    // clear any existing hovering
    clearHighLighting();

    switch (getStatus()) {
        case Neutral: {
            deleteSnapper();
            highlightHoveredEntities(e);
            if (m_infoCursorOverlayPrefs->enabled){
                if (!isShowEntityDescriptionOnHighlight()) {
                    m_infoCursorOverlayData->setZone2("");
                }
                RS_Snapper::forceUpdateInfoCursor(mouse);
            }
            break;
        }
        case Dragging:{
            m_actionData->v2 = mouse;

            if (toGuiDX(m_actionData->v1.distanceTo(m_actionData->v2)) > 10){
                // look for reference points to drag:
                double dist;
                RS_EntityContainer::RefInfo refInfo = m_container->getNearestSelectedRefInfo(m_actionData->v1, &dist);
                RS_Vector ref = refInfo.ref;
                if (ref.valid == true && toGuiDX(dist) < 8){
                    m_actionData->refMovingEntity = refInfo.entity;
                    m_actionData->v1 = ref;
                    moveRelativeZero(m_actionData->v1);
                    setStatus(MovingRef);
                } else {
                    // test for an entity to drag:
                    RS_Entity *en =  catchEntity(m_actionData->v1);
                    if (en && en->isSelected()){
                        RS_Vector vp = en->getNearestRef(m_actionData->v1);
                        if (vp.valid) {
                            m_actionData->v1 = vp;
                        }
                        moveRelativeZero(m_actionData->v1);
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

            mouse = e->snapPoint;

            // additional processing for moving endpoint of lines and arcs
            bool ctrlPressed = e->isControl;
            bool shiftPressed = e->isShift;
            bool addClone = true;

            RS_Entity *refMovingEntity = m_actionData->refMovingEntity;
            RS2::EntityType type = refMovingEntity->rtti();

            // FIXME - SAND - UPDATE ENTITIES PROPERTIES ON MOVE (like chord len, radius of arc etc!) where it's applicable

            switch (type) {
                case RS2::EntityLine: {
                    auto *refMovingLine = dynamic_cast<RS_Line *>(refMovingEntity);
                    RS_Vector basePoint;
                    if (refMovingLine->getStartpoint() == m_actionData->v1){
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
                        m_actionData->v2 = newEndpoint;
                    }
                    else{
                        m_actionData->v2 = mouse;
                        previewRefLine(m_actionData->v2, m_actionData->v1);
                    }
                    if (isInfoCursorForModificationEnabled()) {
                        createEditedLineDescription(nullptr, ctrlPressed, shiftPressed);
                    }
                    break;
                }
                case RS2::EntityArc: {
                    auto *refMovingArc = dynamic_cast<RS_Arc *>(refMovingEntity);
                    auto *clone = dynamic_cast<RS_Arc *>(refMovingArc->cloneProxy());

                    const RS_Vector &arcCenter = refMovingArc->getCenter();
                    const RS_Vector &arcMiddle = refMovingArc->getMiddlePoint();
                    const RS_Vector &arcEnd = refMovingArc->getEndpoint();
                    const RS_Vector &arcStart = refMovingArc->getStartpoint();
                    if (ctrlPressed){ // for arc, we just correct angle of endpoint without changing the center and radius - if we move endpoint ref
                        mouse = getSnapAngleAwarePoint(e, arcCenter, mouse, true);
                        if (arcStart == m_actionData->v1){
                            clone->trimStartpoint(mouse);
                            m_actionData->v2 = clone->getStartpoint();
                            m_preview->addEntity(clone);
                            if (m_showRefEntitiesOnPreview) {
                                previewRefPoint(clone->getCenter());
                            }
                            addClone = false;
                        } else if (arcEnd == m_actionData->v1){
                            clone->trimEndpoint(mouse);
                            m_actionData->v2 = clone->getEndpoint();
                            m_preview->addEntity(clone);
                            if (m_showRefEntitiesOnPreview) {
                                previewRefPoint(clone->getCenter());
                            }
                            addClone = false;
                        }
                        else{ // center
                            m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                            clone->moveRef(m_actionData->v1, m_actionData->v2 - m_actionData->v1);
                            m_preview->addEntity(clone);
                            if (m_showRefEntitiesOnPreview) {
                                previewRefLine(m_actionData->v2, m_actionData->v1);
                            }
                            addClone = false;
                        }
                        if (!addClone){
                            if (m_showRefEntitiesOnPreview) {
                                previewRefLine(arcCenter, m_actionData->v2);
                                previewRefPoint(arcCenter);
                            }
                        }
                    }
                    else{
                        bool referenceMoved = false;                        
                        if (arcMiddle == m_actionData->v1){ // middle point processing
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
                                m_preview->addEntity(clone);
                                referenceMoved = true;
                            }
                            else { // change radius
                                if (m_showRefEntitiesOnPreview) {
                                    previewRefSelectablePoint(mouse);
                                    previewRefLine(arcCenter, clone->getMiddlePoint());
                                }
                            }
                        }
                        else {
                            if (shiftPressed) {
                                if (arcEnd == m_actionData->v1 ||
                                    arcStart == m_actionData->v1) { // change chord length
                                    mouse = LC_LineMath::getNearestPointOnInfiniteLine(mouse, arcStart, arcEnd);
                                }
                            } else {
                                mouse = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                            }
                        }
                        m_actionData->v2 = mouse;
                        if (!referenceMoved) {
                            clone->moveRef(m_actionData->v1, m_actionData->v2 - m_actionData->v1);
                            m_preview->addEntity(clone);
                        }

                        if (m_showRefEntitiesOnPreview) {
                            previewRefLine(m_actionData->v2, m_actionData->v1);
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
                        RS_Vector directionPoint = polyline->getRefPointAdjacentDirection(shiftPressed, m_actionData->v1);
                        if (directionPoint.valid) {
                            m_actionData->v2 = LC_LineMath::getNearestPointOnInfiniteLine(mouse, m_actionData->v1, directionPoint);
                        }
                        else{
                            m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                        }
                    }
                    else{
                        m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                    }
                    if (m_showRefEntitiesOnPreview) {
                        previewRefLine(m_actionData->v2, m_actionData->v1);
                    }
                    break;
                }
                case RS2::EntityCircle:{
                    // fixme -sand - add morphing of circle to ellipse
                    auto *refMovingCircle = dynamic_cast<RS_Circle *>(refMovingEntity);
                    auto *clone = dynamic_cast<RS_Circle *>(refMovingCircle->cloneProxy());
                    m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);

                    if (m_showRefEntitiesOnPreview) {
                        previewRefLine(m_actionData->v2, m_actionData->v1);
                    }
                    clone->moveRef(m_actionData->v1, m_actionData->v2 - m_actionData->v1);
                    m_preview->addEntity(clone);
                    if (isInfoCursorForModificationEnabled()) {
                        createEditedCircleDescription(clone, ctrlPressed, shiftPressed);
                    }
                    addClone = false;
                    break;
                }
                case RS2::EntityEllipse:{
                    // fixme - sand - add rotation of major axis
                    m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefLine(m_actionData->v2, m_actionData->v1);
                    }
                    break;
                }
                // FIXME - add additional processing for dimensions to ensure snapping of dimension lines (same as for creation of dims)
                default: {
                    m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                    if (m_showRefEntitiesOnPreview) {
                        previewRefLine(m_actionData->v2, m_actionData->v1);
                    }
                    break;
                }
            }

            updateCoordinateWidgetByRelZero(m_actionData->v2);

            if (addClone){
                RS_Entity* clone = getClone(refMovingEntity);
                const RS_Vector &offset = m_actionData->v2 - m_actionData->v1;
                clone->moveRef(m_actionData->v1, offset);
                m_preview->addEntity(clone);

                if (isInfoCursorForModificationEnabled()) {
                    msg(tr("Offset"))
                        .relative(offset)
                        .relativePolar(offset)
                        .vector(tr("New Position"), offset)
                        .toInfoCursorZone2(false);
                }
            }

            if (m_showRefEntitiesOnPreview) {
                previewRefSelectablePoint(m_actionData->v2);
                previewRefPoint(m_actionData->v1);
            }

            drawPreview();
            drawHighlights();
            break;
        }
        case Moving: {
            mouse = e->snapPoint;
            m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
            updateCoordinateWidgetByRelZero(m_actionData->v2);

//            preview->addSelectionFrom(*container,viewport);
             // fixme - sand - iterating over all entities!!! Rework selection. Add selection manager to the document, after all...
             for(auto ent: *m_container) {
                if (ent->isSelected()) {
                    RS_Entity* clone = getClone(ent);
                    m_preview->addEntity(clone);
                }
            }

            const RS_Vector &offset = m_actionData->v2 - m_actionData->v1;
            m_preview->move(offset);

            auto *line = new RS_Line(m_actionData->v1, m_actionData->v2);
            m_preview->addEntity(line);
            if (m_showRefEntitiesOnPreview) {
                previewRefLine(m_actionData->v1, m_actionData->v2);
                previewRefPoint(m_actionData->v1);
                previewRefSelectablePoint(m_actionData->v2);
            }
            line->setSelected(true);
            if (isInfoCursorForModificationEnabled()){
                QString type;
                if (e->isControl) {
                   type = tr("Copy Offset");
                } else {
                    type = tr("Move Offset");
                }
                msg(type)
                    .relative(offset)
                    .relativePolar(offset)
                    .toInfoCursorZone2(false);
            }
            break;
        }
        case SetCorner2: {
            if (m_actionData->v1.valid){
                m_actionData->v2 = mouse;
                drawOverlayBox(m_actionData->v1, m_actionData->v2);

                if (isInfoCursorForModificationEnabled()) {
                    // restore selection box to ucs
                    RS_Vector ucsP1 = toUCS(m_actionData->v1);
                    RS_Vector ucsP2 = toUCS(m_actionData->v2);
                    bool selectIntersecting = (ucsP1.x > ucsP2.x);

                    bool alterSelectIntersecting = e->isControl;
                    if (alterSelectIntersecting) {
                        selectIntersecting = !selectIntersecting;
                    }
                    bool deselect = e->isShift;

                    QString msg = deselect ? tr("De-Selecting") : tr("Selecting");
                    msg.append(tr(" entities "));
                    msg.append(selectIntersecting? tr("that intersect with box") : tr("that are within box"));
                    m_infoCursorOverlayData->setZone2(msg);
                    forceUpdateInfoCursor(e);
                }
            }
            break;
        }
        case Panning: {
            RS_Vector const vTarget{e->uiPosition};
            RS_Vector const v01 = vTarget - m_actionData->v1;
            if (v01.squared() >= 64.){
                m_viewport->zoomPan((int) v01.x, (int) v01.y);
                m_actionData->v1 = vTarget;
            }
            break;
        }
        default:
            break;
    }
}

RS_Entity* RS_ActionDefault::getClone(RS_Entity* e){
    RS_Entity* clone;
    int rtti =e->rtti();
    switch (rtti) {
        case RS2::EntityText:
        case RS2::EntityMText: {
            // fixme - sand - ucs - BAD dependency, rework.
            bool drawTextAsDraftInPreview = LC_GET_ONE_BOOL("Render", "DrawTextsAsDraftInPreview", true);
            if (drawTextAsDraftInPreview) {
                clone = e->cloneProxy();
            } else {
                clone = e->clone();
            }
            break;
        }
        default:
            clone = e->clone();
    }
    return clone;
}

void RS_ActionDefault::createEditedLineDescription([[maybe_unused]]RS_Line* clone, [[maybe_unused]]bool ctrlPressed,  [[maybe_unused]]bool shiftPressed) {
    msg(tr("Line"))
        .linear(tr("Length: "), m_actionData->v1.distanceTo(m_actionData->v2))
        .wcsAngle(tr("Angle: "), m_actionData->v1.angleTo(m_actionData->v2))
        .toInfoCursorZone2(true);
}

void RS_ActionDefault::createEditedArcDescription(RS_Arc* clone,  [[maybe_unused]]bool ctrlPressed, [[maybe_unused]] bool shiftPressed) {
    RS_Vector center = clone->getCenter();
    RS_Line tmpLine = RS_Line(clone->getStartpoint(), clone->getEndpoint());
    double height = tmpLine.getDistanceToPoint(clone->getMiddlePoint());

    msg(tr("Arc"))
        .linear(tr("Radius:"), clone->getRadius())
        .vector(tr("Center:"), center)
        .rawAngle(tr("Angle Length:"), clone->getAngleLength())
        .linear(tr("Chord Length:"), clone->getStartpoint().distanceTo(clone->getEndpoint()))
        .linear(tr("Height:"), height)
        .toInfoCursorZone2(true);
}

void RS_ActionDefault::createEditedCircleDescription(RS_Circle* clone,  [[maybe_unused]]bool ctrlPressed,  [[maybe_unused]]bool shiftPressed) {
    RS_Vector center = clone->getCenter();
    msg(tr("Circle"))
        .linear(tr("Radius:"), clone->getRadius())
        .vector(tr("Center:"), center)
        .toInfoCursorZone2(true);
}

void RS_ActionDefault::onMouseLeftButtonPress(int status, LC_MouseEvent *e) {
    switch (status) {
        case Neutral: {
            if (e->isControl){
                m_actionData->v1 = RS_Vector{e->uiPosition};
                setStatus(Panning);
            } else {
                m_actionData->v1 = e->graphPoint;
                setStatus(Dragging);
            }
            break;
        }
        case Moving: {
            m_actionData->v2 = e->snapPoint;
            m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, m_actionData->v2);
            deletePreview();
            RS_Modification m(*m_container, m_viewport);
            RS_MoveData data;
            data.number = 0;
            data.useCurrentLayer = false;
            data.useCurrentAttributes = false;
            data.keepOriginals = e->isControl;
            data.offset = m_actionData->v2 - m_actionData->v1;

            m.move(data);
            if (e->isControl) { // allow creation of several copies
                m_actionData->v1 = m_actionData->v2;
            }
            else {
                goToNeutralStatus();
            }
            moveRelativeZero(m_actionData->v2);
            updateSelectionWidget();
            deleteSnapper();
            break;
        }
        case MovingRef: {
            RS_Vector mouse = e->snapPoint;

            deletePreview();

            // additional processing for moving endpoint of lines and arcs
            bool ctrlPressed = e->isControl;
            bool shiftPressed = e->isShift;
            bool moveRefOnClone = true;

            RS_Entity *refMovingEntity = m_actionData->refMovingEntity;
            RS_Entity *clone = refMovingEntity->clone();
            RS2::EntityType type = refMovingEntity->rtti();
            switch (type) {
                case RS2::EntityLine: {
                    auto *refMovingLine = dynamic_cast<RS_Line *>(refMovingEntity);
                    RS_Vector basePoint;
                    if (refMovingLine->getStartpoint() == m_actionData->v1) {
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
                        m_actionData->v2 = newEndpoint;
                    } else {
                        m_actionData->v2 = mouse;
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
                        if (arcStart == m_actionData->v1) {
                            arcClone->trimStartpoint(mouse);
                            m_actionData->v2 = arcClone->getStartpoint();

                            moveRefOnClone = false;
                        } else if (arcEnd == m_actionData->v1) {
                            arcClone->trimEndpoint(mouse);
                            m_actionData->v2 = arcClone->getEndpoint();
                            moveRefOnClone = false;
                        } else {
                            // center
                            m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                        }
                    } else {
                        bool referenceMoved = false;
                        const RS_Vector &arcMiddle = refMovingArc->getMiddlePoint();
                        if (arcMiddle == m_actionData->v1) {
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
                                if (arcEnd == m_actionData->v1 ||
                                    arcStart == m_actionData->v1) { // change chord length
                                    mouse = LC_LineMath::getNearestPointOnInfiniteLine(mouse, arcStart, arcEnd);
                                }
                            }
                            else{ // free change of endpoint position
                                mouse = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, false);
                            }
                        }
                        if (!referenceMoved) {
                            m_actionData->v2 = mouse;
                            arcClone->moveRef(m_actionData->v1, m_actionData->v2 - m_actionData->v1);
                        }
                        moveRefOnClone = false;
                    }
                    break;
                }
                case RS2::EntityPolyline:{
                    if (shiftPressed || ctrlPressed){
                        auto* polyline = static_cast<RS_Polyline *>(refMovingEntity);
                        RS_Vector directionPoint = polyline->getRefPointAdjacentDirection(shiftPressed, m_actionData->v1);
                        if (directionPoint.valid) {
                            m_actionData->v2 = LC_LineMath::getNearestPointOnInfiniteLine(mouse, m_actionData->v1, directionPoint);
                        }
                        else{
                            m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                        }
                    }
                    else{
                        m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                    }
                    break;
                }
                default: {
                    m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, false);
                    break;
                }
            }

            if (moveRefOnClone) {
                clone->moveRef(m_actionData->v1, m_actionData->v2 - m_actionData->v1);
            }

            if (m_document) {
                clone->setSelected(true);
                clone->setLayer(refMovingEntity->getLayer());
                clone->setPen(refMovingEntity->getPen(false));
                m_container->addEntity(clone);

                // delete and add this into undo
                undoCycleReplace(refMovingEntity, clone);
            }
            goToNeutralStatus();
            updateSelectionWidget();
            redraw();
            break;
        }
        default:
            break;
    }
}

void RS_ActionDefault::onMouseRightButtonPress([[maybe_unused]]int status, LC_MouseEvent *e) {
    //cleanup
    goToNeutralStatus();
    e->originalEvent->accept();
}

void RS_ActionDefault::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionDefault::mouseReleaseEvent()");
    m_actionData->v2 = e->graphPoint;
    switch (status) {
        case Dragging: {
            // select single entity:
            RS_Entity *en = catchEntityByEvent(e);
            if (en != nullptr){
                deletePreview();
                RS_Selection s(*m_container, m_viewport);
                if (e->isShift) {
                    s.selectContour(en);
                }
                else {
                    s.selectSingle(en);
                }
                updateSelectionWidget();
                goToNeutralStatus();
            } else {
                if (m_selectWithPressedMouseOnly){
                    goToNeutralStatus();
                }
                else {
                   setStatus(SetCorner2);
                }
            }
            break;
        }
        case SetCorner2: {
            //v2 = snapPoint(e);
            m_actionData->v2 = e->graphPoint;
            // select window:
            //if (graphicView->toGuiDX(v1.distanceTo(v2))>20) {
            deletePreview();

            // restore selection box to ucs
            RS_Vector ucsP1 = toUCS(m_actionData->v1);
            RS_Vector ucsP2 = toUCS(m_actionData->v2);
            bool selectIntersecting = (ucsP1.x > ucsP2.x);

            RS_Selection s(*m_container, m_viewport);
            bool select = !e->isShift;

            bool alterSelectIntersecting = e->isControl;
            if (alterSelectIntersecting) {
                selectIntersecting = !selectIntersecting;
            }
            // expand selection wcs to ensure that selection box in ucs is fully within bounding rect in wcs
            RS_Vector wcsP1, wcsP2;
            m_viewport->worldBoundingBox(ucsP1, ucsP2, wcsP1, wcsP2);

            s.selectWindow(m_typeToSelect, wcsP1, wcsP2, select, selectIntersecting);
            updateSelectionWidget();
            goToNeutralStatus();
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

void RS_ActionDefault::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]]LC_MouseEvent *e) {
    RS_DEBUG->print("RS_ActionDefault::mouseReleaseEvent()");
    //cleanup
    goToNeutralStatus();
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
            RS2::EntityType rtti = m_actionData->refMovingEntity->rtti();
            switch (rtti){
                case RS2::EntityLine:
                    modifiers = MOD_SHIFT_AND_CTRL_ANGLE(tr("Lengthen Line"));
                    break;
                case RS2::EntityArc:
                    modifiers = MOD_SHIFT_AND_CTRL(tr("Lengthen/Scale"), tr("Lengthen Chord"));
                    break;
                case RS2::EntityPolyline:
                    modifiers = MOD_SHIFT_AND_CTRL(tr("Move in Previous segment direction"), tr("Move in Next segment direction"));
                    break;
                default:
                    modifiers = MOD_SHIFT_ANGLE_SNAP;
            }
            updateMouseWidget(tr("Set new ref position"), "", modifiers);
            break;
        }
        case Neutral: {
            updateMouseWidget(tr("Zoom, pan or select entity"), "", MOD_SHIFT_AND_CTRL(tr("Scroll Horizontally / Select Contour"), tr("Scroll Vertically / Select Child entities")));
            commandPrompt("");
            break;
        }
        case SetCorner2: {
            updateMouseWidgetTRBack(tr("Choose second edge"), MOD_SHIFT_AND_CTRL(tr("Select/Deselect entities"), tr("Select Intersecting")));
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
    return m_typeToSelect;
}

// fixme - sand - avoid direct call to appWindow??
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
