/* Copyright (C) 2026 LibreCAD.org. SPDX-License-Identifier: GPL-2.0-or-later */
#include "lc_action_offsetcurve_updatesource.h"

#include <memory>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
#include <algorithm>
#include <array>
#include <cstring>
#include <type_traits>

#include <QMap>
#include <QSet>
#include <QStringList>
#include <QElapsedTimer>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QScopeGuard>
#include <QSignalBlocker>
#include <QTimer>

#include "lc_actioncontext.h"
#include "drw_base.h"
#include "lc_busycursorguard.h"
#include "lc_offsetcurveui.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvefinalinput.h"
#include "../../../lib/offset_worker/lc_offsetworkerclient.h"
#include "../../../lib/offset_worker/lc_offsetworkeredit.h"
#include "lc_graphicviewport.h"
#include "lc_highlight.h"
#include "lc_visual_snap_manager.h"
#include "rs_document.h"
#include "rs_ellipse.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_layer.h"
#include "rs_graphic.h"
#include "lc_selectedset.h"
#include "lc_action_options_widget.h"
#include "../../../lib/offset_worker/lc_offsetworkerrequest.h"
#include "lc_splinepoints.h"
#include "lc_parabola.h"
#include "lc_hyperbola.h"
#include "rs_line.h"
#include "rs_arc.h"
#include "rs_circle.h"
#include "rs_polyline.h"

namespace {
using SourceKind = LC_OffsetSourceGeometry::Kind;

QString sourceKindName(const SourceKind kind) {
    switch (kind) {
    case SourceKind::Line: return QObject::tr("Line");
    case SourceKind::Arc: return QObject::tr("Arc");
    case SourceKind::Circle: return QObject::tr("Circle");
    case SourceKind::Ellipse: return QObject::tr("Ellipse");
    case SourceKind::Spline: return QObject::tr("Spline");
    case SourceKind::SplinePoints: return QObject::tr("Spline points");
    case SourceKind::Parabola: return QObject::tr("Parabola");
    case SourceKind::BoundedHyperbola: return QObject::tr("Bounded hyperbola");
    case SourceKind::Polyline: return QObject::tr("Polyline");
    case SourceKind::Invalid: break;
    }
    return QObject::tr("Unsupported");
}

bool registeredUpdateLayer(const RS_Entity& entity) {
    const RS_Entity* ancestor = &entity;
    RS_Layer* layer = nullptr;
    for (unsigned depth = 0; ancestor && depth < 64; ++depth, ancestor = ancestor->getParent()) {
        if (!layer) layer = ancestor->RS_Entity::getLayer(false);
        if (const auto* graphic = dynamic_cast<const RS_Graphic*>(ancestor))
            return !layer || const_cast<RS_Graphic*>(graphic)->RS_Graphic::getLayerList()->contains(layer);
    }
    return !ancestor && !layer;
}
constexpr std::size_t UpdateCloneCharge = 4 * LC_OffsetCurveEditSnapshot::MaximumSnapshotBytes;
std::optional<std::size_t> updateCloneCharge(const LC_OffsetCurve& curve) {
    // Logical active storage for a clone and two value witnesses, not an exact
    // allocator/RSS promise. Bound all child metadata before a virtual clone.
    std::size_t bytes = 0;
    const auto add = [&](std::size_t count, std::size_t width = 1) {
        if (!width || count > (UpdateCloneCharge / 3 - bytes) / width) return false;
        bytes += count * width; return true;
    };
    const auto metadata = [&](const RS_Entity& entity) {
        if (!add(sizeof(RS_Line)) || !add(std::size_t(entity.getPen(false).getColor().colorName().size()), 2) ||
            entity.userDefVarCount() > LC_OffsetCurve::MaximumOwnedTailItems ||
            entity.getDrwExtData().size() > LC_OffsetCurveEditSnapshot::MaximumSnapshotItems ||
            !add(entity.reactorHandles().size(), sizeof(quint32)) ||
            !add(entity.getDrwExtData().size(), 32)) return false;
        const auto keys = entity.getAllKeys();
        for (const auto& key : keys) {
            if (!add(64) || !add(std::size_t(key.size()), 2) ||
                !add(std::size_t(entity.getUserDefVar(key).size()), 2)) return false;
        }
        for (const auto& tag : entity.getDrwExtData()) {
            if (!tag) return false;
            if (tag->type() == DRW_Variant::STRING) {
                if (!tag->content.s || !add(tag->content.s->size())) return false;
            } else if (tag->type() == DRW_Variant::BINARY) {
                if (!tag->binary() || tag->content.b != tag->binary() || !add(tag->binary()->size())) return false;
            }
        }
        return true;
    };
    const auto& data = curve.getData();
    if (!add(data.controlPoints.size(), sizeof(RS_Vector)) || !add(data.fitPoints.size(), sizeof(RS_Vector)) ||
        !add(data.knotslist.size(), sizeof(double)) || !add(data.weights.size(), sizeof(double)) ||
        !add(data.savedOpenKnots.size(), sizeof(double)) || !metadata(curve) ||
        !add(curve.unknownOwnedTail().size(), sizeof(LC_OffsetCurveExtension))) return {};
    for (const auto& item : curve.unknownOwnedTail()) {
        if (const auto* string = std::get_if<QString>(&item.value)) if (!add(std::size_t(string->size()), 2)) return {};
    }
    if (std::size_t(curve.count()) > LC_OffsetCurveEditSnapshot::MaximumSnapshotItems ||
        !add(std::size_t(curve.count()), sizeof(RS_LineData))) return {};
    for (const auto* child : curve) {
        if (!child || child->getParent() != &curve || !dynamic_cast<const RS_Line*>(child) || !metadata(*child)) return {};
    }
    return 3 * bytes;
}

class UpdateSourceOptions final : public LC_ActionOptionsWidget {
public:
    UpdateSourceOptions() {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(4, 0, 4, 0);
        layout->addWidget(new QLabel(tr("Source type:"), this));
        m_kind = new QComboBox(this);
        m_kind->setObjectName(QStringLiteral("cbUpdateSourceKind"));
        for (const auto kind : LC_OffsetSourceGeometry::SupportedKinds)
            m_kind->addItem(sourceKindName(kind), static_cast<int>(kind));
        layout->addWidget(m_kind);
        connect(m_kind, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](const int index) {
            if (!m_action || index < 0) return;
            m_action->setSourceKind(static_cast<SourceKind>(m_kind->itemData(index).toInt()));
            m_action->updateOptions();
        });
    }

protected:
    void doUpdateByAction(RS_ActionInterface* action) override {
        m_action = dynamic_cast<LC_ActionOffsetCurveUpdateSource*>(action);
        if (!m_action) return;
        const QSignalBlocker blocker(m_kind);
        m_kind->setCurrentIndex(m_kind->findData(static_cast<int>(m_action->sourceKind())));
    }

private:
    QComboBox* m_kind = nullptr;
    LC_ActionOffsetCurveUpdateSource* m_action = nullptr;
};
}

