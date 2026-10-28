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
#include <QMouseEvent>
#include <QPointer>
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include "lc_offsetcurve.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvefinalinput.h"
#include "lc_offsetcurveui.h"
#include "../../lib/offset_worker/lc_offsetworkerclient.h"
#include "../../lib/offset_worker/lc_offsetworkeredit.h"
#include "lc_actioninfomessagebuilder.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_graphicviewport.h"
#include "lc_highlight.h"
#include "lc_linemath.h"
#include "lc_propertysheetwidget.h"
#include "lc_quickinfowidget.h"
#include "qc_applicationwindow.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_constructionline.h"
#include "rs_debug.h"
#include "rs_document.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_graphicview.h"
#include "rs_graphic.h"
#include "rs_line.h"
#include "rs_layer.h"
#include "rs_modification.h"
#include "rs_polyline.h"
#include "rs_preview.h"
#include "rs_selection.h"
#include "lc_selectedset.h"
#include "lc_selectedsetlistener.h"
#include "lc_visual_snap_manager.h"
#include "rs_settings.h"

class RS_Polyline;

namespace {
QC_ApplicationWindow* owningApplicationWindow(QWidget* widget) {
    for (; widget; widget = widget->parentWidget())
        if (auto* window = qobject_cast<QC_ApplicationWindow*>(widget)) return window;
    return nullptr;
}

bool finiteGripPoint(const RS_Vector& p) {
    return p.valid && std::isfinite(p.x) && std::isfinite(p.y) && p.z == 0;
}
bool boundedGripCache(const LC_OffsetCurve& curve) {
    const auto& d = curve.getData();
    const LC_OffsetCheckLimits hard;
    return d.degree >= 1 && d.degree <= 3 && d.controlPoints.size() <= hard.maxVertices &&
        d.fitPoints.size() <= hard.maxVertices && d.weights.size() <= hard.maxVertices &&
        d.knotslist.size() <= 4*hard.maxVertices && d.savedOpenKnots.size() <= 4*hard.maxVertices &&
        curve.count() > 0 && curve.count() <= (std::size_t{1} << 20);
}
bool registeredGripLayer(const RS_Entity& entity) {
    const RS_Entity* ancestor = &entity;
    RS_Layer* layer = nullptr;
    for (unsigned depth = 0; ancestor && depth < 64; ++depth, ancestor = ancestor->getParent()) {
        if (!layer) layer = ancestor->getLayer(false);
        if (const auto* graphic = dynamic_cast<const RS_Graphic*>(ancestor))
            return !layer || const_cast<RS_Graphic*>(graphic)->RS_Graphic::getLayerList()->contains(layer);
    }
    return !ancestor && !layer;
}
struct OffsetGripFrame {
    std::array<RS_Vector, 3> world;
    double angle = 0;
    bool counterClockwise = true;
    static OffsetGripFrame capture(LC_GraphicViewport& viewport) {
        return {{viewport.toWorld({0, 0}), viewport.toWorld({1, 0}), viewport.toWorld({0, 1})},
            viewport.toWorldAngle(viewport.toAbsUCSAngle(0)), viewport.areAnglesCounterClockwise()};
    }
    bool matches(LC_GraphicViewport& viewport) const {
        const auto current = capture(viewport);
        for (std::size_t i = 0; i < world.size(); ++i)
            if (world[i].x != current.world[i].x || world[i].y != current.world[i].y ||
                world[i].z != current.world[i].z || world[i].valid != current.world[i].valid) return false;
        return angle == current.angle && counterClockwise == current.counterClockwise;
    }
};
struct OffsetGripGesture {
    static constexpr std::size_t MaximumSegments = 63; // 126 endpoints plus two point marks.
    RS_Entity* original = nullptr; // Identity only; membership must precede every dereference.
    unsigned long long id = 0;
    std::uint64_t actionEpoch = 0, undoEpoch = 0;
    std::size_t index = 0;
    RS_Vector grip{false}, rawPoint{false};
    std::shared_ptr<LC_OffsetCurveEditSnapshot> snapshot;
    OffsetGripFrame frame;
    QPointer<RS_GraphicView> view;
    LC_GraphicViewport* viewport = nullptr;
    std::array<std::pair<RS_Vector, RS_Vector>, MaximumSegments> feedback;
    std::size_t segments = 0;
};
}

struct RS_ActionDefault::OffsetGripWork final : LC_SelectedSetListener {
    explicit OffsetGripWork(RS_Document* owner) : document(owner) { document->getSelection()->addListener(this); }
    void selectionChanged() override {}
    void selectedSetDestroyed() override { document = nullptr; }
    RS_Document* document;
    std::unique_ptr<LC_OffsetCurve> staged;
    std::unique_ptr<LC_OffsetCurveEditSnapshot> input;
    LC_OffsetRunLimits limits;
    RS_Entity* original = nullptr; // Identity only until renewed document membership.
    std::uint64_t epoch = 0;
};

struct RS_ActionDefault::OffsetGripLifetime final : LC_SelectedSetListener {
    RS_ActionDefault& action;
    RS_Document* document;
    QPointer<RS_GraphicView> view;
    OffsetGripLifetime(RS_ActionDefault& owner, RS_Document* doc, RS_GraphicView* graphicView)
        : action(owner), document(doc), view(graphicView) {
        document->getSelection()->addListener(this);
    }
    void selectionChanged() override {}
    void selectedSetDestroyed() override {
        document = nullptr; // The selection has already unregistered this listener.
        action.abandonOffsetGripContext(true);
    }
};

struct RS_ActionDefault::ActionData {
    std::unique_ptr<OffsetGripLifetime> lifetime;
    RS_Vector v1;
    RS_Vector v2;
    RS_Vector pressSnapPoint;
    RS_Entity* refMovingEntity = nullptr;
    unsigned long long hoveredEntityId = 0;
    bool hoverRefPoints = false;
    std::uint64_t gripEpoch = 0;
    std::optional<OffsetGripGesture> offsetGrip;
    std::optional<LC_OffsetCurveEditResult> lastOffsetResult;
    std::unique_ptr<LC_OffsetWorkerClient> offsetWorker;
    std::shared_ptr<OffsetGripWork> offsetWork;
    qint64 offsetRequestId = 0;
    bool offsetCommitting = false;
    bool visualSnapRedraw = false;
    bool ignoreOffsetRelease = false;
};

