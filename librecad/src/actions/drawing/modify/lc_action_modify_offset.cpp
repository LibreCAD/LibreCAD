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

#include "lc_action_modify_offset.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <array>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

#include <QStringList>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QTimer>

#include "lc_actioninfomessagebuilder.h"
#include "lc_curveoffset.h"
#include "lc_offset_options_filler.h"
#include "lc_offset_options_widget.h"
#include "lc_offsetcurveui.h"
#include "lc_offsetcurveeditsnapshot.h"
#include "lc_offsetcurvefinalinput.h"
#include "lc_offsetcurvepublication.h"
#include "lc_offsetbatchpreparation.h"
#include "../../../lib/offset_worker/lc_offsetworkerclient.h"
#include "lc_actioncontext.h"
#include "lc_graphicviewport.h"
#include "lc_highlight.h"
#include "lc_visual_snap_manager.h"
#include "rs_document.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "rs_polyline.h"
#include "rs_modification.h"
#include "rs_settings.h"

namespace {
    bool sameValue(double a, double b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }
    bool sameValue(const RS_Vector& a, const RS_Vector& b) {
        return a.valid == b.valid && sameValue(a.x, b.x) && sameValue(a.y, b.y) && sameValue(a.z, b.z);
    }
    bool sameOptions(const RS_OffsetData& a, const RS_OffsetData& b) {
        return a.number == b.number && a.useCurrentAttributes == b.useCurrentAttributes &&
            a.useCurrentLayer == b.useCurrentLayer && a.keepOriginals == b.keepOriginals &&
            a.multipleCopies == b.multipleCopies && a.asOffsetCurve == b.asOffsetCurve &&
            sameValue(a.distance, b.distance) && sameValue(a.coord, b.coord) && sameValue(a.sideFallback, b.sideFallback);
    }
    bool sameActivePen(const RS_Pen& a, const RS_Pen& b) {
        return a == b && a.getFlags() == b.getFlags() && a.getLineTypeId() == b.getLineTypeId() &&
            sameValue(a.getScreenWidth(), b.getScreenWidth()) && sameValue(a.getAlpha(), b.getAlpha()) &&
            sameValue(a.dashOffset(), b.dashOffset()) && a.getColor().colorName() == b.getColor().colorName();
    }
    void promoteNativeSources(QList<RS_Entity*>& sources) {
        QList<RS_Entity*> promoted;
        std::unordered_set<const RS_Entity*> natives;
        for (auto* entity : sources) {
            if (entity != nullptr && entity->getParent() != nullptr &&
                entity->getParent()->rtti() == RS2::EntityOffsetCurve) entity = entity->getParent();
            if (entity != nullptr && entity->rtti() == RS2::EntityOffsetCurve && !natives.insert(entity).second) continue;
            promoted.append(entity);
        }
        sources.swap(promoted);
    }
}

struct LC_ActionModifyOffset::Work {
    qint64 serial = 0;
    RS_Document* document = nullptr;
    std::uint64_t undoEpoch = 0;
    bool preview = false, fixed = false, keepSelected = false;
    int status = 0;
    RS_OffsetData options;
    RS_Pen activePen;
    RS_Layer* activeLayer = nullptr;
    QList<RS_Entity*> selected, actionSources;
    std::array<RS_Vector, 3> world, screen;
    RS_Vector zero, markedZero;
    double angle = 0;
    bool counterClockwise = true;
    QElapsedTimer elapsed;
    std::shared_ptr<LC_OffsetCreationSources> inputs;
    std::unique_ptr<LC_OffsetBatchPreparation> preparation;
    LC_DocumentModificationBatch batch;
};