struct LC_ActionOffsetCurveUpdateSource::PreparedRow {
    LC_OffsetCurve* original = nullptr;
    EntityId id = 0;
    std::unique_ptr<LC_OffsetCurve> clone;
    std::unique_ptr<LC_OffsetCurveEditSnapshot> snapshot;
    qsizetype outcomeIndex = 0;
    bool publish = false;
    std::unique_ptr<LC_OffsetCurveEditSnapshot> input;
    RS_Entity* address = nullptr; // Async inventory identity, before any native downcast.
};

struct LC_ActionOffsetCurveUpdateSource::Work final : LC_SelectedSetListener {
    explicit Work(RS_Document* owner) : document(owner) { document->getSelection()->addListener(this); }
    void selectionChanged() override {}
    void selectedSetDestroyed() override {
        if (publishing) for (auto& row : rows) if (row.publish) row.clone.release();
        document = nullptr;
    }
    qint64 serial = 0;
    RS_Document* document = nullptr; // Independently tracked even if a publication notification deletes the action.
    std::uint64_t undoEpoch = 0;
    Target source;
    SourceKind sourceKind = SourceKind::Ellipse;
    LC_OffsetSourceGeometry sourceGeometry{RS_EllipseData{}};
    std::optional<QCborArray> sourceEncoding;
    RS_Layer* sourceLayer = nullptr;
    LC_OffsetRunLimits limits;
    Report report, frozen;
    std::vector<PreparedRow> rows;
    std::vector<const RS_Entity*> selection;
    std::array<RS_Vector, 3> world;
    double angle = 0;
    bool counterClockwise = true;
    QElapsedTimer elapsed;
    std::size_t next = 0, deepCharged = 0, deepLimit = 0;
    std::size_t inputCharged = 0;
    LC_EllipseOffsetGeometry::CheckWork charged{}, workLimit{};
    bool publishing = false;
    Report freeze() noexcept {
        static_assert(std::is_nothrow_copy_assignable_v<Outcome>);
        std::copy(report.outcomes.cbegin(), report.outcomes.cend(), frozen.outcomes.begin());
        frozen.updated = report.updated; frozen.unchanged = report.unchanged; frozen.refused = report.refused;
        frozen.sourceRefusal = report.sourceRefusal;
        return frozen;
    }
    ~Work() { for (auto& row : rows) if (row.clone) row.clone->setParent(nullptr); }
};

const LC_ActionOffsetCurveUpdateSource::Report& LC_ActionOffsetCurveUpdateSource::lastReport() const {
    return m_work ? m_work->report : m_report;
}

LC_ActionOffsetCurveUpdateSource::LC_ActionOffsetCurveUpdateSource(LC_ActionContext* context)
    : LC_ActionPreSelectionAwareBase("ActionOffsetCurveUpdateSource", context,
          RS2::ActionOffsetCurveUpdateSource, {RS2::EntityOffsetCurve}) {
    m_liveView = m_graphicView;
    if (m_document) m_document->getSelection()->addListener(this);
    connect(m_graphicView, &QObject::destroyed, this, [this] { detachContext(false); });
    m_worker = std::make_unique<LC_OffsetWorkerClient>();
    m_deadline = std::make_unique<QTimer>();
    m_deadline->setSingleShot(true); m_deadline->setTimerType(Qt::PreciseTimer);
    connect(m_deadline.get(), &QTimer::timeout, this, [this] {
        const auto work = m_work;
        if (work) completePreparation(work, tr("The Update from Source preparation deadline expired."));
    });
}

LC_ActionOffsetCurveUpdateSource::~LC_ActionOffsetCurveUpdateSource() {
    cancelPreparation(); m_worker.reset();
}

void LC_ActionOffsetCurveUpdateSource::selectedSetDestroyed() {
    detachContext(true);
}

void LC_ActionOffsetCurveUpdateSource::detachContext(const bool documentDestroyed) {
    cancelPreparation(tr("The Update from Source document or view was destroyed."));
    if (!documentDestroyed && m_document) m_document->getSelection()->removeListener(this);
    // The tracked selection registration is already removed; its document is partly destroyed.
    m_document = nullptr;
    m_graphic = nullptr;
    m_keyEntity = nullptr;
    if (m_preview) { m_preview->setParent(nullptr); m_preview->clear(); }
    for (auto* entity : *m_highlight) { entity->setParent(nullptr); entity->clearSelectionFlag(); }
    m_highlight->clear();
    m_visualSnapManager->detachFromView();
    m_graphicView = nullptr; m_viewport = nullptr; m_formatter = nullptr; m_liveView = nullptr;
    m_selectedEntities.clear();
    m_selectionComplete = false;
    m_finished = true;
    setFinished();
}

bool LC_ActionOffsetCurveUpdateSource::currentContext() const {
    // Selection destruction nulls m_document; a live view/context pointer alone cannot vouch for it.
    return m_actionContext && m_document && m_liveView && !m_liveView->isClosing() &&
        m_liveView.data() == m_graphicView && m_actionContext->getDocument() == m_document &&
        m_actionContext->getGraphicView() == m_liveView.data() &&
        m_liveView->getViewPort() == m_viewport && m_liveView->getDocument() == m_document &&
        (dynamic_cast<const RS_Graphic*>(m_document) || dynamic_cast<const RS_Block*>(m_document)) &&
        registeredUpdateLayer(*m_document) &&
        m_document->isOwner() && LC_OffsetCurveFinalInput::available(*m_document, true,
            !dynamic_cast<const RS_Block*>(m_document));
}

RS_Entity* LC_ActionOffsetCurveUpdateSource::liveMember(const RS_Entity* address) const {
    if (!address || !currentContext()) return nullptr;
    // Membership precedes dereferencing a pointer retained across mouse events.
    const int index = m_document->findEntityIndex(address);
    if (index < 0) return nullptr;
    auto* entity = m_document->entityAt(index);
    if (!entity || entity->getParent() != m_document || entity->isDeleted()) return nullptr;
    return entity;
}