namespace {

// Glowing effects on Mouse hover
    constexpr double MINIMUM_HOVER_TOLERANCE = 3.0;
    constexpr double HOVER_TOLERANCE_FACTOR1 = 1.0;
    constexpr double HOVER_TOLERANCE_FACTOR2 = 10.0;

// whether the entity supports glowing effects on mouse hovering
    bool allowMouseOverGlowing(const RS_Entity *entity){
        if (entity == nullptr) {
            return false;
        }
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
    :LC_OverlayBoxAction("Default",actionContext, RS2::ActionDefault), m_actionData(std::make_unique<ActionData>()){

    RS_DEBUG->print("RS_ActionDefault::RS_ActionDefault");
    m_actionData->lifetime = std::make_unique<OffsetGripLifetime>(*this, m_document, m_graphicView);
    connect(m_graphicView, &QObject::destroyed, this, [this] {
        // RS_GraphicView's viewport and visual snap data have already been destroyed.
        abandonOffsetGripContext(false);
    });
    m_typeToSelect = m_graphicView->getTypeToSelect();
    connect(m_graphicView, &RS_GraphicView::drawingRedrawRequested, this, [this] {
        if (!offsetGripLifetimeIsAlive()) return;
        // Visual-snap notification redraws under its mutex. It is not a document edit.
        if (m_actionData->visualSnapRedraw) {
            m_actionData->hoveredEntityId = 0;
            return;
        }
        ++m_actionData->gripEpoch;
        if (m_actionData->offsetGrip && !m_actionData->offsetCommitting) {
            goToNeutralStatus();
        }
        if (m_actionData->hoveredEntityId != 0) {
            clearHighLighting();
            m_graphicView->redraw(RS2::RedrawOverlay);
        }
    });
    RS_DEBUG->print("RS_ActionDefault::RS_ActionDefault: OK");
}

RS_ActionDefault::~RS_ActionDefault() {
    cancelOffsetGripPreparation();
    m_actionData->offsetWorker.reset(); // No callback may outlive the action or its private clone.
    if (m_actionData->offsetGrip && offsetGripLifetimeIsAlive()) deleteSnapper();
    m_actionData->offsetGrip.reset();
    if (m_actionData->lifetime->view)
        disconnect(m_actionData->lifetime->view.data(), nullptr, this, nullptr);
}

void RS_ActionDefault::init(const int status){
    if (!offsetGripLifetimeIsAlive()) { abandonOffsetGripContext(false); return; }
    RS_DEBUG->print("RS_ActionDefault::init");
    ++m_actionData->gripEpoch;
    cancelOffsetGripPreparation();
    if (m_actionData->offsetGrip) {
        m_actionData->ignoreOffsetRelease = true;
        m_preview->clear();
    }
    m_actionData->offsetGrip.reset();
    m_actionData->refMovingEntity = nullptr;
    m_actionData->lastOffsetResult.reset();
    if (status >= 0){
        checkSupportOfQuickEntityInfo();
    }
    if (status == Neutral){
        deletePreview();
        deleteSnapper();
    }
    RS_PreviewActionInterface::init(status);
    m_actionData->v1 = {};
    m_actionData->v2 = {};
    //    snapMode.clear();
    //    snapMode.restriction = RS2::RestrictNothing;
    //    restrBak = RS2::RestrictNothing;
    //        RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);

    RS_DEBUG->print("RS_ActionDefault::init: OK");
}

void RS_ActionDefault::checkSupportOfQuickEntityInfo(){
    const auto* window = owningApplicationWindow(m_graphicView);
    const LC_QuickInfoWidget *entityInfoWidget = window ? window->getEntityInfoWidget() : nullptr;
    m_allowEntityQuickInfoAuto = false;
    m_allowEntityQuickInfoForCtrl = false;
    if (entityInfoWidget != nullptr){
        m_allowEntityQuickInfoAuto = entityInfoWidget->isAutoSelectEntitiesInDefaultAction();
        m_allowEntityQuickInfoForCtrl = entityInfoWidget->isSelectEntitiesInDefaultActionWithCTRL();
    }
}

void RS_ActionDefault::finish() {
    // Default remains installed; finishing an in-flight native gesture cancels it.
    if (m_actionData->offsetGrip) goToNeutralStatus();
}

bool RS_ActionDefault::isOffsetGripDragging() const { return m_actionData->offsetGrip.has_value(); }
bool RS_ActionDefault::isOffsetGripPreparing() const { return bool(m_actionData->offsetWork); }
const LC_OffsetCurveEditResult* RS_ActionDefault::lastOffsetGripResult() const {
    return m_actionData->lastOffsetResult ? &*m_actionData->lastOffsetResult : nullptr;
}
LC_OffsetRunLimits RS_ActionDefault::offsetGripRunLimits() const { return {}; }

void RS_ActionDefault::onVisualSnapEntityRegistered(RS_Entity* entity) {
    if (!offsetGripLifetimeIsAlive()) return;
    const QScopedValueRollback<bool> redraw(m_actionData->visualSnapRedraw, true);
    BASE_CLASS::onVisualSnapEntityRegistered(entity);
}

void RS_ActionDefault::mouseMoveEvent(QMouseEvent* event) { dispatchOffsetMouse(event, 0); }
void RS_ActionDefault::mousePressEvent(QMouseEvent* event) { dispatchOffsetMouse(event, 1); }
void RS_ActionDefault::mouseReleaseEvent(QMouseEvent* event) { dispatchOffsetMouse(event, 2); }

bool RS_ActionDefault::offsetGripLifetimeIsAlive() const {
    const auto* lifetime = m_actionData->lifetime.get();
    return lifetime && lifetime->document && lifetime->document == m_document &&
        lifetime->view && lifetime->view.data() == m_graphicView &&
        lifetime->view->getViewPort() == m_viewport;
}

void RS_ActionDefault::abandonOffsetGripContext(bool viewportMayBeAlive) {
    cancelOffsetGripPreparation();
    const auto* lifetime = m_actionData->lifetime.get();
    auto* viewport = viewportMayBeAlive && lifetime && lifetime->view &&
        lifetime->view.data() == m_graphicView && lifetime->view->getViewPort() == m_viewport
        ? m_viewport : nullptr;
    // These clear existing overlay containers only: no redraw, document read or lazy allocation.
    if (viewport) {
        viewport->clearOverlayEntitiesContainer(RS2::ActionPreviewEntity);
        viewport->clearOverlayEntitiesContainer(RS2::OverlayEffects);
        for (const auto overlay : {RS2::ActionPreviewEntity, RS2::OverlayEffects, RS2::Snapper, RS2::InfoCursor})
            viewport->clearOverlayDrawablesContainer(overlay);
    }
    ++m_actionData->gripEpoch;
    m_actionData->offsetGrip.reset();
    m_actionData->lastOffsetResult.reset();
    m_actionData->refMovingEntity = nullptr;
    m_actionData->hoveredEntityId = 0;
    m_actionData->ignoreOffsetRelease = true;
    if (m_preview) { m_preview->setParent(nullptr); m_preview->clear(); }
    for (auto* entity : *m_highlight) {
        entity->setParent(nullptr);
        entity->clearSelectionFlag();
    }
    m_highlight->clear();
    m_hasPreview = false;
    m_visualSnapManager->detachFromView();
    m_document = nullptr; m_graphic = nullptr; m_keyEntity = nullptr;
    m_graphicView = nullptr; m_viewport = nullptr; m_formatter = nullptr; m_infoCursorOverlayPrefs = nullptr;
    RS_ActionInterface::setStatus(Neutral); // Detached prompt/cursor hooks below do not visit the UI.
    m_finished = true;
}

bool RS_ActionDefault::offsetGripContextIsCurrent() const {
    if (!offsetGripLifetimeIsAlive()) return false;
    if (!m_actionContext || !m_document || !m_graphicView || !m_viewport ||
        m_actionContext->getDocument() != m_document ||
        m_actionContext->getGraphicView() != m_graphicView) return false;
    if (m_actionData->offsetGrip) {
        const auto& gesture = *m_actionData->offsetGrip;
        if (!gesture.view || gesture.view.data() != m_graphicView || gesture.viewport != m_viewport ||
            !gesture.frame.matches(*m_viewport)) return false;
    }
    return !m_graphicView->isClosing() && m_graphicView->getDocument() == m_document &&
        m_graphicView->getViewPort() == m_viewport;
}

bool RS_ActionDefault::hasPendingOffsetGrip() const {
    if (getStatus() != Dragging || !offsetGripContextIsCurrent() ||
        !finiteGripPoint(m_actionData->v1) || !m_document->isSingleEntitySelected()) return false;
    double distance = std::numeric_limits<double>::quiet_NaN();
    const auto ref = m_document->getNearestSelectedRefInfo(m_actionData->v1, &distance);
    if (!finiteGripPoint(ref.ref) || !std::isfinite(distance) || distance < 0 ||
        !(toGuiDX(distance) < 8) || !ref.entity || m_document->findEntityIndex(ref.entity) < 0) return false;
    const auto* curve = dynamic_cast<const LC_OffsetCurve*>(ref.entity);
    return curve && curve->getParent() == m_document && !curve->isDeleted() &&
        curve->isSelected() && curve->isVisible() && !curve->isLocked() &&
        boundedGripCache(*curve) && curve->hasEditableDefinition() &&
        curve->definitionGripIndex(ref.ref).has_value();
}

void RS_ActionDefault::dispatchOffsetMouse(QMouseEvent* event, int kind) {
    if (!offsetGripLifetimeIsAlive()) {
        abandonOffsetGripContext(false);
        event->accept();
        return;
    }
    if (m_actionData->offsetCommitting) {
        if (kind == 1 && event->button() == Qt::RightButton) goToNeutralStatus();
        event->accept(); return;
    }
    if (kind == 2 && event->button() == Qt::LeftButton && getStatus() == Neutral && m_actionData->ignoreOffsetRelease) {
        m_actionData->ignoreOffsetRelease = false;
        event->accept();
        return;
    }
    if ((m_actionData->offsetGrip || getStatus() == Dragging) && !offsetGripContextIsCurrent()) {
        m_actionData->lastOffsetResult.reset();
        m_actionData->ignoreOffsetRelease = true;
        goToNeutralStatus();
        commandMessage(tr("Offset Curve: the originating view or document has changed."));
        event->accept();
        return;
    }
    if (m_actionData->offsetGrip && !liveOffsetGrip(false)) {
        goToNeutralStatus();
        commandMessage(tr("Offset Curve: the grip or target is no longer available."));
        event->accept();
        return;
    }
    // Event conversion calls snapPoint before onMouseMoveEvent can begin the gesture.
    // Recognize a native grip from the stored press before crossing that dispatch boundary.
    const bool native = m_actionData->offsetGrip.has_value() || (kind == 0 && hasPendingOffsetGrip());
    // A viewport redraw can retire the grip before the next mouse event. Keep
    // guarding that cancelled gesture until its matching release is consumed.
    if ((native || m_actionData->ignoreOffsetRelease) &&
        (!std::isfinite(event->position().x()) || !std::isfinite(event->position().y()) ||
         event->position().x() < std::numeric_limits<int>::min() || event->position().x() > std::numeric_limits<int>::max() ||
         event->position().y() < std::numeric_limits<int>::min() || event->position().y() > std::numeric_limits<int>::max() ||
         !std::isfinite(m_viewport->getFactor().x) || m_viewport->getFactor().x == 0 ||
         !std::isfinite(m_viewport->getFactor().y) || m_viewport->getFactor().y == 0)) {
        LC_OffsetCurveEditResult result; // InvalidInput, both stages not run.
        commandMessage(LC_OffsetCurveUI::refusalText(result));
        m_actionData->lastOffsetResult.reset();
        m_actionData->ignoreOffsetRelease = true;
        goToNeutralStatus();
        event->accept();
        return;
    }
    const auto saved = m_snapMode;
    const auto restore = qScopeGuard([this, saved, native] { if (native) m_snapMode = saved; });
    if (native) {
        // Keep ordinary endpoint/grid/free snapping, but no intersection/visual solver.
        m_snapMode.snapIntersection = false;
        m_snapMode.snapVisual = false;
    }
    if (kind == 0) LC_OverlayBoxAction::mouseMoveEvent(event);
    else if (kind == 1) LC_OverlayBoxAction::mousePressEvent(event);
    else LC_OverlayBoxAction::mouseReleaseEvent(event);
}

bool RS_ActionDefault::beginOffsetGrip(RS_Entity* entity, const RS_Vector& press, const RS_Vector& grip) {
    m_actionData->lastOffsetResult.reset();
    if (!offsetGripContextIsCurrent() || !registeredGripLayer(*m_document) || !m_document->isOwner() || m_document->isLocked() ||
        m_document->findEntityIndex(entity) < 0 ||
        !finiteGripPoint(press) || !finiteGripPoint(grip)) return false;
    auto* curve = dynamic_cast<LC_OffsetCurve*>(entity);
    if (!curve || !registeredGripLayer(*curve) || !curve->isOwner() || curve->getParent() != m_document || curve->isDeleted() || !curve->isSelected() ||
        !curve->isVisible() || curve->isLocked() || !boundedGripCache(*curve) || !curve->hasEditableDefinition()) return false;
    // Admit the complete child tree and capture one bounded value witness before sampling.
    auto snapshot = LC_OffsetCurveEditSnapshot::capture(*curve);
    if (!snapshot) return false;
    const auto index = curve->definitionGripIndex(grip);
    if (!index) return false;
    const auto& grips = curve->cachedDefinitionGrips();
    const double chosen = std::hypot(press.x - grip.x, press.y - grip.y);
    if (!std::isfinite(chosen)) return false;
    // The nearest-ref API breaks ties by order. Native grips must instead be unique.
    for (std::size_t i = 0; i < grips.size(); ++i) {
        if (i == *index) continue;
        const double other = std::hypot(press.x - grips[i].x, press.y - grips[i].y);
        const double allowance = 16*std::numeric_limits<double>::epsilon()*
            std::max({chosen, other, std::numeric_limits<double>::min()});
        if (!std::isfinite(other) || other <= chosen + allowance) return false;
    }
    OffsetGripGesture gesture;
    gesture.original = curve; gesture.id = curve->getId();
    gesture.actionEpoch = ++m_actionData->gripEpoch; gesture.undoEpoch = m_document->undoStateEpoch();
    gesture.index = *index; gesture.grip = grip;
    gesture.snapshot = std::move(snapshot);
    gesture.view = m_graphicView; gesture.viewport = m_viewport;
    gesture.frame = OffsetGripFrame::capture(*m_viewport);
    const auto count = static_cast<std::size_t>(curve->count());
    gesture.segments = std::min(count, OffsetGripGesture::MaximumSegments);
    for (std::size_t i = 0; i < gesture.segments; ++i) {
        const auto at = gesture.segments == 1 ? 0 : i*(count - 1)/(gesture.segments - 1);
        const auto* line = dynamic_cast<const RS_Line*>(curve->entityAt(static_cast<int>(at)));
        if (!line || line->getParent() != curve || !finiteGripPoint(line->getStartpoint()) ||
            !finiteGripPoint(line->getEndpoint())) return false;
        gesture.feedback[i] = {line->getStartpoint(), line->getEndpoint()};
    }
    m_actionData->offsetGrip.emplace(std::move(gesture));
    clearVisualSnap();
    return true;
}

LC_OffsetCurve* RS_ActionDefault::liveOffsetGrip(bool checkGeometry) const {
    if (!m_actionData->offsetGrip || !offsetGripContextIsCurrent() ||
        !registeredGripLayer(*m_document) || !m_document->isOwner() || m_document->isLocked()) return nullptr;
    // Retain the value witness across callbackful source reads/cancellation.
    const QPointer<const RS_ActionDefault> alive(this);
    const auto& gesture = *m_actionData->offsetGrip;
    const auto snapshot = gesture.snapshot;
    const auto epoch = gesture.actionEpoch;
    const auto grip = gesture.grip;
    const auto gripIndex = gesture.index;
    if (gesture.actionEpoch != m_actionData->gripEpoch || gesture.undoEpoch != m_document->undoStateEpoch() ||
        m_document->findEntityIndex(gesture.original) < 0) return nullptr;
    // Only after the membership comparison is it safe to inspect the saved pointer.
    auto* curve = dynamic_cast<LC_OffsetCurve*>(gesture.original);
    if (!curve || !registeredGripLayer(*curve) || !curve->isOwner() || curve->getId() != gesture.id || curve->getParent() != m_document || curve->isDeleted() ||
        !curve->isSelected() || !curve->isVisible() || curve->isLocked()) return nullptr;
    if (checkGeometry) {
        if (!boundedGripCache(*curve) || !curve->hasEditableDefinition() ||
            !snapshot || !snapshot->matches(*curve)) return nullptr;
        if (!alive || !m_actionData->offsetGrip || epoch != m_actionData->gripEpoch ||
            !offsetGripContextIsCurrent() || m_document->findEntityIndex(curve) < 0) return nullptr;
        const auto index = curve->definitionGripIndex(grip);
        if (!index || *index != gripIndex) return nullptr;
    }
    return curve;
}

void RS_ActionDefault::previewOffsetGrip(const LC_MouseEvent* event) {
    if (!liveOffsetGrip(false) || !finiteGripPoint(event->snapPoint)) {
        commandMessage(tr("Offset Curve: the grip or target is no longer available."));
        goToNeutralStatus();
        return;
    }
    try {
        auto& gesture = *m_actionData->offsetGrip;
        gesture.rawPoint = event->snapPoint; // Do not project, angle-snap again, or fabricate a new cache.
        for (std::size_t i = 0; i < gesture.segments; ++i) {
            auto* line = obtainPreviewLine(gesture.feedback[i].first, gesture.feedback[i].second);
            line->setPen(RS_Pen{RS_Color{128, 128, 128}, RS2::Width00, RS2::DotLine});
        }
        previewPoint(gesture.grip);
        previewPoint(gesture.rawPoint);
        updateCoordinateWidgetByRelZero(gesture.rawPoint);
        drawPreview();
    } catch (...) {
        goToNeutralStatus();
        commandMessage(tr("Offset Curve: the bounded drag preview could not be prepared."));
    }
}

void RS_ActionDefault::completeOffsetGrip(const LC_MouseEvent* event) {
    const QPointer<RS_ActionDefault> alive(this);
    m_actionData->offsetCommitting = true;
    bool deferred = false;
    const auto cleanup = qScopeGuard([alive, &deferred] {
        if (!alive || deferred) return;
        alive->m_actionData->offsetCommitting = false;
        if (!alive->offsetGripLifetimeIsAlive()) { alive->abandonOffsetGripContext(false); return; }
        alive->goToNeutralStatus();
        if (alive && alive->offsetGripLifetimeIsAlive()) alive->redraw();
    });
    try {
        auto* original = liveOffsetGrip(true);
        if (!alive) return;
        if (!original || !finiteGripPoint(event->snapPoint)) {
            commandMessage(tr("Offset Curve: the grip or target is no longer available."));
            return;
        }
        const auto unchangedGrip = original->definitionGripIndex(event->snapPoint);
        if (unchangedGrip && *unchangedGrip == m_actionData->offsetGrip->index) return;
        auto work = std::make_shared<OffsetGripWork>(m_document);
        work->epoch = m_actionData->gripEpoch; work->original = original;
        work->staged.reset(static_cast<LC_OffsetCurve*>(original->clone()));
        work->staged->setParent(nullptr);
        work->input = LC_OffsetCurveEditSnapshot::capture(*work->staged);
        if (!alive || !offsetGripLifetimeIsAlive() || !m_actionData->offsetGrip ||
            work->epoch != m_actionData->gripEpoch) return;
        if (!work->input) { commandMessage(tr("Offset Curve: the input exceeds the snapshot limits.")); return; }
        m_actionData->offsetWork = work;
        // A settings hook may reenter or revoke this context. The clone and its
        // snapshot remain owned until the outer preparation/callback has unwound.
        const auto limits = offsetGripRunLimits();
        if (!alive || m_actionData->offsetWork != work || !offsetGripLifetimeIsAlive()) return;
        work->limits = limits;
        auto prepared = LC_OffsetWorkerEdit::prepareThrough(*work->staged, event->snapPoint, limits);
        if (!prepared.generate() && !prepared.membership()) {
            m_actionData->lastOffsetResult = LC_OffsetWorkerEdit::applyLocal(*work->staged, std::move(prepared));
            if (!*m_actionData->lastOffsetResult) commandMessage(LC_OffsetCurveUI::refusalText(*m_actionData->lastOffsetResult));
            return;
        }
        const auto& gesture = *m_actionData->offsetGrip;
        if (m_actionData->offsetRequestId == std::numeric_limits<qint64>::max() ||
            gesture.actionEpoch > std::uint64_t(std::numeric_limits<qint64>::max()) ||
            gesture.undoEpoch > std::uint64_t(std::numeric_limits<qint64>::max()) ||
            gesture.id > static_cast<unsigned long long>(std::numeric_limits<qint64>::max())) {
            commandMessage(tr("Offset Curve: the worker context is exhausted.")); return;
        }
        if (!m_actionData->offsetWorker) m_actionData->offsetWorker = std::make_unique<LC_OffsetWorkerClient>();
        const auto id = ++m_actionData->offsetRequestId;
        LC_OffsetWorkerRequest::Request request{{id, qint64(gesture.actionEpoch), qint64(gesture.id),
            qint64(gesture.undoEpoch)}, LC_OffsetWorkerRequest::Generate{}};
        if (prepared.generate()) request.operation = *prepared.generate();
        else request.operation = *prepared.membership();
        deferred = m_actionData->offsetWorker->submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [alive, work](LC_OffsetWorkerClient::Outcome outcome) {
                if (!alive || alive->m_actionData->offsetWork != work) return;
                const auto failure = outcome.status == LC_OffsetWorkerClient::Status::TimedOut ?
                    tr("Offset Curve: computation timed out; the drawing was not changed.") :
                    tr("Offset Curve: the worker could not complete the edit; the drawing was not changed.");
                alive->finishOffsetGrip(work, std::move(outcome.completed), failure);
            });
        if (!deferred) commandMessage(tr("Offset Curve: the worker could not start; the drawing was not changed."));
    } catch (...) {
        if (alive && offsetGripLifetimeIsAlive()) commandMessage(tr("Offset Curve: the drag edit could not be completed."));
    }
}

void RS_ActionDefault::cancelOffsetGripPreparation() {
    m_actionData->offsetWork.reset();
    m_actionData->offsetCommitting = false;
    if (m_actionData->offsetWorker) m_actionData->offsetWorker->cancel();
}

void RS_ActionDefault::finishOffsetGrip(const std::shared_ptr<OffsetGripWork>& work,
    std::unique_ptr<LC_OffsetWorkerCompletion> completion, const QString& failure) {
    const QPointer<RS_ActionDefault> alive(this);
    const auto cleanup = qScopeGuard([alive, work] {
        if (!alive || alive->m_actionData->offsetWork != work) return;
        alive->m_actionData->offsetCommitting = false;
        if (!alive->offsetGripLifetimeIsAlive()) { alive->abandonOffsetGripContext(false); return; }
        alive->goToNeutralStatus();
        if (alive && alive->offsetGripLifetimeIsAlive()) alive->redraw();
    });
    try {
        if (!completion) { if (offsetGripLifetimeIsAlive()) commandMessage(failure); return; }
        if (const auto* result = std::get_if<LC_OffsetWorkerResult::Native>(&completion->reply().result))
            m_actionData->lastOffsetResult = static_cast<const LC_OffsetCurveEditResult&>(*result);
        const bool unchangedInput = work->input && work->input->matches(*work->staged);
        if (!alive || m_actionData->offsetWork != work) return;
        if (!unchangedInput || liveOffsetGrip(true) != work->original) {
            if (alive && offsetGripLifetimeIsAlive()) commandMessage(tr("Offset Curve: the source changed while preparing the edit."));
            return;
        }
        if (!alive || m_actionData->offsetWork != work) return;
        m_actionData->lastOffsetResult = completion->apply(*work->staged);
        const auto result = *m_actionData->lastOffsetResult;
        if (!result) { commandMessage(LC_OffsetCurveUI::refusalText(result)); return; }
        if (!result.changed) return;
        const auto hard = makeDefaultOffsetSourceBudget();
        const auto& requested = work->limits.outputBudget;
        const auto layout = LC_EllipseOffsetGeometry::cacheLayoutCountsFromControls(
            work->staged->getData().degree, work->staged->getData().controlPoints.size());
        // Parent display settings may expand a child's cheap line cache. Its
        // actual materialized tree must still fit the original reservation.
        if (!isValidOffsetBudget(requested) || !layout ||
            layout->spans > std::min(requested.maxCubicPieces, hard.maxCubicPieces) ||
            measureOffsetOutput({work->staged.get()}, std::min(requested.maxDeepEntities,
                hard.maxDeepEntities)).status != LC_OffsetTreeStatus::Ok) {
            m_actionData->lastOffsetResult->refusal = LC_OffsetCurveRefusal::NotCacheRepresentable;
            m_actionData->lastOffsetResult->changed = false;
            commandMessage(LC_OffsetCurveUI::refusalText(*m_actionData->lastOffsetResult)); return;
        }
        auto* original = liveOffsetGrip(true);
        if (!alive || m_actionData->offsetWork != work) return;
        if (!original || original != work->original || liveOffsetGrip(false) != original) {
            commandMessage(tr("Offset Curve: the source changed while preparing the edit.")); return;
        }
        const auto snapshot = m_actionData->offsetGrip->snapshot;
        std::vector<LC_OffsetCurveFinalInput::Input> inputs{{original, snapshot.get()}};
        if (!m_document->isSingleEntitySelected() || !registeredGripLayer(*original) ||
            !LC_OffsetCurveFinalInput::validate(*m_document, inputs) || !original->getFlag(RS2::FlagSelected)) return;
        auto* document = m_document;
        auto* viewport = m_viewport;
        auto* replacement = work->staged.get();
        const auto retainDocumentOwner = qScopeGuard([work, document] {
            if (work->document == document && work->staged &&
                document->findEntityIndex(work->staged.get()) >= 0) work->staged.release();
        });
        const bool committed = document->undoableModify(viewport, [&](LC_DocumentModificationBatch& batch) {
            batch.entitiesToAdd.reserve(1); batch.entitiesToDelete.reserve(1);
            batch.dontSetActiveLayerAndPen(); batch.replace(original, replacement); return true;
        }, [alive, viewport](const LC_DocumentModificationBatch& batch, RS_Document* doc) {
            if (alive && alive->offsetGripLifetimeIsAlive() && batch.success)
                RS_Selection::selectEntitiesList(doc, viewport, batch.entitiesToAdd, true);
        });
        if (!alive) return;
        if (!committed) commandMessage(tr("Offset Curve: the replacement was not committed."));
        else m_movingJustCompleted = true;
    } catch (...) {
        if (alive && offsetGripLifetimeIsAlive()) commandMessage(tr("Offset Curve: the drag edit could not be completed."));
    }
}

void RS_ActionDefault::keyPressEvent(QKeyEvent *e){
    if (!offsetGripLifetimeIsAlive()) { e->accept(); return; }
    LC_OverlayBoxAction::keyPressEvent(e);
    if (e->isAccepted()) {
        return;
    }
    //        std::cout<<"RS_ActionDefault::keyPressEvent(): begin"<<std::endl;
    switch (e->key()) {
        case Qt::Key_Shift:
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
            break;
    }
}

void RS_ActionDefault::keyReleaseEvent(QKeyEvent *e){
    if (e->key() == Qt::Key_Shift){
        e->accept();
    }
}

/*
   Highlights hovered entities that are visible and not locked.

   - by Melwyn Francis Carlo <carlo.melwyn@outlook.com>
*/
void RS_ActionDefault::highlightHoveredEntities(const LC_MouseEvent *event){
//    clearHighLighting();

    const bool controlPressed = event->isControl;

    const bool shouldShowQuickInfoWidget = m_allowEntityQuickInfoAuto || (controlPressed && m_allowEntityQuickInfoForCtrl);
    const bool showHighlightEntity = m_highlightEntitiesOnHover || shouldShowQuickInfoWidget;
    const bool showEntityDescriptions = isShowEntityDescriptionOnHighlight();

    if (!showHighlightEntity && !showEntityDescriptions) {
        clearHighLighting();
        return;
    }

    const RS2::ResolveLevel level = controlPressed ? RS2::ResolveAll : RS2::ResolveNone;
    double hitDistance = 0.0;
    auto entity = catchEntity(event->graphPoint, level, &hitDistance);
    if (entity && entity->getParent() && entity->getParent()->rtti() == RS2::EntityOffsetCurve)
        entity = entity->getParent();

    if (entity == nullptr) {
        clearHighLighting();
        m_infoCursorOverlayData->setZone2("");
        return;
    }
    if (!entity->isVisible()){
        clearHighLighting();
        return;
    }

    if (showEntityDescriptions){
        const QString entityInfoStr = obtainEntityDescriptionForInfoCursor(entity, RS2::EntityDescriptionLevel::DescriptionLong);
        if (!entityInfoStr.isEmpty()) {
            m_infoCursorOverlayData->setZone2(entityInfoStr);
            forceUpdateInfoCursor(event);
        }
    }

    if (entity->isLocked() && !shouldShowQuickInfoWidget){
        clearHighLighting();
        return;
    }

    const auto entityType = entity->rtti();
    const double hoverToleranceFactor = (entityType == RS2::EntityEllipse) ? HOVER_TOLERANCE_FACTOR1 : HOVER_TOLERANCE_FACTOR2;

    const double hoverTolerance{hoverToleranceFactor / m_viewport->getFactor().magnitude()};

    double hoverToleranceAdjusted = ((entityType != RS2::EntityEllipse) && (hoverTolerance < MINIMUM_HOVER_TOLERANCE))
                                     ? MINIMUM_HOVER_TOLERANCE
                                     : hoverTolerance;

    const int minEdge = std::min(m_viewport->getWidth(), m_viewport->getHeight());
    const double screenTolerance = toGraphDX(static_cast<int>(0.01 * minEdge));
    hoverToleranceAdjusted = std::min(hoverToleranceAdjusted, screenTolerance);
    bool isPointOnEntity = false;
    const RS_Vector currentMousePosition = event->graphPoint;
    if (RS2::isDimensionalEntity(entityType) || RS2::isTextEntity(entityType)){
        double nearestDistanceToPointOnEntity = 0.;

        entity->getNearestPointOnEntity(currentMousePosition, true, &nearestDistanceToPointOnEntity);

        if (nearestDistanceToPointOnEntity <= hoverToleranceAdjusted) {
            isPointOnEntity = true;
        }
    } else if (entityType == RS2::EntityInsert) {
        isPointOnEntity = hitDistance <= hoverToleranceAdjusted;
    } else {
        isPointOnEntity = entity->isPointOnEntity(currentMousePosition, hoverToleranceAdjusted);
    }

    // Glowing effect on mouse hovering
    if (isPointOnEntity){
        highlightEntity(entity);
        if (shouldShowQuickInfoWidget){
            updateQuickInfoWidget(entity);
        }
    } else {
        clearHighLighting();
    }
}

void RS_ActionDefault::forceUpdateInfoCursor(const LC_MouseEvent *event) const {
    const RS_Vector pos = event->graphPoint;
    RS_Snapper::forceUpdateInfoCursor(pos);
}

bool RS_ActionDefault::isShowEntityDescriptionOnHighlight() const {
    return m_graphicView->isShowEntityDescriptionOnHover() && (m_infoCursorOverlayPrefs != nullptr && m_infoCursorOverlayPrefs->enabled);
}

void RS_ActionDefault::onMouseMoveEvent([[maybe_unused]] const int status, const LC_MouseEvent* e) {
    if (m_actionData->offsetGrip && (!finiteGripPoint(e->graphPoint) || !finiteGripPoint(e->snapPoint))) {
        goToNeutralStatus();
        commandMessage(tr("Offset Curve: the drag point is not finite."));
        return;
    }
    RS_Vector mouse = e->graphPoint;

    updateCoordinateWidgetByRelZero(mouse);

    // clear any existing hovering
    if (status != Neutral) {
        clearHighLighting();
    }

    switch (status) {
        case Neutral: {
            if (!isSnapExpected()) {
                deleteSnapper();
            }
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
                double dist = NAN;
                bool singleEntitySelection = m_document->isSingleEntitySelected();
                RS_EntityContainer::RefInfo refInfo;
                if (singleEntitySelection) {
                    // we'll try to change ref point if there is single entity selected
                    refInfo = m_document->getNearestSelectedRefInfo(m_actionData->v1, &dist);
                }
                RS_Vector ref = refInfo.ref;
                // dist will be initialized if ref.valid = true.
                if (ref.valid && toGuiDX(dist) < 8){
                    auto* owner = refInfo.entity;
                    if (owner && owner->getParent() && owner->getParent()->rtti() == RS2::EntityOffsetCurve)
                        owner = owner->getParent();
                    const auto* native = dynamic_cast<const LC_OffsetCurve*>(owner);
                    if (native && native->hasEditableDefinition()) {
                        try {
                            if (!beginOffsetGrip(owner, m_actionData->v1, ref)) {
                                commandMessage(tr("Offset Curve: no unique editable grip was selected."));
                                m_actionData->ignoreOffsetRelease = true;
                                goToNeutralStatus();
                                break;
                            }
                        } catch (...) {
                            m_actionData->ignoreOffsetRelease = true;
                            goToNeutralStatus();
                            commandMessage(tr("Offset Curve: the drag could not be prepared."));
                            break;
                        }
                        m_actionData->refMovingEntity = owner;
                        m_actionData->v1 = ref;
                        setStatus(MovingRef);
                        deletePreview();
                        previewOffsetGrip(e);
                        break;
                    }
                    m_actionData->refMovingEntity = refInfo.entity;
                    m_actionData->v1 = ref;
                    moveRelativeZero(m_actionData->v1);
                    setStatus(MovingRef);
                } else {
                    // test for an entity to drag:
                    RS_Entity *en =  catchEntity(m_actionData->v1);
                    if (en != nullptr && en->isSelected()){
                        if (m_actionData->pressSnapPoint.valid) {
                            m_actionData->v1 = m_actionData->pressSnapPoint;
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
            if (m_actionData->offsetGrip) { previewOffsetGrip(e); break; }
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
                    auto *refMovingLine = static_cast<RS_Line *>(refMovingEntity);
                    RS_Vector basePoint;
                    if (refMovingLine->getStartpoint() == m_actionData->v1){
                        basePoint = refMovingLine->getEndpoint();
                    }
                    else {
                        basePoint = refMovingLine->getStartpoint();
                    }
                    mouse = getSnapAngleAwarePoint(e, basePoint, mouse, true);
                    if (ctrlPressed){ // - if CTRL pressed, move endpoint with saving current angle of line
                        auto constructionLine = RS_ConstructionLine(nullptr,
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
                    auto *refMovingArc = static_cast<RS_Arc *>(refMovingEntity);
                    auto *clone = static_cast<RS_Arc *>(refMovingArc->cloneProxy());

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
                                    double angleFromCenterToMiddle = arcCenter.angleTo(arcMiddle);
                                    double angleFromCenterToMouse = arcCenter.angleTo(mouse);
                                    if (!LC_LineMath::isSameAngle(angleFromCenterToMiddle,angleFromCenterToMouse)) {
                                        // checking direction of mouse relating to center (own for counter-part of circle)
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
                    auto *refMovingCircle = static_cast<RS_Circle *>(refMovingEntity);
                    auto *clone = static_cast<RS_Circle *>(refMovingCircle->cloneProxy());
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

            const RS_Vector &offset = m_actionData->v2 - m_actionData->v1;

            // Move each clone individually and only then add it to m_preview,
            // rather than adding all clones first and moving the whole
            // container - m_preview may already contain entities that must
            // stay fixed in place, e.g. the ortho/horizontal/vertical
            // restriction guide lines RS_PreviewActionInterface::mouseMoveEvent()
            // adds via visualizeOrdinaryRestrictions() before this handler
            // runs, which m_preview->move(offset) would otherwise drag along
            // with the selection.
            QList<RS_Entity*> selection;
            if (m_document->collectSelected(selection)) {
                for (auto ent : std::as_const(selection)) {
                    RS_Entity* clone = getClone(ent);
                    clone->move(offset);
                    m_preview->addEntity(clone);
                }
            }

            auto *line = new RS_Line(m_actionData->v1, m_actionData->v2);
            m_preview->addEntity(line);
            if (m_showRefEntitiesOnPreview) {
                previewRefLine(m_actionData->v1, m_actionData->v2);
                previewRefPoint(m_actionData->v1);
                previewRefSelectablePoint(m_actionData->v2);
            }
            line->setSelectionFlag(true); // just use for drawing in preview, so just flag is ok
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
                    bool selectIntersecting = ucsP1.x > ucsP2.x;

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
            const RS_Vector vTarget{e->uiPosition};
            const RS_Vector v01 = vTarget - m_actionData->v1;
            if (v01.squared() >= 64.){
                m_viewport->zoomPan(static_cast<int>(v01.x), static_cast<int>(v01.y));
                m_actionData->v1 = vTarget;
            }
            break;
        }
        default:
            break;
    }
}

RS_Entity* RS_ActionDefault::getClone(const RS_Entity* e){
    RS_Entity* clone = nullptr;
    const int rtti =e->rtti();
    switch (rtti) {
        case RS2::EntityText:
        case RS2::EntityMText: {
            // fixme - sand - ucs - BAD dependency, rework.
            const bool drawTextAsDraftInPreview = LC_GET_ONE_BOOL("Render", "DrawTextsAsDraftInPreview", true);
            if (drawTextAsDraftInPreview) {
                clone = e->cloneProxy();
            } else {
                clone = e->clone();
            }
            break;
        }
        default:
            clone = e->clone();
            break;
    }
    return clone;
}

void RS_ActionDefault::createEditedLineDescription([[maybe_unused]]RS_Line* clone, [[maybe_unused]]bool ctrlPressed,  [[maybe_unused]]bool shiftPressed) const {
    msg(tr("Line"))
        .linear(tr("Length: "), m_actionData->v1.distanceTo(m_actionData->v2))
        .wcsAngle(tr("Angle: "), m_actionData->v1.angleTo(m_actionData->v2))
        .toInfoCursorZone2(true);
}

void RS_ActionDefault::createEditedArcDescription(const RS_Arc* clone,  [[maybe_unused]]bool ctrlPressed, [[maybe_unused]] bool shiftPressed) const {
    const RS_Vector center = clone->getCenter();
    const auto tmpLine = RS_Line(clone->getStartpoint(), clone->getEndpoint());
    const double height = tmpLine.getDistanceToPoint(clone->getMiddlePoint());

    msg(tr("Arc"))
        .linear(tr("Radius:"), clone->getRadius())
        .vector(tr("Center:"), center)
        .rawAngle(tr("Angle Length:"), clone->getAngleLength())
        .linear(tr("Chord Length:"), clone->getStartpoint().distanceTo(clone->getEndpoint()))
        .linear(tr("Height:"), height)
        .toInfoCursorZone2(true);
}

void RS_ActionDefault::createEditedCircleDescription(const RS_Circle* clone,  [[maybe_unused]]bool ctrlPressed,  [[maybe_unused]]bool shiftPressed) const {
    const RS_Vector center = clone->getCenter();
    msg(tr("Circle"))
        .linear(tr("Radius:"), clone->getRadius())
        .vector(tr("Center:"), center)
        .toInfoCursorZone2(true);
}

void RS_ActionDefault::onMouseMovingRefCompleted(const LC_MouseEvent* e) {
    if (m_actionData->offsetGrip) { completeOffsetGrip(e); return; }
    RS_Vector mouse = e->snapPoint;

    deletePreview();

    // additional processing for moving endpoint of lines and arcs
    const bool ctrlPressed    = e->isControl;
    const bool shiftPressed   = e->isShift;
    bool moveRefOnClone = true;

    RS_Entity *refMovingEntity = m_actionData->refMovingEntity;
    RS_Entity *clone           = refMovingEntity->clone();
    const RS2::EntityType type       = refMovingEntity->rtti();
    switch (type) {
        case RS2::EntityLine: {
            const auto *refMovingLine = static_cast<RS_Line *>(refMovingEntity);
            RS_Vector basePoint;
            if (refMovingLine->getStartpoint() == m_actionData->v1) {
                basePoint = refMovingLine->getEndpoint();
            } else {
                basePoint = refMovingLine->getStartpoint();
            }
            mouse = getSnapAngleAwarePoint(e, basePoint, mouse);
            if (ctrlPressed) {
                // - if CTRL pressed, move endpoint with saving current angle of line
                const auto constructionLine = RS_ConstructionLine(nullptr,
                                                                           RS_ConstructionLineData(refMovingLine->getStartpoint(),
                                                                                                   refMovingLine->getEndpoint()));
                const RS_Vector newEndpoint = constructionLine.getNearestPointOnEntity(mouse, false);
                m_actionData->v2      = newEndpoint;
            } else {
                m_actionData->v2 = mouse;
            }
            break;
        }
        case RS2::EntityArc: {
            const auto *refMovingArc = static_cast<RS_Arc *>(refMovingEntity);
            auto *arcClone     = static_cast<RS_Arc *>(clone);

            const RS_Vector &arcCenter = refMovingArc->getCenter();
            const RS_Vector &arcStart  = refMovingArc->getStartpoint();
            const RS_Vector &arcEnd    = refMovingArc->getEndpoint();
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
                    moveRefOnClone   = false;
                } else {
                    // center
                    m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, mouse, true);
                }
            } else {
                bool referenceMoved        = false;
                const RS_Vector &arcMiddle = refMovingArc->getMiddlePoint();
                if (arcMiddle == m_actionData->v1) {
                    mouse = LC_LineMath::getNearestPointOnInfiniteLine(mouse, arcCenter, arcMiddle);
                    if (shiftPressed) {
                        const double fromMouse   = arcCenter.distanceTo(mouse);
                        const double radius      = refMovingArc->getRadius();
                        double scaleFactor = fromMouse / radius;
                        const double fromMiddle  = arcMiddle.distanceTo(mouse);
                        if (fromMiddle > radius) {
                            const double angleFromCenterToMiddle = arcCenter.angleTo(arcMiddle);
                            const double angleFromCenterToMouse = arcCenter.angleTo(mouse);
                            // checking direction of mouse relating to center (own for counter-part of circle)
                            if (!LC_LineMath::isSameAngle(angleFromCenterToMiddle, angleFromCenterToMouse)) {
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
                const auto* polyline           = static_cast<RS_Polyline *>(refMovingEntity);
                const RS_Vector directionPoint = polyline->getRefPointAdjacentDirection(shiftPressed, m_actionData->v1);
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

    if (m_document != nullptr) {
        // delete and add this into undo
        m_document->undoableModify(m_viewport, [refMovingEntity, clone](LC_DocumentModificationBatch& ctx)-> bool {
                            ctx.dontSetActiveLayerAndPen();
                            clone->setLayer(refMovingEntity->getLayer());
                            clone->setPen(refMovingEntity->getPen(false));
                            ctx += clone;
                            ctx -= refMovingEntity;
                            return true;
                        }, [](const LC_DocumentModificationBatch& ctx, RS_Document* doc)-> void {
                            doc->select(ctx.entitiesToAdd);
                        });
    }
    goToNeutralStatus();
    deleteSnapper();
    redraw();
    m_movingJustCompleted = true;
}

void RS_ActionDefault::onMouseMovingCompleted(const LC_MouseEvent* e) {
    m_actionData->v2 = e->snapPoint;
    m_actionData->v2 = getSnapAngleAwarePoint(e, m_actionData->v1, m_actionData->v2);
    deletePreview();

    QList<RS_Entity*> selectedEntities;
    m_document->getSelection()->collectSelectedEntities(selectedEntities);
    bool keepOriginal = e->isControl;

    m_document->undoableModify(m_viewport, [this,keepOriginal, selectedEntities](LC_DocumentModificationBatch& ctx)-> bool {
                             RS_MoveData data;
                             data.number               = 0;
                             data.useCurrentLayer      = false;
                             data.useCurrentAttributes = false;
                             data.keepOriginals        = keepOriginal;
                             data.offset               = m_actionData->v2 - m_actionData->v1;
                             RS_Modification::move(data, selectedEntities, false, ctx);
                             return true;
                         }, [keepOriginal, selectedEntities](const LC_DocumentModificationBatch& ctx, RS_Document* doc)-> void {
                             if (keepOriginal) {
                                 doc->select(selectedEntities, false);
                             }
                             doc->select(ctx.entitiesToAdd);
                         });

    if (e->isControl) { // allow creation of several copies
        m_actionData->v1 = m_actionData->v2;
    }
    else {
        goToNeutralStatus();
    }
    moveRelativeZero(m_actionData->v2);
    deleteSnapper();
    m_movingJustCompleted = true;
}

void RS_ActionDefault::onMouseRightButtonPress([[maybe_unused]]int status, const LC_MouseEvent* e) {
    //cleanup
    goToNeutralStatus();
    deleteSnapper();
    e->originalEvent->accept();
}

void RS_ActionDefault::onMouseLeftButtonPress(const int status, const LC_MouseEvent* e) {
    if (status == Neutral) m_actionData->ignoreOffsetRelease = false;
    switch (status) {
        case Neutral: {
            if (e->isControl){
                m_actionData->v1 = RS_Vector{e->uiPosition};
                setStatus(Panning);
            } else {
                // dragging should be without modifiers to let custom menu invocation
                if (e->originalEvent->modifiers() == Qt::NoModifier) {
                    m_actionData->v1 = e->graphPoint;
                    m_actionData->pressSnapPoint = e->snapPoint;
                    setStatus(Dragging);
                }
            }
            break;
        }
        case Moving: {
            if (m_completeMovingByMousePressed) {
                onMouseMovingCompleted(e);
            }
            break;
        }
        case MovingRef: {
            if (!m_actionData->offsetGrip && m_completeMovingByMousePressed) {
                onMouseMovingRefCompleted(e);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDefault::onMouseLeftButtonRelease(const int status, const LC_MouseEvent* e) {
    if (status == Neutral && m_actionData->ignoreOffsetRelease) {
        m_actionData->ignoreOffsetRelease = false;
        return;
    }
    RS_DEBUG->print("RS_ActionDefault::mouseReleaseEvent()");
    m_actionData->v2 = e->graphPoint;
    switch (status) {
        case Neutral: {
            if (m_movingJustCompleted) {
                m_movingJustCompleted = false;
                break;
            }
            [[fallthrough]];
        }
        case Dragging: {
            // select single entity:
            RS_Entity *en = catchEntityByEvent(e);
            if (en != nullptr){
                deletePreview();
                const RS_Selection s(m_document, m_viewport);
                if (e->isShift) {
                    s.selectContour(en);
                }
                else {
                    s.selectSingle(en);
                }
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
            m_actionData->v2 = e->graphPoint;
            // select window:
            deletePreview();

            // restore selection box to ucs
            const RS_Vector ucsP1 = toUCS(m_actionData->v1);
            const RS_Vector ucsP2 = toUCS(m_actionData->v2);
            bool selectIntersecting = (ucsP1.x > ucsP2.x);

            const bool select = !e->isShift;

            const bool alterSelectIntersecting = e->isControl;
            if (alterSelectIntersecting) {
                selectIntersecting = !selectIntersecting;
            }
            // expand selection wcs to ensure that selection box in ucs is fully within bounding rect in wcs
            RS_Vector wcsP1;
            RS_Vector wcsP2;
            m_viewport->worldBoundingBox(ucsP1, ucsP2, wcsP1, wcsP2);

            RS_Selection s(m_document, m_viewport);
            s.selectWindow(m_typeToSelect, wcsP1, wcsP2, select, selectIntersecting);

            goToNeutralStatus();
            //}
            break;
        }
        case Panning: {
            goToNeutralStatus();
            break;
        }
        case Moving: {
            if (!m_completeMovingByMousePressed) {
                onMouseMovingCompleted(e);
            }
            break;
        }
        case MovingRef: {
            if (m_actionData->offsetGrip || !m_completeMovingByMousePressed) {
                onMouseMovingRefCompleted(e);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionDefault::onMouseRightButtonRelease([[maybe_unused]]int status, [[maybe_unused]] const LC_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionDefault::mouseReleaseEvent()");
    //cleanup
    goToNeutralStatus();
    deleteSnapper();
}

void RS_ActionDefault::goToNeutralStatus(){
    if (!offsetGripLifetimeIsAlive()) { abandonOffsetGripContext(false); return; }
    cancelOffsetGripPreparation();
    ++m_actionData->gripEpoch;
    if (m_actionData->offsetGrip) {
        m_actionData->ignoreOffsetRelease = true;
        m_preview->clear(); // Also clear partial feedback not yet drawn (m_hasPreview can be false).
        m_actionData->offsetGrip.reset();
        // Entry cleared this manager; native motion and keys cannot add visual snaps.
        // A redraw may arrive under another manager's shared snap-data mutex.
        deleteSnapper();
    }
    m_actionData->refMovingEntity = nullptr;
    deletePreview();
    clearHighLighting();
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

void RS_ActionDefault::updateActionPrompt(){
    if (!offsetGripLifetimeIsAlive()) return;
    switch (getStatus()) {
        case Moving:{
            updatePromptTRCancel(tr("Set new position"), MOD_SHIFT_AND_CTRL_ANGLE(tr("Create a copy")));
            break;
        }
        case MovingRef: {
            if (m_actionData->offsetGrip) {
                updatePromptTRCancel(tr("Set point on offset curve"));
                break;
            }
            LC_ModifiersInfo modifiers;
            const RS2::EntityType rtti = m_actionData->refMovingEntity
                ? m_actionData->refMovingEntity->rtti() : RS2::EntityUnknown;
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
                    break;
            }
            updatePrompt(tr("Set new ref position"), "", modifiers);
            break;
        }
        case Neutral: {
            updatePrompt(tr("Zoom, pan or select entity"), "", MOD_SHIFT_AND_CTRL(tr("Scroll Horizontally / Select Contour"), tr("Pan / Scroll Vertically / Select Child entities")));
            commandPrompt("");
            break;
        }
        case SetCorner2: {
            updatePromptTRBack(tr("Choose second edge"), MOD_SHIFT_AND_CTRL(tr("Select/Deselect entities"), tr("Select Intersecting")));
            break;
        }
        default: {
            updatePrompt();
            break;
        }
    }
}

RS2::CursorType RS_ActionDefault::doGetMouseCursor(const int status){
    if (!offsetGripLifetimeIsAlive()) return RS2::NoCursorChange;
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

// Direct dragging captures the source snap on press (#2689). Show that
// point before dragging without enabling visual guides during selection.
bool RS_ActionDefault::isSnapExpected() {
    return (getStatus() == Neutral && m_document->hasSelection()) || BASE_CLASS::isSnapExpected();
}

// Visual guides apply to move destinations, not selection rectangles (#2143).
bool RS_ActionDefault::isInVisualSnapStatus(int status) {
    if (m_actionData->offsetGrip) return false;
    return status == Moving || status == MovingRef;
}

bool RS_ActionDefault::clearHighlightsOnMouseMove() const {
    return getStatus() != Neutral;
}

void RS_ActionDefault::clearHighLighting(){
    m_actionData->hoveredEntityId = 0;
    deleteHighlights();
}

void RS_ActionDefault::resume(){
    if (!offsetGripLifetimeIsAlive()) return;
    clearHighLighting();
    checkSupportOfQuickEntityInfo();
    BASE_CLASS::resume();
}

void RS_ActionDefault::suspend(){
    if (!offsetGripLifetimeIsAlive()) return;
    if (m_actionData->offsetGrip) goToNeutralStatus();
    clearHighLighting();
    BASE_CLASS::suspend();
}

void RS_ActionDefault::highlightEntity(const RS_Entity *entity) {
    if (!allowMouseOverGlowing(entity)) {
        clearHighLighting();
        return;
    }
    // Drawing redraws and action transitions invalidate the retained world-coordinate clone.
    if (m_actionData->hoveredEntityId != entity->getId() || m_highlight->isEmpty()
        || m_actionData->hoverRefPoints != m_highlightEntitiesRefPointsOnHover) {
        deleteHighlights();
        highlightHover(entity);
        m_actionData->hoveredEntityId = entity->getId();
        m_actionData->hoverRefPoints = m_highlightEntitiesRefPointsOnHover;
    } else {
        // Overlay painting clears the clone's own selection flag after drawing its grips.
        auto* clone = m_highlight->entityAt(0);
        clone->RS_Entity::setSelectionFlag(m_highlightEntitiesRefPointsOnHover
                                          && (!clone->isContainer() || !clone->isLocked()));
    }
}

RS2::EntityType RS_ActionDefault::getTypeToSelect() const {
    return m_typeToSelect;
}

void RS_ActionDefault::initFromSettings() {
    LC_OverlayBoxAction::initFromSettings();
    LC_GROUP("CADPreferences"); {
        m_completeMovingByMousePressed = LC_GET_BOOL("AdHockMovingEndsByMouseClick", true);
    }
}

void RS_ActionDefault::updateQuickInfoWidget(RS_Entity *pEntity){
    auto* window = owningApplicationWindow(m_graphicView);
    LC_QuickInfoWidget *entityInfoWidget = window ? window->getEntityInfoWidget() : nullptr;
    if (entityInfoWidget != nullptr){
        entityInfoWidget->processEntity(pEntity);
    }
}