LC_ActionModifyOffset::LC_ActionModifyOffset(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("ActionModifyOffset", actionContext,RS2::ActionModifyOffset,
                         {RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse, RS2::EntityLine, RS2::EntityPolyline,
                          RS2::EntitySpline, RS2::EntityOffsetCurve, RS2::EntitySplinePoints,
                          RS2::EntityParabola, RS2::EntityHyperbola})
    , m_offsetData(new RS_OffsetData())
    , m_previewCache(std::make_unique<LC_OffsetPreviewCache>()){

    m_offsetData->distance = 0.;
    m_offsetData->number = 1;
    m_offsetData->useCurrentAttributes = true;
    m_offsetData->useCurrentLayer = true;
    m_liveView = m_graphicView;
    if (m_document) m_document->getSelection()->addListener(this);
    if (m_graphic && m_graphic != m_document) m_graphic->getSelection()->addListener(this);
    connect(m_graphicView, &QObject::destroyed, this, [this] { detachContext(); });
    m_worker = std::make_unique<LC_OffsetWorkerClient>();
    m_guardTimer = std::make_unique<QTimer>(); m_guardTimer->setInterval(25);
    connect(m_guardTimer.get(), &QTimer::timeout, this, [this] {
        const auto work = m_work; if (!work) return;
        try {
            const auto limit = work->preview ? LC_OffsetWorkerClient::Deadlines::preview : LC_OffsetWorkerClient::Deadlines::commit;
            if (work->elapsed.elapsed() >= limit) cancelPreparation(tr("Offset preparation timed out; the drawing was not changed."));
            else if (!currentWork(work)) cancelPreparation(tr("Offset inputs changed; the drawing was not changed."));
        } catch (...) { cancelPreparation(tr("Offset preparation could not retain its bounded input snapshot.")); }
    });
}

LC_ActionModifyOffset::~LC_ActionModifyOffset() { cancelPreparation(); m_worker.reset(); }

bool LC_ActionModifyOffset::isPreparing() const noexcept { return bool(m_work); }

bool LC_ActionModifyOffset::currentContext() const {
    return m_actionContext && m_document && m_graphic && m_liveView && !m_liveView->isClosing() &&
        m_liveView.data() == m_graphicView && m_liveView->getDocument() == m_document &&
        m_liveView->getViewPort() == m_viewport && m_actionContext->getDocument() == m_document &&
        m_actionContext->getGraphicView() == m_liveView.data() && m_document->isOwner() &&
        (dynamic_cast<const RS_Graphic*>(m_document) || dynamic_cast<const RS_Block*>(m_document)) &&
        LC_OffsetCurveFinalInput::available(*m_document, true, !dynamic_cast<const RS_Block*>(m_document));
}

void LC_ActionModifyOffset::selectedSetDestroyed() { detachContext(); }

void LC_ActionModifyOffset::detachContext() {
    cancelPreparation(); m_previewCache->clear(); m_sourceInputs.reset();
    m_document = nullptr; m_graphic = nullptr; m_keyEntity = nullptr;
    if (m_preview) { m_preview->setParent(nullptr); m_preview->clear(); }
    for (auto* entity : *m_highlight) { entity->setParent(nullptr); entity->clearSelectionFlag(); }
    m_highlight->clear(); m_visualSnapManager->detachFromView();
    m_graphicView = nullptr; m_viewport = nullptr; m_formatter = nullptr; m_liveView = nullptr;
    m_selectedEntities.clear(); m_selectionComplete = false; m_finished = true; setFinished();
}

void LC_ActionModifyOffset::init(int status) {
    cancelPreparation();
    if (currentContext()) LC_ActionModifyBase::init(status);
}

void LC_ActionModifyOffset::suspend() {
    cancelPreparation();
    if (currentContext()) LC_ActionModifyBase::suspend();
}

void LC_ActionModifyOffset::keyPressEvent(QKeyEvent* event) {
    if (isPreparing() && event->key() == Qt::Key_Escape) { finish(); event->accept(); return; }
    if (currentContext()) LC_ActionModifyBase::keyPressEvent(event);
    else event->accept();
}

bool LC_ActionModifyOffset::selectionNeedsWorker() const {
    if (m_sourceInputs && m_previewSources == m_selectedEntities && m_sourceInputs->needsWorker()) return true;
    std::unordered_set<const RS_Entity*> missing(m_selectedEntities.begin(), m_selectedEntities.end());
    for (auto* entity : *m_document) {
        if (missing.erase(entity) && (dynamic_cast<const RS_Ellipse*>(entity) || dynamic_cast<const LC_OffsetCurve*>(entity)))
            return true;
    }
    if (missing.empty()) return false;
    std::size_t searched = 0;
    for (auto* entity : *m_document) {
        const auto* container = dynamic_cast<const RS_EntityContainer*>(entity);
        if (!container || (!dynamic_cast<const RS_Polyline*>(entity) && !dynamic_cast<const LC_OffsetCurve*>(entity))) continue;
        if (container->size() > LC_OffsetCreationSources::MaximumNodes - searched) return true;
        searched += container->size();
        for (const auto* child : *container) if (missing.erase(child) && dynamic_cast<const LC_OffsetCurve*>(entity)) return true;
        if (missing.empty()) return false;
    }
    return true; // Unknown/stale retained addresses must refuse owned admission, never reach synchronous dereferences.
}

void LC_ActionModifyOffset::trigger() {
    if (!currentContext() || (m_work && !m_work->preview)) return;
    if (!selectionNeedsWorker()) {
        cancelPreparation();
        if (m_sourceInputs) { m_previewCache->clear(); m_sourceInputs.reset(); m_previewSources.clear(); }
        LC_ActionModifyBase::trigger(); return;
    }
    const QPointer<LC_ActionModifyOffset> alive(this);
    deletePreview(); deleteInfoCursor(); deleteHighlights(); deleteSnapper();
    if (alive && currentContext()) beginPreparation(false);
}

bool LC_ActionModifyOffset::currentWork(const std::shared_ptr<Work>& work) const {
    if (m_work != work || work->serial != m_serial || !currentContext() || m_document != work->document ||
        m_document->undoStateEpoch() != work->undoEpoch || getStatus() != work->status ||
        m_selectedEntities != work->actionSources || m_distanceIsFixed != work->fixed ||
        !sameOptions(*m_offsetData, work->options) || isKeepModifiedEntitiesSelected() != work->keepSelected ||
        m_graphic->getActiveLayer() != work->activeLayer || !sameActivePen(m_graphic->getActivePen(), work->activePen) ||
        work->elapsed.elapsed() >= (work->preview ? LC_OffsetWorkerClient::Deadlines::preview : LC_OffsetWorkerClient::Deadlines::commit)) return false;
    auto it = m_document->getSelection()->begin();
    for (auto* address : work->selected) { if (it == m_document->getSelection()->end() || *it != address) return false; ++it; }
    if (it != m_document->getSelection()->end()) return false;
    const std::array<RS_Vector, 3> world{m_viewport->toWorld({0, 0}), m_viewport->toWorld({1, 0}), m_viewport->toWorld({0, 1})};
    for (std::size_t i = 0; i < world.size(); ++i) if (!sameValue(world[i], work->world[i])) return false;
    const std::array<RS_Vector, 3> screen{m_viewport->toWorldFromUi(0, 0), m_viewport->toWorldFromUi(1, 0), m_viewport->toWorldFromUi(0, 1)};
    for (std::size_t i = 0; i < screen.size(); ++i) if (!sameValue(screen[i], work->screen[i])) return false;
    if (!sameValue(m_viewport->getRelativeZero(), work->zero) || !sameValue(m_viewport->getMarkedRelativeZero(), work->markedZero)) return false;
    return m_viewport->toWorldAngle(m_viewport->toAbsUCSAngle(0)) == work->angle &&
        m_viewport->areAnglesCounterClockwise() == work->counterClockwise && work->inputs->matches(*m_document);
}

void LC_ActionModifyOffset::cancelPreparation(const QString& reason) {
    const auto work = std::exchange(m_work, {});
    if (m_serial < std::numeric_limits<qint64>::max()) ++m_serial;
    if (m_guardTimer) m_guardTimer->stop();
    if (m_worker) m_worker->cancel();
    if (!work) return;
    m_pendingOutcome.reset();
    if (!reason.isEmpty() && currentContext()) {
        const QPointer<LC_ActionModifyOffset> alive(this);
        deletePreview(); m_preview->clear();
        if (!work->preview) commandMessage(reason);
        if (alive && currentContext()) redrawDrawing();
    }
}

void LC_ActionModifyOffset::beginPreparation(bool preview) {
    cancelPreparation();
    const QPointer<LC_ActionModifyOffset> alive(this);
    try {
        QElapsedTimer elapsed; elapsed.start();
        if (!currentContext() || m_serial == std::numeric_limits<qint64>::max() ||
            m_document->undoStateEpoch() > std::uint64_t(std::numeric_limits<qint64>::max())) return;
        if (!m_sourceInputs || m_previewSources != m_selectedEntities || !m_sourceInputs->matches(*m_document)) {
            m_previewCache->clear(); m_sourceInputs.reset();
            m_sourceInputs = LC_OffsetCreationSources::capture(*m_document, m_selectedEntities);
            m_previewSources = m_selectedEntities;
        }
        if (!m_sourceInputs) throw std::runtime_error("Offset sources exceed owned snapshot admission");
        auto work = std::make_shared<Work>(); work->serial = ++m_serial; work->elapsed = elapsed;
        work->document = m_document; work->undoEpoch = m_document->undoStateEpoch(); work->preview = preview;
        work->options = *m_offsetData; work->fixed = m_distanceIsFixed; work->status = getStatus();
        work->keepSelected = isKeepModifiedEntitiesSelected(); work->actionSources = m_selectedEntities;
        work->activeLayer = m_graphic->getActiveLayer(); work->activePen = m_graphic->getActivePen();
        for (auto* selected : *m_document->getSelection()) {
            if (work->selected.size() == 4096) throw std::length_error("Offset selected inventory exceeded");
            work->selected.append(selected);
        }
        work->world = {m_viewport->toWorld({0, 0}), m_viewport->toWorld({1, 0}), m_viewport->toWorld({0, 1})};
        work->screen = {m_viewport->toWorldFromUi(0, 0), m_viewport->toWorldFromUi(1, 0), m_viewport->toWorldFromUi(0, 1)};
        work->zero = m_viewport->getRelativeZero(); work->markedZero = m_viewport->getMarkedRelativeZero();
        work->angle = m_viewport->toWorldAngle(m_viewport->toAbsUCSAngle(0));
        work->counterClockwise = m_viewport->areAnglesCounterClockwise(); work->inputs = m_sourceInputs;
        work->preparation = std::make_unique<LC_OffsetBatchPreparation>(work->options, work->inputs->sources(),
            preview, preview ? LC_OffsetBatchLimits::preview() : LC_OffsetBatchLimits{});
        m_work = work; m_guardTimer->start();
        QTimer::singleShot(0, this, [alive, work] { if (alive) alive->advance(work); });
    } catch (...) {
        if (!alive) return;
        cancelPreparation();
        if (currentContext() && !preview) commandMessage(tr("Offset could not prepare its bounded input snapshot; the drawing was not changed."));
    }
}

void LC_ActionModifyOffset::advance(const std::shared_ptr<Work>& work) {
    if (m_work != work) return;
    const QPointer<LC_ActionModifyOffset> alive(this);
    try {
        if (!currentWork(work)) { cancelPreparation(tr("Offset inputs changed; the drawing was not changed.")); return; }
        const auto limit = work->preview ? LC_OffsetWorkerClient::Deadlines::preview : LC_OffsetWorkerClient::Deadlines::commit;
        if (work->elapsed.elapsed() >= limit) { cancelPreparation(tr("Offset preparation timed out; the drawing was not changed.")); return; }
        if (work->preparation->advance(work->batch, m_previewCache.get())) { completePreparation(work); return; }
        if (m_requestId == std::numeric_limits<qint64>::max()) throw std::length_error("Offset request identity exhausted");
        auto request = work->preparation->request({++m_requestId, work->serial, work->serial, qint64(work->undoEpoch)}, m_previewCache.get());
        if (!request) throw std::runtime_error("Offset source transport admission refused");
        const bool submitted = m_worker->submit(*request, work->preview ? LC_OffsetWorkerClient::Mode::Preview : LC_OffsetWorkerClient::Mode::Commit,
            [alive, work](LC_OffsetWorkerClient::Outcome outcome) {
                if (!alive || alive->m_work != work) return;
                try {
                    if (!alive->currentWork(work)) { alive->cancelPreparation(tr("Offset inputs changed; the drawing was not changed.")); return; }
                    if (outcome.status != LC_OffsetWorkerClient::Status::Completed || !outcome.completed || !outcome.reaped ||
                        !work->preparation->accept(*outcome.completed)) {
                        alive->cancelPreparation(tr("Offset worker preparation failed; the drawing was not changed.")); return;
                    }
                    QTimer::singleShot(0, alive.data(), [alive, work] { if (alive) alive->advance(work); });
                } catch (...) { if (alive) alive->cancelPreparation(tr("Offset worker preparation was refused; the drawing was not changed.")); }
            });
        if (!submitted) cancelPreparation(tr("Offset worker admission failed; the drawing was not changed."));
    } catch (...) { if (alive) cancelPreparation(tr("Offset preparation was refused; the drawing was not changed.")); }
}

void LC_ActionModifyOffset::completePreparation(const std::shared_ptr<Work>& work) {
    if (!currentWork(work)) { cancelPreparation(tr("Offset inputs changed; the drawing was not changed.")); return; }
    auto& batch = work->batch;
    auto report = std::make_unique<LC_OffsetBatchOutcome>(work->preparation->outcome());
    for (auto& source : report->sources) source.source = work->inputs->original(source.source);
    if (!work->inputs->localize(batch.entitiesToAdd, work->activeLayer, batch.setActiveLayer))
        throw std::runtime_error("Offset private layer rebinding failed");
    for (auto* entity : std::as_const(batch.entitiesToAdd)) entity->setParent(m_document);
    if (batch.setActivePen) for (auto* entity : std::as_const(batch.entitiesToAdd)) entity->setPen(work->activePen);
    m_workerReport = std::make_unique<LC_OffsetBatchOutcome>(*report);
    const QPointer<LC_ActionModifyOffset> alive(this);
    if (work->preview) {
        if (!currentWork(work)) { cancelPreparation(); return; }
        deletePreview(); m_preview->clear();
        if (std::size_t(batch.entitiesToAdd.size()) <= maxPreviewDetail()) {
            auto owners = work->preparation->take();
            for (auto& entity : owners) { m_preview->addEntity(entity.get()); entity.release(); }
        } else {
            RS_Vector lo{false}, hi{false};
            for (const auto* entity : std::as_const(batch.entitiesToAdd)) if (entity->hasValidBorders()) {
                lo = lo.valid ? RS_Vector::minimum(lo, entity->getMin()) : entity->getMin();
                hi = hi.valid ? RS_Vector::maximum(hi, entity->getMax()) : entity->getMax();
            }
            if (lo.valid && hi.valid) {
                previewRefLine({lo.x, lo.y}, {hi.x, lo.y}); previewRefLine({hi.x, lo.y}, {hi.x, hi.y});
                previewRefLine({hi.x, hi.y}, {lo.x, hi.y}); previewRefLine({lo.x, hi.y}, {lo.x, lo.y});
            }
        }
        if (work->status == SetPosition && m_showRefEntitiesOnPreview) {
            previewRefPoint(work->options.coord); previewRefSelectablePoint(work->options.sideFallback);
            previewRefLine(work->options.coord, work->options.sideFallback);
        }
        m_guardTimer->stop();
        if (isInfoCursorForModificationEnabled()) for (const auto& source : report->sources) if (!source.complete()) {
            appendInfoCursorZoneMessage(source.succeeded() ? stopReason(source) : failureReason(source, true), 2, false); break;
        }
        if (!alive || m_work != work) return;
        drawPreview();
        if (!alive || m_work != work) return;
        if (!currentWork(work)) { cancelPreparation(tr("Offset preview inputs changed.")); return; }
        m_work.reset();
        if (currentContext()) redrawDrawing();
        return;
    }
    if (!report->anySourceSucceeded()) {
        m_work.reset(); m_guardTimer->stop(); m_pendingOutcome = std::move(report); doTriggerCompletion(false); return;
    }
    QList<RS_Entity*> removed, deselected;
    for (auto* source : std::as_const(batch.entitiesToDelete)) removed.append(work->inputs->original(source));
    for (const auto& source : report->sources) if (source.succeeded()) deselected.append(const_cast<RS_Entity*>(source.source));
    // Match existing copy provenance policy before the no-callback model seal.
    std::unordered_set<quint32> freed;
    std::unordered_set<unsigned long long> freedLineages;
    for (const auto* source : std::as_const(removed)) {
        if (source->sourceHandle()) freed.insert(source->sourceHandle());
        if (source->ownerLineage()) freedLineages.insert(source->lineage());
    }
    std::map<unsigned long long, int> copies;
    std::map<std::pair<unsigned long long, int>, unsigned long long> lineages;
    const auto copiedLineage = [&](unsigned long long source, int number) {
        auto& made = lineages[{source, number}];
        if (!made) made = RS_Entity::newLineage();
        return made;
    };
    for (auto* entity : std::as_const(batch.entitiesToAdd)) {
        if (!entity->sourceHandle() || !freed.erase(entity->sourceHandle())) entity->clearDwgProvenance(RS_Entity::Identity);
        if (entity->ownerLineage()) {
            const auto original = entity->lineage(); const auto number = ++copies[original];
            if (!freedLineages.erase(original)) {
                entity->setLineage(copiedLineage(original, number));
                entity->setOwnerLineage(copiedLineage(entity->ownerLineage(), number));
            }
        }
    }
    auto publication = LC_OffsetCurvePublication::prepareCreation(*m_document, *m_viewport, removed,
        batch.entitiesToAdd, work->keepSelected ? batch.entitiesToAdd : QList<RS_Entity*>{}, deselected, work->inputs->roots());
    m_pendingOutcome = std::move(report);
    if (!publication || !currentWork(work) || !publication->commit()) {
        cancelPreparation(tr("Offset inputs changed before publication; the drawing was not changed.")); return;
    }
    work->preparation->release(); // No callback/allocation between commit and ownership transfer.
    m_work.reset(); m_guardTimer->stop(); publication->notify();
    if (!alive || !currentContext()) return;
    m_viewport->notifyChanged();
    if (alive && currentContext()) doTriggerCompletion(true);
}

bool LC_ActionModifyOffset::isEditableOffsetCurve() const {
    return m_offsetData->asOffsetCurve;
}

void LC_ActionModifyOffset::setEditableOffsetCurve(bool enabled) const {
    m_offsetData->asOffsetCurve = enabled;
}

void LC_ActionModifyOffset::doSaveOptions() {
    save("Distance", getDistance());
    save("DistanceFixed",isFixedDistance());

    save("UseCurrentLayer", isUseCurrentLayer());
    save("UseCurrentAttributes", isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
    save("MultipleCopies", isUseMultipleCopies());
    save("Copies", getCopiesNumber());
    save("EditableOffsetCurve", isEditableOffsetCurve());
}

void LC_ActionModifyOffset::doLoadOptions() {
    const double dist = loadDouble("Distance", 10.0);
    setDistance(dist);

    const bool distFixed = loadBool("DistanceFixed",false);
    setDistanceFixed(distFixed);

    const bool curLayer = loadBool("UseCurrentLayer", true);
    setUseCurrentLayer(curLayer);
    const bool curAtts = loadBool("UseCurrentAttributes", true);
    setUseCurrentAttributes(curAtts);

    // as AutoCAD's "Erase source=No": an offset adds to the drawing
    const bool keepOriginals = loadBool("KeepOriginals", true);
    setKeepOriginals(keepOriginals);

    const bool multiCopy  = loadBool("MultipleCopies", false);
    setUseMultipleCopies(multiCopy);

    const int copies = loadInt("Copies", 1);
    setCopiesNumber(copies);
    // The interactive default belongs to the saved-options lifecycle, not construction.
    setEditableOffsetCurve(loadBool("EditableOffsetCurve", true));
}

bool LC_ActionModifyOffset::isInVisualSnapStatus(const int status) {
    return (status == SetReferencePoint) || (status == SetPosition);
}

bool LC_ActionModifyOffset::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
    promoteNativeSources(m_selectedEntities);
    // the sources may be replaced: what the preview kept for them is dropped
    m_previewCache->clear();
    m_previewSources.clear();
    m_pendingOutcome = std::make_unique<LC_OffsetBatchOutcome>(RS_Modification::offsetWithOutcome(
        *m_offsetData, m_selectedEntities, false, LC_OffsetBatchLimits{}, ctx));
    return m_pendingOutcome->anySourceSucceeded();
}

QString LC_ActionModifyOffset::failureReason(const LC_OffsetSourceOutcome& source, const bool preview) {
    switch (source.status) {
        case LC_OffsetSourceStatus::Succeeded:
            return {};
        case LC_OffsetSourceStatus::NotVisibleOrLocked:
            return tr("hidden or locked");
        case LC_OffsetSourceStatus::TargetLayerUnavailable:
            return tr("the current layer is missing, frozen or locked");
        case LC_OffsetSourceStatus::Vanished:
            return tr("nothing is left at this distance");
        case LC_OffsetSourceStatus::LimitExceeded:
            return preview ? tr("too complex to preview; a click offsets it in full") : tr("the offset is too complex");
        case LC_OffsetSourceStatus::InvalidSource:
            return tr("it cannot be offset");
        case LC_OffsetSourceStatus::OffsetFailed:
            break;
    }
    return offsetFailureReason(source);
}

QString LC_ActionModifyOffset::stopReason(const LC_OffsetSourceOutcome& source) {
    if (source.stoppedBy == LC_OffsetSourceStatus::OffsetFailed) {
        return offsetFailureReason(source);
    }
    return tr("nothing is left at this distance");
}

QString LC_ActionModifyOffset::offsetFailureReason(const LC_OffsetSourceOutcome& source) {
    switch (source.detail) {
        case LC_OffsetFailureDetail::None:
            break;
        case LC_OffsetFailureDetail::ZeroLength:
            return tr("it has no length");
        case LC_OffsetFailureDetail::Unbounded:
            return tr("it is unbounded");
        case LC_OffsetFailureDetail::EllipticSegments:
            return tr("it has elliptic segments");
        case LC_OffsetFailureDetail::PolylineNotTrimmed:
            return tr("its offset would cross itself, which polylines do not support yet");
        case LC_OffsetFailureDetail::SourceRetraces:
            return tr("it runs over itself");
        case LC_OffsetFailureDetail::InvalidDistance:
            return tr("the distance is not valid");
        case LC_OffsetFailureDetail::AmbiguousSide:
            return tr("the point does not show which side to offset to");
    }
    switch (source.engineStatus) {
        case LC_CurveOffsetStatus::InvalidSource:
            return tr("it cannot be offset");
        case LC_CurveOffsetStatus::UnsupportedNonPlanar:
            return tr("it does not lie in the drawing plane");
        case LC_CurveOffsetStatus::AmbiguousSide:
            // on the entity, or as near to sides that disagree
            return tr("the point does not show which side to offset to");
        case LC_CurveOffsetStatus::UndefinedTangent:
            return tr("the curve does not move, or has a point where its direction cannot be found");
        case LC_CurveOffsetStatus::DiscontinuousNormal:
            return tr("the curve has a gap or a corner the offset cannot join");
        case LC_CurveOffsetStatus::SingularOffset:
        case LC_CurveOffsetStatus::AmbiguousRegularity:
            return tr("the distance meets the curve's radius of curvature where that cannot be resolved");
        case LC_CurveOffsetStatus::AmbiguousTopology:
            return tr("the offset touches itself where it cannot be trimmed reliably");
        case LC_CurveOffsetStatus::FitFailed:
        case LC_CurveOffsetStatus::ToleranceNotMet:
            return tr("the offset cannot be fitted within the tolerance");
        case LC_CurveOffsetStatus::InvalidDistance:
            return tr("the distance is not valid");
        default:
            // InvalidRequest among them: a programming error, not the user's
            return tr("the offset could not be made");
    }
}

QString LC_ActionModifyOffset::keptPlainReason(
    const LC_OffsetSourceOutcome& source, const LC_OffsetPlainOutcome& plain) {
    switch (plain.reason) {
    case LC_OffsetPlainReason::NotTopLevel:
        return {}; // no native-request notices for nested, detached or staged-only sources
    case LC_OffsetPlainReason::SeveralPieces:
        return tr("the offset has several pieces");
    case LC_OffsetPlainReason::OutputBudget:
        return tr("the editable cache would exceed the output budget");
    case LC_OffsetPlainReason::ZeroSum:
        return tr("the accumulated offset is exactly zero");
    case LC_OffsetPlainReason::UndefinedDefinition:
        return tr("the native definition is undefined or inconsistent");
    case LC_OffsetPlainReason::CheckFailed:
    case LC_OffsetPlainReason::NotCacheRepresentable: {
        LC_OffsetCurveEditResult result;
        result.refusal = plain.reason == LC_OffsetPlainReason::CheckFailed
            ? LC_OffsetCurveRefusal::CheckFailed : LC_OffsetCurveRefusal::NotCacheRepresentable;
        result.checkReason = plain.checkReason;
        return LC_OffsetCurveUI::refusalText(result);
    }
    case LC_OffsetPlainReason::RebaseRefused:
        break;
    }
    for (const auto& attempt : source.rebases) {
        if (attempt.copy != plain.copy) continue;
        if (attempt.nativeRefusal && *attempt.nativeRefusal != LC_OffsetCurveRefusal::None) {
            LC_OffsetCurveEditResult result;
            result.refusal = *attempt.nativeRefusal;
            result.engineStatus = attempt.engineStatus;
            result.checkReason = attempt.checkReason;
            return tr("native rebasing was refused: %1").arg(LC_OffsetCurveUI::refusalText(result));
        }
        if (attempt.proposalRefusal) {
            using Refusal = LC_OffsetCurveAgain::Refusal;
            switch (*attempt.proposalRefusal) {
            case Refusal::None:
                return tr("the remaining work allowance did not admit native rebasing");
            case Refusal::InvalidDefinition:
                return tr("the native source definition is invalid");
            case Refusal::NonCanonicalBasis:
                return tr("the native source basis is not canonical");
            case Refusal::InvalidIncrement:
                return tr("the offset increment is invalid");
            case Refusal::UnresolvedSide:
                return tr("the cache side could not be resolved");
            case Refusal::InvalidCurrentProfile:
                return tr("the retained native profile is invalid");
            case Refusal::ZeroSum:
                return tr("the accumulated offset is exactly zero");
            case Refusal::NonFiniteCombination:
                return tr("the accumulated distance is not finite");
            case Refusal::InexactCombination:
                return tr("the accumulated distance cannot be represented exactly");
            case Refusal::FirstNotProvedUntrimmed:
                return tr("the first inward profile is not proved untrimmed");
            case Refusal::FinalNotProvedUntrimmed:
                return tr("the final arc profile is not proved untrimmed");
            }
        }
        break;
    }
    return tr("the native retained-set composition was not certified");
}

QString LC_ActionModifyOffset::keptPlainMessage(const LC_OffsetBatchOutcome& outcome) {
    // Four fixed-vocabulary reason buckets plus one counted remainder: no
    // per-source output, source names, pointer dereferences or unbounded text.
    struct Bucket { QString reason; qulonglong count = 0; };
    constexpr std::size_t maximumReasons = 4;
    std::vector<Bucket> buckets;
    buckets.reserve(maximumReasons);
    qulonglong copies = 0, sources = 0, otherCopies = 0;
    for (const auto& source : outcome.sources) {
        if (!source.succeeded()) continue; // attempted/rolled-back output is not a committed plain copy
        bool countedSource = false;
        for (const auto& plain : source.keptPlain) {
            const QString reason = keptPlainReason(source, plain);
            if (reason.isEmpty()) continue;
            ++copies;
            if (!countedSource) { ++sources; countedSource = true; }
            auto bucket = std::find_if(buckets.begin(), buckets.end(),
                [&](const Bucket& item) { return item.reason == reason; });
            if (bucket != buckets.end()) ++bucket->count;
            else if (buckets.size() < maximumReasons) buckets.push_back({reason, 1});
            else ++otherCopies;
        }
    }
    if (!copies) return {};
    QStringList reasons;
    for (const auto& bucket : buckets)
        reasons.append(tr("%1: %2").arg(bucket.count).arg(bucket.reason));
    if (otherCopies) reasons.append(tr("%1 further copies had other plain fallback reasons").arg(otherCopies));
    return tr("Offset copies kept plain: %1 (sources: %2). %3")
        .arg(copies).arg(sources).arg(reasons.join(QStringLiteral("; ")));
}

void LC_ActionModifyOffset::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    // Only sources that were offset change selection; a failed source stays
    // selected. A removed source is known by identity only and never touched.
    // One bulk call each way: without additive selection, select() clears
    // whatever it was not given.
    if (!ctx.success || m_pendingOutcome == nullptr) {
        return;
    }
    std::unordered_set<const RS_Entity*> offsetSources;
    std::unordered_set<const RS_Entity*> removedSources;
    QList<RS_Entity*> toSelect;
    for (const LC_OffsetSourceOutcome& source : std::as_const(m_pendingOutcome->sources)) {
        if (source.succeeded()) {
            offsetSources.insert(source.source);
            toSelect.append(source.createdEntities);
            if (source.sourceRemoved) {
                removedSources.insert(source.source);
            }
        }
    }
    QList<RS_Entity*> toUnselect;
    for (RS_Entity* selected : std::as_const(m_selectedEntities)) {
        if (offsetSources.count(selected) == 0) {
            toSelect.append(selected); // not offset: stays selected
        }
        else if (removedSources.count(selected) == 0) {
            toUnselect.append(selected); // offset, and still in the drawing
        }
    }
    if (!toUnselect.isEmpty()) {
        unselect(toUnselect);
    }
    if (keepSelected) {
        select(toSelect);
    }
}