void LC_ActionOffsetCurveUpdateSource::init(const int status) {
    cancelPreparation(tr("Update from Source was cancelled; the drawing was not changed."));
    if (!currentContext()) {
        m_report = {};
        m_report.sourceRefusal = tr("The editable document or view has changed.");
        publishReport();
        return;
    }
    if (mayInitWithContextEntity(status)) {
        const auto* saved = m_actionContext->getContextMenuActionContextEntity();
        if (saved && !liveMember(saved))
            m_actionContext->saveContextMenuActionContext(nullptr, RS_Vector(false), false);
    }
    LC_ActionPreSelectionAwareBase::init(status);
}

void LC_ActionOffsetCurveUpdateSource::setSourceKind(const LC_OffsetSourceGeometry::Kind kind) {
    if (std::find(LC_OffsetSourceGeometry::SupportedKinds.begin(),
                  LC_OffsetSourceGeometry::SupportedKinds.end(), kind) ==
        LC_OffsetSourceGeometry::SupportedKinds.end() || kind == m_sourceKind) return;
    m_sourceKind = kind;
    deletePreviewAndHighlights();
    updateActionPrompt();
    updateMouseCursor();
    redrawDrawing();
}

bool LC_ActionOffsetCurveUpdateSource::isEntityAllowedToSelect(RS_Entity* entity) const {
    entity = liveMember(entity);
    // Existing selected native targets stay in the attempt inventory even if
    // their visibility/lock/definition changes before the source is picked.
    return entity && entity->rtti() == RS2::EntityOffsetCurve &&
        dynamic_cast<LC_OffsetCurve*>(entity);
}

void LC_ActionOffsetCurveUpdateSource::doInitWithContextEntity(RS_Entity* entity, const RS_Vector&) {
    if (!isEntityAllowedToSelect(entity)) return;
    // The base helper toggles selection. Context activation must not deselect
    // an already-selected target or disturb unrelated preselection.
    if (!entity->getFlag(RS2::FlagSelected)) m_document->select(QList<RS_Entity*>{entity}, true);
    m_selectedEntities = {entity};
    onSelectionCompleted(true, true);
    redrawDrawing();
}

void LC_ActionOffsetCurveUpdateSource::onSelectionCompleted(bool, const bool fromInit) {
    if (!currentContext()) return;
    if (!fromInit) collectSelectedEntities();
    m_targets.clear();
    QSet<RS_Entity*> seen;
    for (auto* address : std::as_const(m_selectedEntities)) {
        auto* entity = liveMember(address);
        if (!isEntityAllowedToSelect(entity) || seen.contains(entity)) continue;
        seen.insert(entity);
        m_targets.append({entity, entity->getId()});
    }
    m_selectionComplete = !m_targets.isEmpty();
    if (!m_selectionComplete) commandMessage(tr("Select offset curves first."));
    updateActionPrompt();
    updateMouseCursor();
}

QString LC_ActionOffsetCurveUpdateSource::targetRefusal(RS_Entity* entity) const {
    if (!entity) return tr("Target is no longer a live top-level entity in this document.");
    if (!registeredUpdateLayer(*entity)) return tr("Target layer is no longer registered in this document.");
    if (entity->rtti() != RS2::EntityOffsetCurve || !dynamic_cast<LC_OffsetCurve*>(entity))
        return tr("Target is not an offset curve.");
    if (!static_cast<const LC_OffsetCurve*>(entity)->isOwner()) return tr("Target does not own its display cache.");
    if (!entity->isVisible()) return tr("Target is hidden.");
    if (entity->isLocked()) return tr("Target is locked.");
    const auto* curve = static_cast<const LC_OffsetCurve*>(entity);
    if (!curve->isDefined()) return tr("Target has no editable offset definition.");
    if (!curve->isConsistent()) return tr("Target cache no longer matches its stored definition.");
    return {};
}

QString LC_ActionOffsetCurveUpdateSource::sourceRefusal(RS_Entity* entity) const {
    if (!entity) return tr("Pick a live top-level %1 in this editable document.").arg(sourceKindName(m_sourceKind));
    if (!registeredUpdateLayer(*entity)) return tr("Source layer is no longer registered in this document.");
    if (entity->rtti() == RS2::EntityOffsetCurve)
        return tr("Offset curves are not valid source entities.");
    const auto geometry = LC_OffsetSourceGeometry::capture(*entity);
    if (!geometry || geometry->kind() != m_sourceKind || !geometry->isWellFormed() ||
        !LC_OffsetWorkerRequest::Values::isSourceGeometryEncodable(*geometry))
        return tr("Source is not a supported %1.").arg(sourceKindName(m_sourceKind));
    if (!entity->isVisible()) return tr("Source is hidden.");
    // A locked source is read-only input, not a replacement target.
    return {};
}

RS_Entity* LC_ActionOffsetCurveUpdateSource::sourceAt(const LC_MouseEvent* event) const {
    if (!event || !currentContext()) return nullptr;
    // The general editing picker excludes locked layers. Sources are read-only;
    // inspect directly owned candidates, retaining normal range and tie order.
    double nearest = getCatchDistance(getSnapRange(), m_catchEntityGuiRange);
    RS_EntityContainer nearby(nullptr, false);
    m_document->appendNearby(event->graphPoint, nearest, nearby);
    RS_Entity* result = nullptr;
    for (auto* entity : nearby) {
        if (!entity || entity->getParent() != m_document || entity->isDeleted() ||
            !sourceRefusal(entity).isEmpty()) continue;
        const double distance = entity->getDistanceToPoint(event->graphPoint);
        if (std::isfinite(distance) && distance >= 0 && distance <= nearest) {
            nearest = distance;
            result = entity;
        }
    }
    return result;
}

void LC_ActionOffsetCurveUpdateSource::onMouseMoveEventSelected(int, const LC_MouseEvent* event) {
    if (isPreparing()) return;
    if (auto* source = sourceAt(event)) highlightHover(source);
}

void LC_ActionOffsetCurveUpdateSource::onMouseLeftButtonReleaseSelected(int, const LC_MouseEvent* event) {
    if (isPreparing()) return;
    if (!currentContext()) {
        trigger();
        return;
    }
    auto* source = sourceAt(event);
    if (!source) {
        commandMessage(tr("Pick a visible top-level %1 in this editable document.").arg(sourceKindName(m_sourceKind)));
        return;
    }
    useSource(source);
}

void LC_ActionOffsetCurveUpdateSource::useSource(RS_Entity* source) {
    if (isPreparing()) return;
    const QPointer<LC_ActionOffsetCurveUpdateSource> alive(this);
    const auto clearSource = qScopeGuard([alive] {
        if (alive) { alive->m_source = nullptr; alive->m_sourceId = 0; }
    });
    m_source = source;
    auto* member = liveMember(source);
    m_sourceId = member ? member->getId() : 0;
    trigger();
}


