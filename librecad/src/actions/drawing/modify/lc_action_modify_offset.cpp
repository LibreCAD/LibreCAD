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
#include <unordered_set>

#include <QStringList>

#include "lc_actioninfomessagebuilder.h"
#include "lc_curveoffset.h"
#include "lc_offset_options_filler.h"
#include "lc_offset_options_widget.h"
#include "rs_document.h"
#include "rs_modification.h"
#include "rs_settings.h"

LC_ActionModifyOffset::LC_ActionModifyOffset(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("ActionModifyOffset", actionContext,RS2::ActionModifyOffset,
                         {RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse, RS2::EntityLine, RS2::EntityPolyline,
                          RS2::EntitySpline, RS2::EntitySplinePoints})
    , m_offsetData(new RS_OffsetData())
    , m_previewCache(std::make_unique<LC_OffsetPreviewCache>()){

    m_offsetData->distance = 0.;
    m_offsetData->number = 1;
    m_offsetData->useCurrentAttributes = true;
    m_offsetData->useCurrentLayer = true;
}

// fixme - support remove originals mode
// fixme - number of copies support
// fixme - support attributes support
// todo - basically, it seems that this action should be re-thought in general. There are several limitations (say,
// todo - some entities like splines do not support offset.
// todo - also, it seems that it's related to parallel/equidistant polyline actions...
// todo - so probably either this action should be reworked, or existing actions should be extended to support
// todo - selection and better offset operations...

LC_ActionModifyOffset::~LC_ActionModifyOffset() = default;

void LC_ActionModifyOffset::doSaveOptions() {
    save("Distance", getDistance());
    save("DistanceFixed",isFixedDistance());

    save("UseCurrentLayer", isUseCurrentLayer());
    save("UseCurrentAttributes", isUseCurrentAttributes());
    save("KeepOriginals", isKeepOriginals());
    save("MultipleCopies", isUseMultipleCopies());
    save("Copies", getCopiesNumber());
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

    const bool keepOriginals = loadBool("KeepOriginals", false);
    setKeepOriginals(keepOriginals);

    const bool multiCopy  = loadBool("MultipleCopies", false);
    setUseMultipleCopies(multiCopy);

    const int copies = loadInt("Copies", 1);
    setCopiesNumber(copies);
}

bool LC_ActionModifyOffset::isInVisualSnapStatus(const int status) {
    return (status == SetReferencePoint) || (status == SetPosition);
}

bool LC_ActionModifyOffset::doTriggerModifications(LC_DocumentModificationBatch& ctx) {
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
    switch (source.engineStatus) {
        case LC_CurveOffsetStatus::AmbiguousSide:
            return tr("the point is on the curve, so it gives no side");
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
            return tr("the offset could not be made");
    }
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
    QList<RS_Entity*> toSelect;
    for (const LC_OffsetSourceOutcome& source : std::as_const(m_pendingOutcome->sources)) {
        if (source.succeeded()) {
            offsetSources.insert(source.source);
            toSelect.append(source.createdEntities);
        }
    }
    QList<RS_Entity*> toUnselect;
    for (RS_Entity* selected : std::as_const(m_selectedEntities)) {
        if (offsetSources.count(selected) == 0) {
            toSelect.append(selected); // not offset: stays selected
        }
        else if (m_offsetData->keepOriginals) {
            toUnselect.append(selected);
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
    int failed = 0;
    int total = 0;
    QStringList reasons;
    // sources that got fewer copies than asked for, and the copies of the last of them
    int shortOfCopies = 0;
    int made = 0;
    int requested = 0;
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
                ++shortOfCopies;
                made = source.copiesMade;
                requested = source.copiesRequested;
            }
        }
    }
    m_pendingOutcome.reset();
    if (failed > 0) {
        commandMessage(tr("%1 of %2 selected entities could not be offset: %3")
                           .arg(failed)
                           .arg(total)
                           .arg(reasons.join(QStringLiteral("; "))));
    }
    if (shortOfCopies == 1) {
        commandMessage(tr("Only %1 of %2 copies fit, since nothing is left at a larger distance; the original was kept")
                           .arg(made)
                           .arg(requested));
    }
    else if (shortOfCopies > 1) {
        commandMessage(tr("%1 selected entities got fewer than %2 copies, since nothing is left at a larger distance; "
                          "their originals were kept")
                           .arg(shortOfCopies)
                           .arg(requested));
    }
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
    m_pendingOutcome.reset();
    m_previewCache->clear();
    m_previewSources.clear();
    LC_ActionModifyBase::finish();
}

std::size_t LC_ActionModifyOffset::maxPreviewDetail() {
    const int configured = LC_GET_ONE_INT("Appearance", "MaxPreview", 100);
    return configured > 0 ? static_cast<std::size_t>(configured) : 0;
}

void LC_ActionModifyOffset::previewOffset() {
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

void LC_ActionModifyOffset::onMouseLeftButtonReleaseSelected(const int status, const LC_MouseEvent* e) {
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
                updatePromptTRBack(tr("Specify direction of offset"));
            }
            else {
                updatePromptTRBack(tr("Specify reference point for direction of offset"));
            }
            break;
        case SetPosition:
            updatePromptTRBack(tr("Specify direction of offset"));
            break;
        default:
            updatePrompt();
            break;
    }
}

void LC_ActionModifyOffset::updateActionPromptForSelection() {
    updatePromptTRCancel(tr("Select line, polyline, ellipse, circle, arc, spline or spline through points to create offset") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Offset immediately after selection")));
}

LC_ModifyOperationFlags* LC_ActionModifyOffset::getModifyOperationFlags() {
    return m_offsetData.get();
}