void LC_ActionModifyOffset::doTriggerCompletion(const bool success) {
    const QPointer<LC_ActionModifyOffset> alive(this);
    int failed = 0;
    int total = 0;
    QStringList reasons;
    // sources that got fewer copies than asked for, and the copies of the last
    // of them: since nothing was left, or since a copy could not be made
    int shortOfCopies = 0;
    int made = 0;
    int requested = 0;
    int stoppedByFailure = 0;
    int stoppedMade = 0;
    int stoppedRequested = 0;
    QStringList stopReasons;
    QString plainNotice;
    if (m_pendingOutcome != nullptr) {
        for (const LC_OffsetSourceOutcome& source : std::as_const(m_pendingOutcome->sources)) {
            ++total;
            if (!source.succeeded()) {
                ++failed;
                const QString reason = failureReason(source, false);
                if (!reasons.contains(reason)) {
                    reasons.append(reason);
                }
            }
            else if (!source.complete()) {
                if (source.stoppedBy == LC_OffsetSourceStatus::OffsetFailed) {
                    ++stoppedByFailure;
                    stoppedMade = source.copiesMade;
                    stoppedRequested = source.copiesRequested;
                    const QString reason = stopReason(source);
                    if (!stopReasons.contains(reason)) {
                        stopReasons.append(reason);
                    }
                }
                else {
                    ++shortOfCopies;
                    made = source.copiesMade;
                    requested = source.copiesRequested;
                }
            }
        }
    }
    if (m_pendingOutcome != nullptr) plainNotice = keptPlainMessage(*m_pendingOutcome);
    m_pendingOutcome.reset();
    if (failed > 0) {
        commandMessage(tr("%1 of %2 selected entities could not be offset: %3")
                           .arg(failed)
                           .arg(total)
                           .arg(reasons.join(QStringLiteral("; "))));
        if (!alive || !currentContext()) return;
    }
    if (shortOfCopies == 1) {
        commandMessage(tr("Only %1 of %2 copies fit, since nothing is left at a larger distance; the original was kept")
                           .arg(made)
                           .arg(requested));
        if (!alive || !currentContext()) return;
    }
    else if (shortOfCopies > 1) {
        commandMessage(tr("%1 selected entities got fewer than %2 copies, since nothing is left at a larger distance; "
                          "their originals were kept")
                           .arg(shortOfCopies)
                           .arg(requested));
        if (!alive || !currentContext()) return;
    }
    if (stoppedByFailure == 1) {
        commandMessage(tr("Only %1 of %2 copies were made: %3; the original was kept")
                           .arg(stoppedMade)
                           .arg(stoppedRequested)
                           .arg(stopReasons.join(QStringLiteral("; "))));
        if (!alive || !currentContext()) return;
    }
    else if (stoppedByFailure > 1) {
        commandMessage(tr("%1 selected entities got fewer than %2 copies: %3; their originals were kept")
                           .arg(stoppedByFailure)
                           .arg(stoppedRequested)
                           .arg(stopReasons.join(QStringLiteral("; "))));
        if (!alive || !currentContext()) return;
    }
    if (!plainNotice.isEmpty()) commandMessage(plainNotice);
    if (!alive || !currentContext()) return;
    if (success) {
        finish();
        return;
    }
    // Nothing was offset: keep the selection and the current step, so another
    // side or distance can be tried.
    deletePreview();
    updateActionPrompt();
    redrawDrawing();
}