void LC_ActionOffsetCurveUpdateSource::trigger() {
    if (isPreparing()) return;
    if (!currentContext()) {
        m_report = {};
        m_report.sourceRefusal = tr("The editable document or view has changed.");
        for (const auto& target : std::as_const(m_targets)) {
            m_report.outcomes.append({target.id, std::nullopt, m_report.sourceRefusal});
            ++m_report.refused;
        }
        publishReport();
        return;
    }
    if (!m_selectionComplete || m_targets.isEmpty()) {
        commandMessage(tr("Select offset curves first."));
        return;
    }
    beginPreparation();
}

void LC_ActionOffsetCurveUpdateSource::cancelPreparation(const QString& reason) {
    const auto work = m_work;
    if (m_serial < std::numeric_limits<qint64>::max()) ++m_serial;
    if (m_deadline) m_deadline->stop();
    if (m_worker) m_worker->cancel();
    if (!work) return;
    if (!reason.isEmpty() && !work->publishing) {
        work->report.updated = work->report.unchanged = 0;
        work->report.refused = unsigned(work->report.outcomes.size());
        work->report.sourceRefusal = reason;
        for (auto& outcome : work->report.outcomes) outcome.refusal = reason;
    }
    m_report = work->freeze();
    m_work.reset();
}

void LC_ActionOffsetCurveUpdateSource::finish() {
    cancelPreparation(tr("Update from Source was cancelled; the drawing was not changed."));
    if (currentContext()) LC_ActionPreSelectionAwareBase::finish();
    else { m_finished = true; setFinished(); }
}

void LC_ActionOffsetCurveUpdateSource::suspend() {
    cancelPreparation(tr("Update from Source was cancelled; the drawing was not changed."));
    if (currentContext()) LC_ActionPreSelectionAwareBase::suspend();
}

void LC_ActionOffsetCurveUpdateSource::keyPressEvent(QKeyEvent* event) {
    if (isPreparing() && event->key() == Qt::Key_Escape) { finish(); event->accept(); return; }
    if (!currentContext()) { event->accept(); return; }
    LC_ActionPreSelectionAwareBase::keyPressEvent(event);
}

bool LC_ActionOffsetCurveUpdateSource::currentWork(const std::shared_ptr<Work>& work) const {
    const QPointer<const LC_ActionOffsetCurveUpdateSource> alive(this);
    if (m_work != work || work->serial != m_serial || !currentContext()) return false;
    if (!alive || m_work != work || m_document != work->document ||
        m_document->undoStateEpoch() != work->undoEpoch) return false;
    const auto* selected = m_document->getSelection();
    auto it = selected->begin();
    for (const auto* address : work->selection) {
        if (it == selected->end() || *it != address) return false;
        ++it;
    }
    if (it != selected->end()) return false;
    const std::array<RS_Vector, 3> world{m_viewport->toWorld({0, 0}),
        m_viewport->toWorld({1, 0}), m_viewport->toWorld({0, 1})};
    for (std::size_t i = 0; i < world.size(); ++i)
        if (world[i].x != work->world[i].x || world[i].y != work->world[i].y ||
            world[i].z != work->world[i].z || world[i].valid != work->world[i].valid) return false;
    if (m_viewport->toWorldAngle(m_viewport->toAbsUCSAngle(0)) != work->angle ||
        m_viewport->areAnglesCounterClockwise() != work->counterClockwise) return false;
    if (m_sourceKind != work->sourceKind) return false;
    if (!work->report.sourceRefusal.isEmpty()) return true; // Inventory the original source refusal without math.
    const int index = m_document->findEntityIndex(work->source.address);
    if (index < 0) return false;
    const auto* source = m_document->entityAt(index);
    if (!source || source->getId() != work->source.id || source->getParent() != m_document ||
        !registeredUpdateLayer(*source) || !LC_OffsetCurveFinalInput::available(*source, false) ||
        source->RS_Entity::getLayer(false) != work->sourceLayer) return false;
    const auto geometry = LC_OffsetSourceGeometry::capture(*source);
    const auto encoding = geometry ? LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*geometry) : std::nullopt;
    return encoding && work->sourceEncoding && *encoding == *work->sourceEncoding;
}

bool LC_ActionOffsetCurveUpdateSource::currentRow(const std::shared_ptr<Work>& work, const std::size_t index) const {
    if (!currentWork(work)) return false;
    const auto& row = work->rows[index];
    auto* member = liveMember(row.original);
    if (member != row.original || !member || member->getId() != row.id ||
        !targetRefusal(member).isEmpty() || !row.snapshot || !row.input || !row.clone) return false;
    const QPointer<const LC_ActionOffsetCurveUpdateSource> alive(this);
    const bool sourceMatches = row.snapshot->matches(*row.original);
    if (!alive || m_work != work || !currentWork(work) || !sourceMatches) return false;
    const bool inputMatches = row.input->matches(*row.clone);
    return alive && m_work == work && currentWork(work) && inputMatches;
}

