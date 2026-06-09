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

#include "lc_actioninfomessagebuilder.h"
#include "lc_offset_options_filler.h"
#include "lc_offset_options_widget.h"
#include "rs_document.h"
#include "rs_modification.h"

LC_ActionModifyOffset::LC_ActionModifyOffset(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("ActionModifyOffset", actionContext,RS2::ActionModifyOffset,
                         {RS2::EntityArc, RS2::EntityCircle, RS2::EntityEllipse, RS2::EntityLine, RS2::EntityPolyline})
    , m_offsetData(new RS_OffsetData()){

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
    return RS_Modification::offset(*m_offsetData, m_selectedEntities, false, ctx);
}

void LC_ActionModifyOffset::doTriggerSelectionUpdate(const bool keepSelected, const LC_DocumentModificationBatch& ctx) {
    if (m_offsetData->keepOriginals) {
        unselect(m_selectedEntities);
    }
    if (keepSelected) {
        select(ctx.entitiesToAdd);
    }
}

void LC_ActionModifyOffset::doTriggerCompletion([[maybe_unused]]bool success) {
     finish();
    // fixme - sand - review whethe we can stay in entity's selection mode and don't finish there...
    // m_selectionComplete = false;
    // if (!m_distanceIsFixed){
    //     if (getStatus() == SetPosition){
    //         setStatus(SetReferencePoint);
    //     }
    // }
}

void LC_ActionModifyOffset::onMouseMoveEventSelected(const int status, const LC_MouseEvent* e) {
    const RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetReferencePoint:{
            m_offsetData->coord = getRelZeroAwarePoint(e, mouse);
            LC_DocumentModificationBatch ctx;
            RS_Modification::offset(*m_offsetData, m_selectedEntities, true, ctx);
            previewEntitiesToAdd(ctx);
            break;
        }
        case SetPosition:{
            m_offsetData->coord = m_referencePoint;
            const RS_Vector offset = mouse - m_referencePoint;
            if (!m_distanceIsFixed){
                m_offsetData->distance = offset.magnitude();
            }
            LC_DocumentModificationBatch ctx;
            RS_Modification::offset(*m_offsetData, m_selectedEntities, true, ctx);
            previewEntitiesToAdd(ctx);
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
    updatePromptTRCancel(tr("Select line, polyline, ellipse, circle or arc to create offset") + getSelectionCompletionHintMsg(),
                              MOD_SHIFT_AND_CTRL(tr("Select contour"), tr("Offset immediately after selection")));
}

LC_ModifyOperationFlags* LC_ActionModifyOffset::getModifyOperationFlags() {
    return m_offsetData.get();
}