void LC_ActionModifyOffset::finish() {
    cancelPreparation();
    m_pendingOutcome.reset();
    m_previewCache->clear();
    m_previewSources.clear();
    m_sourceInputs.reset();
    if (currentContext()) LC_ActionModifyBase::finish();
    else { m_finished = true; setFinished(); }
}

std::size_t LC_ActionModifyOffset::maxPreviewDetail() {
    const int configured = LC_GET_ONE_INT("Appearance", "MaxPreview", 100);
    return configured > 0 ? static_cast<std::size_t>(configured) : 0;
}

void LC_ActionModifyOffset::previewOffset() {
    if (!currentContext() || (m_work && !m_work->preview)) return;
    if (selectionNeedsWorker()) { beginPreparation(true); return; }
    cancelPreparation();
    if (m_sourceInputs) { m_previewCache->clear(); m_sourceInputs.reset(); m_previewSources.clear(); }
    promoteNativeSources(m_selectedEntities);
    // made on every mouse move: with preview limits, and from what was made
    // before for the same sources, side and distance
    if (m_previewSources != m_selectedEntities) {
        m_previewCache->clear();
        m_previewSources = m_selectedEntities;
    }
    LC_DocumentModificationBatch ctx;
    const LC_OffsetBatchOutcome outcome = RS_Modification::offsetWithOutcome(
        *m_offsetData, m_selectedEntities, true, LC_OffsetBatchLimits::preview(), ctx, m_previewCache.get());
    if (ctx.entitiesToAdd.isEmpty()) {
        // no acceptable offset now: the preview stays empty, and says why
        if (isInfoCursorForModificationEnabled()) {
            for (const LC_OffsetSourceOutcome& source : outcome.sources) {
                if (!source.succeeded()) {
                    appendInfoCursorZoneMessage(failureReason(source, true), 2, false);
                    break;
                }
            }
        }
        return;
    }
    if (isInfoCursorForModificationEnabled()) {
        // some copies are drawn: say why the others are not
        for (const LC_OffsetSourceOutcome& source : outcome.sources) {
            if (!source.complete()) {
                appendInfoCursorZoneMessage(source.succeeded() ? stopReason(source) : failureReason(source, true), 2,
                                            false);
                break;
            }
        }
    }
    if (static_cast<std::size_t>(ctx.entitiesToAdd.size()) <= maxPreviewDetail()) {
        if (ctx.setActivePen && m_document != nullptr) {
            // the pen the commit will give them
            const RS_Pen pen = m_document->getActivePen();
            for (RS_Entity* e : std::as_const(ctx.entitiesToAdd)) {
                e->setPen(pen);
            }
        }
        previewEntitiesToAdd(ctx); // the preview adopts each entity once
        return;
    }
    // More output than the preview may draw in detail: draw its bounding box
    // instead, from entities the preview never adopts. The committed offset is
    // the same either way.
    RS_Vector lo{false};
    RS_Vector hi{false};
    for (RS_Entity* e : std::as_const(ctx.entitiesToAdd)) {
        e->calculateBorders();
        const RS_Vector a = e->getMin();
        const RS_Vector b = e->getMax();
        if (a.valid && b.valid && std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(b.x) &&
            std::isfinite(b.y)) {
            lo = lo.valid ? RS_Vector::minimum(lo, a) : a;
            hi = hi.valid ? RS_Vector::maximum(hi, b) : b;
        }
    }
    qDeleteAll(ctx.entitiesToAdd);
    ctx.entitiesToAdd.clear();
    if (lo.valid && hi.valid) {
        previewRefLine({lo.x, lo.y}, {hi.x, lo.y});
        previewRefLine({hi.x, lo.y}, {hi.x, hi.y});
        previewRefLine({hi.x, hi.y}, {lo.x, hi.y});
        previewRefLine({lo.x, hi.y}, {lo.x, lo.y});
    }
}