void LC_ActionOffsetCurveUpdateSource::beginPreparation() {
    const QPointer<LC_ActionOffsetCurveUpdateSource> alive(this);
    try {
        const auto targets = m_targets; // Freeze inventory before observed source methods can reenter.
        if (targets.size() > qsizetype(LC_OffsetCurveEditSnapshot::MaximumSnapshotItems) ||
            m_serial == std::numeric_limits<qint64>::max() ||
            m_document->undoStateEpoch() > std::uint64_t(std::numeric_limits<qint64>::max())) {
            m_report = {}; m_report.sourceRefusal = tr("The Update from Source workset exceeds its bounded inventory.");
            publishReport(); return;
        }
        auto work = std::make_shared<Work>(m_document); work->elapsed.start();
        work->serial = ++m_serial; work->document = m_document; work->undoEpoch = m_document->undoStateEpoch();
        work->limits = m_limits;
        const auto hard = makeDefaultOffsetSourceBudget();
        work->deepLimit = std::size_t{4} * std::min(hard.maxDeepEntities, m_limits.outputBudget.maxDeepEntities);
        const LC_OffsetCheckLimits check;
        work->workLimit = {4 * std::min(check.maxCacheSpans, m_limits.checkLimits.maxCacheSpans),
            4 * std::min(check.maxVertices, m_limits.checkLimits.maxVertices),
            4 * std::min(check.maxDistancePairs, m_limits.checkLimits.maxDistancePairs),
            std::min(check.maxDepth, m_limits.checkLimits.maxDepth),
            4 * std::min(check.maxOrderingComparisons, m_limits.checkLimits.maxOrderingComparisons)};
        for (const auto* selected : *m_document->getSelection()) {
            if (work->selection.size() == LC_OffsetCurveEditSnapshot::MaximumSnapshotItems) {
                m_report = {}; m_report.sourceRefusal = tr("The selected workset exceeds the bounded snapshot inventory.");
                publishReport(); return;
            }
            work->selection.push_back(selected);
        }
        work->world = {m_viewport->toWorld({0, 0}), m_viewport->toWorld({1, 0}), m_viewport->toWorld({0, 1})};
        work->angle = m_viewport->toWorldAngle(m_viewport->toAbsUCSAngle(0));
        work->counterClockwise = m_viewport->areAnglesCounterClockwise();
        auto* source = liveMember(m_source);
        if (source && source->getId() != m_sourceId) source = nullptr;
        work->report.sourceRefusal = sourceRefusal(source);
        work->sourceKind = m_sourceKind;
        if (!alive || work->serial != m_serial || !currentContext() || m_document != work->document) return;
        if (work->report.sourceRefusal.isEmpty()) {
            work->source = {source, source->getId()};
            const auto geometry = LC_OffsetSourceGeometry::capture(*source);
            work->sourceEncoding = geometry ? LC_OffsetWorkerRequest::Values::encodeSourceGeometry(*geometry) : std::nullopt;
            if (!geometry || !work->sourceEncoding) {
                work->report.sourceRefusal = tr("The selected source geometry could not be safely captured.");
            } else {
                work->sourceGeometry = *geometry;
                work->sourceLayer = source->getLayer(false);
            }
        }
        work->rows.reserve(std::size_t(targets.size()));
        work->report.outcomes.reserve(targets.size());
        for (const auto& target : targets) {
            PreparedRow row; row.address = target.address; row.id = target.id;
            row.outcomeIndex = work->report.outcomes.size(); work->rows.push_back(std::move(row));
            Outcome outcome; outcome.targetId = target.id; work->report.outcomes.append(std::move(outcome));
        }
        work->frozen.outcomes.resize(work->report.outcomes.size()); // Separate preallocated immutable receipt storage.
        m_work = work;
        m_deadline->start(int(std::max<qint64>(1, LC_OffsetWorkerClient::Deadlines::commit - work->elapsed.elapsed())));
        QTimer::singleShot(0, this, [alive, work] { if (alive) alive->advance(work); });
        updateActionPrompt();
    } catch (...) {
        if (!alive) return;
        cancelPreparation(tr("Update from Source could not prepare its bounded workset."));
        if (currentContext()) commandMessage(tr("Update from Source could not prepare its bounded workset."));
    }
}

