/****************************************************************************
**
* Action that creates a circle by given arc or ellipse by ellipse arc

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "lc_action_draw_circle_by_arc.h"

#include "lc_circle_by_arc_options_filler.h"
#include "lc_circle_by_arc_options_widget.h"
#include "lc_linemath.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_entity.h"

// Command line support - potentially, it's possible to use coordinates for selection of arc - yet it seems it is overkill,
// selection by mouse is more convenient so do nothing there

LC_ActionDrawCircleByArc::LC_ActionDrawCircleByArc(LC_ActionContext* actionContext) : LC_AbstractActionWithPreview(
    "ActionDrawCircleByArc", actionContext, RS2::ActionDrawCircleByArc) {
}

LC_ActionDrawCircleByArc::~LC_ActionDrawCircleByArc() = default;

// support of trigger on init functions (so on invocation, we'll check for selection and create circles for selected arcs)
bool LC_ActionDrawCircleByArc::doCheckMayTriggerOnInit(const int status) {
    return status == SetArc;
}
void LC_ActionDrawCircleByArc::doSaveOptions() {
    save("ReplaceArc", m_replaceArcByCircle);
    save("PenMode", m_penMode);
    save("LayerMode", m_layerMode);
    save("RadiusShift", m_radiusShift);
}

void LC_ActionDrawCircleByArc::doLoadOptions() {
    m_replaceArcByCircle = loadBool("ReplaceArc", true);
    m_penMode = loadInt("PenMode", 0);
    m_layerMode = loadInt("LayerMode", 0);
    m_radiusShift = loadDouble("RadiusShift", 0.0);
}

// fixme - sand - CTX_entity init!

bool LC_ActionDrawCircleByArc::isAcceptSelectedEntityToTriggerOnInit(RS_Entity* pEntity) {
    // here we'll accept only selected arcs or ellipse arcs
    const int rtti = pEntity->rtti();
    bool result = rtti == RS2::EntityArc;
    if (!result) {
        if (rtti == RS2::EntityEllipse) {
            const auto* ellipse = static_cast<RS_Ellipse*>(pEntity);
            result = ellipse->isEllipticArc();
        }
    }
    return result;
}

void LC_ActionDrawCircleByArc::doPerformOriginalEntitiesDeletionOnInitTrigger(QList<RS_Entity*>& list, LC_DocumentModificationBatch& ctx) {
    if (m_replaceArcByCircle) {
        for (const auto e : list) {
            deleteOriginalArcOrEllipse(e, ctx);
        }
    }
}

// trigger support
bool LC_ActionDrawCircleByArc::doCheckMayTrigger() {
    return m_entity != nullptr;
}

RS_Vector LC_ActionDrawCircleByArc::doGetRelativeZeroAfterTrigger() {
    // for normal trigger, we'll position relative zero to center point
    return m_entity->getCenter();
}

void LC_ActionDrawCircleByArc::doAfterTrigger() {
    m_entity = nullptr;
    setStatus(SetArc);
}

bool LC_ActionDrawCircleByArc::doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx) {
    doCreateEntitiesOnTrigger(m_entity, ctx.entitiesToAdd);
    if (m_replaceArcByCircle) {
        deleteOriginalArcOrEllipse(m_entity, ctx);
    }
    return true;
}

bool LC_ActionDrawCircleByArc::isSetActivePenAndLayerOnTrigger() {
    return false; // custom implementation in this action will take care of pen and layer
}

// creation of circle or ellipse for given entity
void LC_ActionDrawCircleByArc::doCreateEntitiesOnTrigger(RS_Entity* en, QList<RS_Entity*>& list) {
    const int rtti = en->rtti();
    switch (rtti) {
        case RS2::EntityArc: {
            // prepare data for cycle based on arc
            const auto* arc = static_cast<RS_Arc*>(en);
            const RS_CircleData circleData = createCircleData(arc);
            // setup new circle
            RS_Entity* circle = new RS_Circle(m_document, circleData);
            // apply attributes
            applyPenAndLayerBySourceEntity(arc, circle, m_penMode, m_layerMode);
            list << circle;
            break;
        }
        case RS2::EntityEllipse: {
            // prepare data for cycle based on arc
            const auto* ellipseArc = static_cast<RS_Ellipse*>(en);
            const RS_EllipseData ellipseData = createEllipseData(ellipseArc);
            // setup new circle
            const auto ellipse = new RS_Ellipse(m_document, ellipseData);
            // apply attributes
            applyPenAndLayerBySourceEntity(ellipseArc, ellipse, m_penMode, m_layerMode);
            list << ellipse;
            break;
        }
        default:
            break;
    }
}

void LC_ActionDrawCircleByArc::deleteOriginalArcOrEllipse(RS_Entity* en, LC_DocumentModificationBatch& ctx) const {
    if (checkMayExpandEntity(en, en->is(RS2::EntityArc) ? "Arc" : "Ellipse")) {
        ctx -= en;
    }
}

RS_CircleData LC_ActionDrawCircleByArc::createCircleData(const RS_Arc* arc) const {
    const RS_Vector center = arc->getCenter();
    double radius = arc->getRadius();
    if (!m_replaceArcByCircle) {
        // adjust radius to specified shift if necessary
        if (LC_LineMath::isMeaningful(m_radiusShift)) {
            radius = radius + m_radiusShift;
        }
    }
    const auto result = RS_CircleData(center, radius);
    return result;
}

RS_EllipseData LC_ActionDrawCircleByArc::createEllipseData(const RS_Ellipse* ellipseArc) const {
    const RS_EllipseData originalData = ellipseArc->getData();

    RS_EllipseData result;
    result.center = originalData.center;
    result.reversed = originalData.reversed;

    RS_Vector majorP = originalData.majorP;

    if (!m_replaceArcByCircle) {
        // adjust major axis of ellipse for necessary shift
        if (LC_LineMath::isMeaningful(m_radiusShift)) {
            majorP = majorP.relative(m_radiusShift, ellipseArc->getAngle());
        }
    }

    result.majorP = majorP;
    result.ratio = originalData.ratio;
    result.angle1 = 0.0;
    result.angle2 = 0.0;
    return result;
}

bool LC_ActionDrawCircleByArc::doCheckMayDrawPreview([[maybe_unused]] const LC_MouseEvent* event, const int status) {
    return status == SetArc;
}

void LC_ActionDrawCircleByArc::drawSnapper() {
    // disable snapper
}

void LC_ActionDrawCircleByArc::doPreparePreviewEntities([[maybe_unused]] const LC_MouseEvent* e, [[maybe_unused]] RS_Vector& snap,
                                                        QList<RS_Entity*>& list, [[maybe_unused]] int status) {
    RS_Entity* en = catchAndDescribe(e, m_circleType, RS2::ResolveAll);
    if (en != nullptr) {
        highlightHover(en);
        const int rtti = en->rtti();
        const bool isArc = en->isArc() && rtti == RS2::EntityArc;
        if (isArc) {
            auto* arc = static_cast<RS_Arc*>(en);
            const RS_CircleData circleData = createCircleData(arc);
            RS_Entity* circle = new RS_Circle(m_document, circleData);
            prepareEntityDescription(circle, RS2::EntityDescriptionLevel::DescriptionCreating);
            list << circle;

            if (m_showRefEntitiesOnPreview) {
                createRefPoint(circleData.center, list);
            }

            m_entity = arc;
        }
        else {
            if (rtti == RS2::EntityEllipse) {
                auto* ellipseArc = static_cast<RS_Ellipse*>(en);

                if (ellipseArc->isEllipticArc()) {
                    const RS_EllipseData ellipseData = createEllipseData(ellipseArc);
                    const auto ellipse = new RS_Ellipse(m_document, ellipseData);
                    prepareEntityDescription(ellipse, RS2::EntityDescriptionLevel::DescriptionCreating);
                    list << ellipse;

                    if (m_showRefEntitiesOnPreview) {
                        createRefPoint(ellipse->getCenter(), list);
                    }
                    m_entity = ellipseArc;
                }
            }
        }
    }
    else {
        m_entity = nullptr;
    }
}

void LC_ActionDrawCircleByArc::doOnLeftMouseButtonRelease([[maybe_unused]] const LC_MouseEvent* e, const int status,
                                                          [[maybe_unused]] const RS_Vector& snapPoint) {
    // just trigger on entity selection
    if (status == SetArc) {
        trigger();
    }
}

void LC_ActionDrawCircleByArc::updateActionPrompt() {
    switch (getStatus()) {
        case SetArc:
            updatePromptTRCancel(tr("Select arc or ellipse arc"));
            break;
        default:
            updatePrompt();
            break;
    }
}

bool LC_ActionDrawCircleByArc::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "radiusShift") {
        setRadiusShift(distance);
        return true;
    }
    return false;
}

RS2::CursorType LC_ActionDrawCircleByArc::doGetMouseCursor([[maybe_unused]] int status) {
    return RS2::SelectCursor;
}

LC_ActionOptionsWidget* LC_ActionDrawCircleByArc::createOptionsWidget() {
    return new LC_CircleByArcOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionDrawCircleByArc::createOptionsFiller() {
    return new LC_CircleByArcOptionsFiller();
}

void LC_ActionDrawCircleByArc::setReplaceArcByCircle(const bool value) {
    m_replaceArcByCircle = value;
}