void LC_ActionModifyOffset::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e) {
    if (!currentContext() || (m_work && !m_work->preview)) return;
    const RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetReferencePoint:{
            m_offsetData->coord = getRelZeroAwarePoint(e, mouse);
            m_offsetData->sideFallback = RS_Vector(false);
            previewOffset();
            break;
        }
        case SetPosition:{
            m_offsetData->coord = m_referencePoint;
            m_offsetData->sideFallback = mouse; // for a reference point on the curve
            const RS_Vector offset = mouse - m_referencePoint;
            if (!m_distanceIsFixed){
                m_offsetData->distance = offset.magnitude();
            }
            previewOffset();
            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(m_referencePoint);
                previewRefSelectablePoint(mouse);
                previewRefLine(m_referencePoint, mouse);
            }

            if (isInfoCursorForModificationEnabled()){
                msg(tr("Offset"))
                    .linear(tr("Distance:"), m_offsetData->distance)
                    .toInfoCursorZone2(false);
            }
            break;
        }
        default:
            break;
    }
}

bool LC_ActionModifyOffset::doUpdateDistanceByInteractiveInput(const QString& tag, const double distance) {
    if (tag == "distance") {
        setDistance(distance);
        return true;
    }
    return false;
}

bool LC_ActionModifyOffset::doProcessCommand(const int status, const QString& command) {
    bool accept = false;
    switch (status) {
        // a typed distance fixes it, as clicking the field in the options widget
        // would: it stops the second click of the reference-point flow (or the
        // mouse's own distance from the reference point) from overriding it
        case SetReferencePoint:
        case SetPosition: {
            bool ok = false;
            const double distance = RS_Math::eval(command, &ok);
            if (ok && distance > RS_TOLERANCE) {
                accept = true;
                setDistance(distance);
                setDistanceFixed(true);
            }
            else {
                commandMessage(tr("Not a valid expression"));
            }
            updateOptions();
            break;
        }
        default:
            break;
    }
    return accept;
}