void LC_ActionOffsetCurveUpdateSource::advance(const std::shared_ptr<Work>& work) {
    if (m_work != work) return;
    if (!currentWork(work)) { completePreparation(work, tr("The Update from Source input or context changed.")); return; }
    if (work->elapsed.elapsed() >= LC_OffsetWorkerClient::Deadlines::commit) {
        completePreparation(work, tr("The Update from Source preparation deadline expired.")); return;
    }
    if (work->next == work->rows.size()) { completePreparation(work); return; }
    const auto index = work->next++;
    auto& row = work->rows[index];
    auto& outcome = work->report.outcomes[row.outcomeIndex];
    const QPointer<LC_ActionOffsetCurveUpdateSource> alive(this);
    const auto current = [alive, work] { return alive && alive->m_work == work && alive->currentWork(work); };
    const auto next = [alive, work] {
        if (alive && alive->m_work == work) QTimer::singleShot(0, alive.data(), [alive, work] { if (alive) alive->advance(work); });
    };
    const auto refuse = [&](const QString& reason) {
        outcome.refusal = reason; ++work->report.refused;
        row.clone.reset(); row.input.reset(); row.snapshot.reset(); next();
    };
    try {
        auto* member = liveMember(row.address);
        if (member && member->getId() != row.id) member = nullptr;
        outcome.refusal = targetRefusal(member);
        if (outcome.refusal.isEmpty()) outcome.refusal = work->report.sourceRefusal;
        if (!outcome.refusal.isEmpty()) { ++work->report.refused; next(); return; }
        row.original = static_cast<LC_OffsetCurve*>(member);
        const auto inputCharge = updateCloneCharge(*row.original);
        if (!inputCharge) { refuse(tr("Target cache or metadata exceeds safe clone admission.")); return; }
        if (*inputCharge > 4 * UpdateCloneCharge - work->inputCharged) {
            refuse(tr("The Update from Source snapshot and clone admission budget was exhausted.")); return;
        }
        work->inputCharged += *inputCharge;
        row.snapshot = LC_OffsetCurveEditSnapshot::capture(*row.original);
        if (!current()) { if (alive && m_work == work) completePreparation(work, tr("The Update from Source input changed.")); return; }
        if (!row.snapshot) { refuse(tr("Target cache or metadata exceeds safe clone admission.")); return; }
        row.clone.reset(static_cast<LC_OffsetCurve*>(row.original->clone()));
        row.clone->setParent(nullptr); row.clone->clearSelectionFlag();
        if (!current()) { if (alive && m_work == work) completePreparation(work, tr("The Update from Source input changed.")); return; }
        row.input = LC_OffsetCurveEditSnapshot::capture(*row.clone);
        if (!alive || m_work != work) return;
        if (!currentRow(work, index)) {
            if (alive && m_work == work) refuse(tr("Target changed while its private replacement was prepared."));
            return;
        }
        std::optional<LC_OffsetWorkerEdit::Preparation> prepared;
        const bool legacyEllipse = work->sourceGeometry.kind() == SourceKind::Ellipse &&
                                   !row.clone->sourceDefinition();
        if (legacyEllipse) {
            auto definition = row.clone->definition();
            definition.basis = std::get<RS_EllipseData>(work->sourceGeometry.value);
            prepared.emplace(LC_OffsetWorkerEdit::prepare(*row.clone, definition, work->limits));
        } else {
            LC_OffsetSourceDefinition definition;
            if (row.clone->sourceDefinition()) definition = *row.clone->sourceDefinition();
            else {
                const auto& original = row.clone->definition();
                definition.signedDistance = original.towardCentre ? -original.distance : original.distance;
                definition.relativeTolerance = original.relTolerance;
                definition.cacheReversed = original.cacheReversed;
            }
            definition.source = work->sourceGeometry;
            prepared.emplace(LC_OffsetWorkerEdit::prepareSourceDefinition(*row.clone, definition, work->limits));
        }
        if (!current()) { if (alive && m_work == work) completePreparation(work, tr("The Update from Source input changed.")); return; }
        if (!prepared->generate() && !prepared->generateSource()) {
            outcome.edit = LC_OffsetWorkerEdit::applyLocal(*row.clone, std::move(*prepared));
            if (!*outcome.edit) { refuse(LC_OffsetCurveUI::refusalText(*outcome.edit)); return; }
            if (!outcome.edit->changed) {
                ++work->report.unchanged; row.clone.reset(); row.input.reset(); row.snapshot.reset(); next(); return;
            }
            const auto cost = measureOffsetOutput({row.clone.get()}, work->deepLimit - work->deepCharged);
            if (cost.status != LC_OffsetTreeStatus::Ok) { refuse(tr("The Update from Source output budget was exhausted.")); return; }
            work->deepCharged += cost.deepEntities; row.input.reset(); ++work->report.updated; next(); return;
        }
        if (m_requestId == std::numeric_limits<qint64>::max() || row.id > EntityId(std::numeric_limits<qint64>::max()) ||
            work->deepCharged == work->deepLimit) { refuse(tr("The Update from Source worker budget was exhausted.")); return; }
        auto operation = prepared->generate()
            ? std::variant<LC_OffsetWorkerRequest::Generate, LC_OffsetWorkerRequest::GenerateSource>{*prepared->generate()}
            : std::variant<LC_OffsetWorkerRequest::Generate, LC_OffsetWorkerRequest::GenerateSource>{*prepared->generateSource()};
        auto& generatedLimits = std::visit([](auto& op) -> LC_OffsetRunLimits& { return op.limits; }, operation);
        generatedLimits.outputBudget.maxDeepEntities = std::min(generatedLimits.outputBudget.maxDeepEntities,
            work->deepLimit - work->deepCharged);
        auto& c = generatedLimits.checkLimits;
        const LC_OffsetCheckLimits hard;
        c.maxCacheSpans = std::min({c.maxCacheSpans, hard.maxCacheSpans, work->workLimit.cacheSpans - work->charged.cacheSpans});
        c.maxVertices = std::min({c.maxVertices, hard.maxVertices, work->workLimit.vertices - work->charged.vertices});
        c.maxDistancePairs = std::min({c.maxDistancePairs, hard.maxDistancePairs, work->workLimit.distancePairs - work->charged.distancePairs});
        c.maxOrderingComparisons = std::min({c.maxOrderingComparisons, hard.maxOrderingComparisons,
            work->workLimit.orderingComparisons - work->charged.orderingComparisons});
        c.maxDepth = std::min(c.maxDepth, hard.maxDepth);
        if ((work->limits.checkLimits.maxCacheSpans && !c.maxCacheSpans) ||
            (work->limits.checkLimits.maxVertices && !c.maxVertices) ||
            (work->limits.checkLimits.maxDistancePairs && !c.maxDistancePairs)) {
            refuse(tr("The Update from Source shared checker budget was exhausted.")); return;
        }
        outcome.workerReservation = generatedLimits;
        const auto reserved = generatedLimits;
        work->deepCharged += reserved.outputBudget.maxDeepEntities;
        work->charged.cacheSpans += c.maxCacheSpans; work->charged.vertices += c.maxVertices;
        work->charged.distancePairs += c.maxDistancePairs; work->charged.orderingComparisons += c.maxOrderingComparisons;
        const auto id = ++m_requestId;
        LC_OffsetWorkerRequest::Request request{{id, work->serial, qint64(row.id), qint64(work->undoEpoch)},
            LC_OffsetWorkerRequest::Generate{}};
        std::visit([&request](auto&& op) { request.operation = std::forward<decltype(op)>(op); },
            std::move(operation));
        const std::weak_ptr<Work> weak = work;
        const bool accepted = m_worker->submit(request, LC_OffsetWorkerClient::Mode::Commit,
            [alive, weak, index, reserved](LC_OffsetWorkerClient::Outcome result) {
                const auto work = weak.lock();
                if (!alive || !work || alive->m_work != work) return;
                auto& row = work->rows[index]; auto& outcome = work->report.outcomes[row.outcomeIndex];
                try {
                    std::optional<std::size_t> knownDeep;
                    bool childSucceeded = false, counted = false;
                    if (result.completed) if (const auto* value = std::get_if<LC_OffsetWorkerResult::Native>(&result.completed->reply().result)) {
                        outcome.edit = static_cast<const LC_OffsetCurveEditResult&>(*value);
                        outcome.exactWorkerWorkKnown = result.exactGeometryWorkKnown;
                        childSucceeded = bool(*value);
                        // A real child candidate now exists. Keep its prepared
                        // count visible during observed source reads; a late
                        // refusal below removes it before any document handoff.
                        if (childSucceeded && value->changed) { ++work->report.updated; counted = true; }
                        if (result.exactGeometryWorkKnown) {
                            const auto& c = reserved.checkLimits; const auto& used = value->checkWork;
                            work->charged.cacheSpans -= c.maxCacheSpans - used.cacheSpans;
                            work->charged.vertices -= c.maxVertices - used.vertices;
                            work->charged.distancePairs -= c.maxDistancePairs - used.distancePairs;
                            work->charged.orderingComparisons -= c.maxOrderingComparisons - used.orderingComparisons;
                            if (value->attemptedDeepCountComplete)
                                knownDeep = std::max(value->attemptedOutputUsage.deepEntities, value->outputUsage.deepEntities);
                        }
                    }
                    if (!alive->currentWork(work)) { alive->completePreparation(work, tr("The Update from Source input or context changed.")); return; }
                    if (!result.completed || result.status != LC_OffsetWorkerClient::Status::Completed) {
                        outcome.refusal = result.status == LC_OffsetWorkerClient::Status::TimedOut ?
                            tr("The native worker timed out.") : tr("The native worker could not complete this target.");
                    } else {
                        const bool matches = alive->currentRow(work, index);
                        if (!alive || alive->m_work != work) return;
                        if (!matches) outcome.refusal = tr("Target changed while its private replacement was prepared.");
                        else {
                            outcome.edit = result.completed->apply(*row.clone);
                            if (!*outcome.edit) outcome.refusal = LC_OffsetCurveUI::refusalText(*outcome.edit);
                            else if (outcome.edit->changed) {
                                if (!counted) { ++work->report.updated; counted = true; }
                                const auto cost = measureOffsetOutput({row.clone.get()}, reserved.outputBudget.maxDeepEntities);
                                if (cost.status != LC_OffsetTreeStatus::Ok) {
                                    outcome.refusal = tr("The received output exceeds its original reservation."); knownDeep.reset();
                                } else { if (knownDeep) *knownDeep = std::max(*knownDeep, cost.deepEntities); }
                            } else {
                                if (counted) { --work->report.updated; counted = false; }
                                ++work->report.unchanged;
                            }
                        }
                    }
                    if (knownDeep && *knownDeep <= reserved.outputBudget.maxDeepEntities &&
                        (!childSucceeded || (outcome.refusal.isEmpty() && row.clone && outcome.edit && outcome.edit->changed)))
                        work->deepCharged -= reserved.outputBudget.maxDeepEntities - *knownDeep;
                    if (!outcome.refusal.isEmpty()) {
                        if (counted) --work->report.updated;
                        ++work->report.refused; row.clone.reset(); row.snapshot.reset();
                    }
                    else if (!outcome.edit || !outcome.edit->changed) { row.clone.reset(); row.snapshot.reset(); }
                    row.input.reset();
                    QTimer::singleShot(0, alive.data(), [alive, work] { if (alive) alive->advance(work); });
                } catch (...) {
                    if (alive && alive->m_work == work)
                        alive->completePreparation(work, tr("Update from Source failed before publication."));
                }
            });
        if (!accepted) refuse(tr("The native worker could not admit this target."));
    } catch (...) {
        if (alive && m_work == work) completePreparation(work, tr("Update from Source failed before publication."));
    }
}

void LC_ActionOffsetCurveUpdateSource::completePreparation(const std::shared_ptr<Work>& work, const QString& reason) {
    if (m_work != work) return;
    const QPointer<LC_ActionOffsetCurveUpdateSource> alive(this);
    try {
        const auto refusal = !reason.isEmpty() ? reason : !currentWork(work) ?
            tr("The Update from Source input or context changed.") :
            work->elapsed.elapsed() >= LC_OffsetWorkerClient::Deadlines::commit ?
            tr("The Update from Source preparation deadline expired.") : QString{};
        if (!refusal.isEmpty()) {
            cancelPreparation(refusal);
            if (currentContext()) publishReport();
            return;
        }
        m_deadline->stop();
        LC_DocumentModificationBatch prepared;
        prepared.dontSetActiveLayerAndPen();
        const bool publish = publishPrepared(prepared, work->rows, work->document, work->undoEpoch, work->report, work);
        if (!alive || m_work != work) return;
        const bool current = currentWork(work);
        if (!alive || m_work != work) return;
        if (publish && !current) {
            completePreparation(work, tr("The Update from Source input or context changed.")); return;
        }
        if (publish) {
            auto* document = m_document; auto* viewport = m_viewport;
            work->publishing = true;
            const auto retainOwners = qScopeGuard([work, document] {
                if (work->document != document) return;
                for (auto& row : work->rows) if (row.clone && document->findEntityIndex(row.clone.get()) >= 0) row.clone.release();
            });
            const bool committed = document->undoableModify(viewport, [&](LC_DocumentModificationBatch& batch) {
                batch.dontSetActiveLayerAndPen();
                batch.entitiesToAdd.swap(prepared.entitiesToAdd); batch.entitiesToDelete.swap(prepared.entitiesToDelete);
                return true;
            }, [alive](const LC_DocumentModificationBatch& batch, RS_Document*) {
                if (alive && alive->currentContext()) alive->doTriggerSelections(batch);
            });
            if (!alive || m_work != work) return;
            work->publishing = false;
            if (!committed) {
                for (auto& row : work->rows) if (row.publish) {
                    --work->report.updated; ++work->report.refused;
                    work->report.outcomes[row.outcomeIndex].refusal = tr("The replacement batch was not committed.");
                }
            }
        }
        if (!alive || m_work != work) return;
        m_report = work->freeze(); m_work.reset(); m_worker->cancel();
        doTriggerCompletion(publish);
    } catch (...) {
        if (!alive || m_work != work) return;
        cancelPreparation(tr("Update from Source failed before publication."));
        if (currentContext()) publishReport();
    }
}

bool LC_ActionOffsetCurveUpdateSource::doTriggerModifications(LC_DocumentModificationBatch& batch) {
    batch.dontSetActiveLayerAndPen();
    m_report = {};
    if (!currentContext()) return false;
    auto* const admittedDocument = m_document;
    const auto admittedEpoch = m_document->undoStateEpoch();
    auto* source = liveMember(m_source);
    if (source && source->getId() != m_sourceId) source = nullptr;
    m_report.sourceRefusal = sourceRefusal(source);
    std::optional<RS_EllipseData> basis;
    if (m_report.sourceRefusal.isEmpty() && m_sourceKind == SourceKind::Ellipse)
        basis = static_cast<RS_Ellipse*>(source)->getData();
    else if (m_report.sourceRefusal.isEmpty())
        m_report.sourceRefusal = tr("Typed source replacement requires the isolated worker path.");

    std::vector<PreparedRow> staged;
    {
        LC_BusyCursorGuard busy;
        for (const auto& target : std::as_const(m_targets)) {
            Outcome outcome;
            outcome.targetId = target.id;
            auto* entity = liveMember(target.address);
            if (entity && entity->getId() != target.id) entity = nullptr;
            outcome.refusal = targetRefusal(entity);
            if (outcome.refusal.isEmpty()) outcome.refusal = m_report.sourceRefusal;
            if (!outcome.refusal.isEmpty()) {
                ++m_report.refused;
                m_report.outcomes.append(std::move(outcome));
                continue;
            }

            auto* original = static_cast<LC_OffsetCurve*>(entity);
            auto snapshot = LC_OffsetCurveEditSnapshot::capture(*original);
            if (!snapshot) {
                outcome.refusal = tr("Target cache or metadata exceeds safe clone admission.");
                ++m_report.refused;
                m_report.outcomes.append(std::move(outcome));
                continue;
            }
            std::unique_ptr<LC_OffsetCurve> clone(static_cast<LC_OffsetCurve*>(original->clone()));
            clone->setParent(nullptr); // Private setters must not scan the live selection domain.
            if (liveMember(original) != original || original->getId() != target.id ||
                !targetRefusal(original).isEmpty() || !snapshot->matches(*original)) {
                outcome.refusal = tr("Target changed while its private replacement was prepared.");
                ++m_report.refused;
                m_report.outcomes.append(std::move(outcome));
                continue;
            }
            clone->clearSelectionFlag();
            // H supplies the shared formatter/guard. No create/adopt/checkCache,
            // update(), fallback, or second setter call belongs in this wrapper.
            outcome.edit = clone->setBasis(*basis, m_limits);
            if (!*outcome.edit) {
                outcome.refusal = LC_OffsetCurveUI::refusalText(*outcome.edit);
                ++m_report.refused;
            } else if (!outcome.edit->changed) {
                ++m_report.unchanged;
            } else {
                staged.push_back({original, target.id, std::move(clone), std::move(snapshot),
                    m_report.outcomes.size()});
                ++m_report.updated;
            }
            m_report.outcomes.append(std::move(outcome));
        }
    }
    return publishPrepared(batch, staged, admittedDocument, admittedEpoch, m_report);
}