void LC_ActionModifyOffset::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
    if (!currentContext() || (m_work && !m_work->preview)) return;
    switch (status){
        case SetReferencePoint:{
            m_referencePoint = getRelZeroAwarePoint(e, e->snapPoint);
            m_offsetData->coord = m_referencePoint;
            m_offsetData->sideFallback = RS_Vector(false);
            if (!m_distanceIsFixed){
                addSnappedPointToVisualSnap(m_referencePoint);
                moveRelativeZero(m_referencePoint);
            }
            if (m_distanceIsFixed){
                trigger();
            }
            else{
              setStatus(SetPosition);
            }
            break;
        }
        case SetPosition:{
           m_offsetData->sideFallback = e->snapPoint; // where the last move put it, unless there was none
           trigger();
           break;
        }
        default:
            break;
    }
}

LC_ActionOptionsWidget* LC_ActionModifyOffset::createOptionsWidget() {
    return new LC_OffsetOptionsWidget();
}

LC_ActionOptionsPropertiesFiller* LC_ActionModifyOffset::createOptionsFiller() {
    return new LC_OffsetOptionsFiller();
}

double LC_ActionModifyOffset::getDistance() const {
    return m_offsetData->distance;
}

void LC_ActionModifyOffset::setDistance(const double distance) const {
    m_offsetData->distance = distance;
}