bool LC_ActionOffsetCurveUpdateSource::publishPrepared(LC_DocumentModificationBatch& batch,
    std::vector<PreparedRow>& staged, RS_Document* admittedDocument, const std::uint64_t admittedEpoch,
    Report& report, const std::shared_ptr<Work>& work) {
    const QPointer<LC_ActionOffsetCurveUpdateSource> alive(this);
    // Shared handoff for synchronous non-GUI preparation and async GUI receipts.
    // Every callbackful read precedes list handoff and the final stored seal.
    qsizetype published = 0;
    for (auto& entry : staged) {
        if (!entry.clone) continue;
        const bool matched = liveMember(entry.original) == entry.original && entry.original->getId() == entry.id &&
            targetRefusal(entry.original).isEmpty() && entry.snapshot->matches(*entry.original);
        if (!alive || (work && m_work != work)) return false;
        if (!matched) {
            --report.updated;
            ++report.refused;
            report.outcomes[entry.outcomeIndex].refusal =
                tr("Target changed while its private replacement was prepared.");
            entry.clone.reset();
            continue;
        }
        ++published;
    }
    if (published == 0) return false;

    // Preserve caller prefixes and all candidate owners if either list preparation throws.
    LC_DocumentModificationBatch prepared;
    prepared.entitiesToAdd = batch.entitiesToAdd;
    prepared.entitiesToDelete = batch.entitiesToDelete;
    const auto maximum = std::numeric_limits<qsizetype>::max();
    if (published > maximum - prepared.entitiesToAdd.size() ||
        published > maximum - prepared.entitiesToDelete.size())
        throw std::length_error("Update from Source batch exceeds native list capacity");
    // reserve() can retain already-reserved shared storage on supported Qt.
    prepared.entitiesToAdd.detach();
    prepared.entitiesToDelete.detach();
    prepared.entitiesToAdd.reserve(prepared.entitiesToAdd.size() + published);
    prepared.entitiesToDelete.reserve(prepared.entitiesToDelete.size() + published);
    std::vector<LC_OffsetCurveFinalInput::Input> inputs;
    inputs.reserve(staged.size());
    for (const auto& entry : staged)
        if (entry.clone) inputs.push_back({entry.original, entry.snapshot.get()});
    const auto lateRefusal = tr("Target changed while its private replacement was prepared.");
    // All observed callbacks and private-list preparation precede the all-root
    // stored seal. A later target can have changed an earlier observed peer.
    if (currentContext() && m_document == admittedDocument && m_document->undoStateEpoch() == admittedEpoch &&
        (!work || (currentWork(work) && work->elapsed.elapsed() < LC_OffsetWorkerClient::Deadlines::commit)))
        LC_OffsetCurveFinalInput::validate(*m_document, inputs);
    if (!alive || (work && m_work != work)) return false;
    std::size_t input = 0;
    published = 0;
    for (auto& entry : staged) if (entry.clone) {
        entry.publish = inputs[input++].matches;
        if (!entry.publish) {
            --report.updated;
            ++report.refused;
            report.outcomes[entry.outcomeIndex].refusal = lateRefusal;
        } else {
            prepared.replace(entry.original, entry.clone.get());
            ++published;
        }
    }
    if (published == 0) return false;

    // No validation, allocation or virtual callback below the ownership boundary.
    static_assert(noexcept(batch.entitiesToAdd.swap(prepared.entitiesToAdd)));
    static_assert(noexcept(batch.entitiesToDelete.swap(prepared.entitiesToDelete)));
    static_assert(noexcept(std::declval<std::unique_ptr<LC_OffsetCurve>&>().release()));
    batch.entitiesToAdd.swap(prepared.entitiesToAdd);
    batch.entitiesToDelete.swap(prepared.entitiesToDelete);
    // Async GUI work keeps its owners until the document actually accepts the
    // presealed lists. The protected synchronous callback retains its contract.
    if (!work) for (auto& entry : staged) if (entry.publish) entry.clone.release();
    return true;
}

void LC_ActionOffsetCurveUpdateSource::doTriggerSelectionUpdate(
    const bool keepSelected, const LC_DocumentModificationBatch& batch) {
    if (keepSelected) select(batch.entitiesToAdd);
    // Refused/no-op originals and unrelated selection were never deselected.
}

void LC_ActionOffsetCurveUpdateSource::publishReport() {
    QString message = tr("Update from Source: %1 updated, %2 unchanged, %3 refused.")
        .arg(m_report.updated).arg(m_report.unchanged).arg(m_report.refused);
    QMap<QString, unsigned> reasons;
    for (const auto& outcome : std::as_const(m_report.outcomes))
        if (!outcome.refusal.isEmpty()) ++reasons[outcome.refusal];
    if (reasons.isEmpty() && !m_report.sourceRefusal.isEmpty())
        reasons[m_report.sourceRefusal] = 1;
    QStringList details;
    for (auto it = reasons.cbegin(); it != reasons.cend(); ++it)
        details.append(tr("%1 (%2)").arg(it.key()).arg(it.value()));
    if (!details.isEmpty()) message += " " + details.join("; ");
    commandMessage(message);
}

void LC_ActionOffsetCurveUpdateSource::doTriggerCompletion(bool) {
    const QPointer<LC_ActionOffsetCurveUpdateSource> alive(this);
    publishReport();
    if (!alive) return;
    if (m_report.sourceRefusal.isEmpty() && currentContext()) finish();
    else if (!currentContext()) { m_finished = true; setFinished(); }
    else updateActionPrompt();
}

void LC_ActionOffsetCurveUpdateSource::onMouseRightButtonReleaseSelected(int, const LC_MouseEvent*) {
    if (currentContext()) finish();
}

void LC_ActionOffsetCurveUpdateSource::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select offset curves") + getSelectionCompletionHintMsg(),
        MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Complete selection")));
}

void LC_ActionOffsetCurveUpdateSource::updateActionPromptForSelected(int) {
    updatePromptTRCancel(isPreparing() ? tr("Updating offset curves...") :
        tr("Select source %1").arg(sourceKindName(m_sourceKind)));
}

LC_ActionOptionsWidget* LC_ActionOffsetCurveUpdateSource::createOptionsWidget() {
    return new UpdateSourceOptions();
}

RS2::CursorType LC_ActionOffsetCurveUpdateSource::doGetMouseCursorSelected(int) {
    return RS2::SelectCursor;
}