void LC_ActionModifyOffset::setDistanceFixed(const bool value) {
    m_distanceIsFixed = value;
    if (!value){
        if (getStatus() == SetPosition){
            setStatus(SetReferencePoint);
        }
    }
}

bool LC_ActionModifyOffset::isAllowTriggerOnEmptySelection() {
    return false;
}

void LC_ActionModifyOffset::onMouseRightButtonReleaseSelected(const int status, [[maybe_unused]] const LC_MouseEvent* event) {
    cancelPreparation();
    if (!currentContext()) return;
    deletePreview();
    if (status == SetReferencePoint){
        if (m_selectionComplete) {
            m_selectionComplete = false;
        }
        else{
            initPrevious(status);
        }
    }
    else{
        initPrevious(status);
    }
}

void LC_ActionModifyOffset::updateActionPromptForSelected(const int status) {
    switch (status) {
        case SetReferencePoint:
            if (m_distanceIsFixed){
                updatePromptTRBack(tr("Specify direction of offset, or enter distance <%1>").arg(getDistance()));
            }
            else {
                updatePromptTRBack(
                    tr("Specify reference point for direction of offset, or enter distance <%1>").arg(getDistance()));
            }
            break;
        case SetPosition:
            updatePromptTRBack(tr("Specify direction of offset, or enter distance <%1>").arg(getDistance()));
            break;
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionModifyOffset::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select entities to offset") + getSelectionCompletionHintMsg(),
                         MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Offset immediately after selection")));
}

LC_ModifyOperationFlags* LC_ActionModifyOffset::getModifyOperationFlags() {
    return m_offsetData.get();
}
