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

#include "rs_actionmodifyoffset.h"
#include "rs_dialogfactory.h"
#include "rs_graphicview.h"
#include "rs_modification.h"
#include "rs_preview.h"
#include "qg_modifyoffsetoptions.h"
#include "rs_debug.h"

RS_ActionModifyOffset::RS_ActionModifyOffset(LC_ActionContext *actionContext)
    :LC_ActionModifyBase("Modify Offset", actionContext,RS2::ActionModifyOffset,
                         {RS2::EntityArc, RS2::EntityCircle, RS2::EntityLine, RS2::EntityPolyline},
                         true)
    , data(new RS_OffsetData()){

    data->distance = 0.;
    data->number = 1;
    data->useCurrentAttributes = true;
    data->useCurrentLayer = true;
}

// fixme - support remove originals mode
// fixme - number of copies support
// fixme - support attributes support
// todo - basically, it seems that this action should be re-thought in general. There are several limitations (say,
// todo - some entities like ellipse or splines do not support offset.
// todo - also, it seems that it's related to parallel/equidistant polyline actions...
// todo - so probably either this action should be reworked, or existing actions should be extended to support
// todo - selection and better offset operations...

RS_ActionModifyOffset::~RS_ActionModifyOffset() = default;

void RS_ActionModifyOffset::doTrigger(bool keepSelected) {
    RS_Modification m(*m_container, m_viewport);
    m.offset(*data, selectedEntities, false, keepSelected);
    finish(false);
}

void RS_ActionModifyOffset::onMouseMoveEventSelected(int status, LC_MouseEvent *e) {
    RS_Vector mouse = e->snapPoint;
    switch (status){
        case SetReferencePoint:{
            data->coord = getRelZeroAwarePoint(e, mouse);
            RS_Modification m(*m_preview, m_viewport, false);
            m.offset(*data, selectedEntities, true, false);
            break;
        }
        case SetPosition:{
            data->coord = referencePoint;
            RS_Vector offset = mouse - referencePoint;
            if (!distanceIsFixed){
                data->distance = offset.magnitude();
            }
            RS_Modification m(*m_preview, m_viewport, false);
            m.offset(*data, selectedEntities, true, false);

            if (m_showRefEntitiesOnPreview) {
                previewRefPoint(referencePoint);
                previewRefSelectablePoint(mouse);
                previewRefLine(referencePoint, mouse);
            }

            if (isInfoCursorForModificationEnabled()){
                msg(tr("Offset"))
                    .linear(tr("Distance:"), data->distance)
                    .toInfoCursorZone2(false);
            }
            break;
        }
        default:
            break;
    }
}

void RS_ActionModifyOffset::mouseLeftButtonReleaseEventSelected(int status, LC_MouseEvent *e) {
    switch (status){
        case SetReferencePoint:{
            referencePoint = getRelZeroAwarePoint(e, e->snapPoint);
            data->coord = referencePoint;
            if (!distanceIsFixed){
                moveRelativeZero(referencePoint);
            }
            if (distanceIsFixed){
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

LC_ActionOptionsWidget* RS_ActionModifyOffset::createOptionsWidget() {
    return new QG_ModifyOffsetOptions();
}

double RS_ActionModifyOffset::getDistance() {
    return data->distance;
}

void RS_ActionModifyOffset::setDistance(double distance) {
    data->distance = distance;
}

void RS_ActionModifyOffset::setDistanceFixed(bool value) {
    distanceIsFixed = value;
    if (!value){
        if (getStatus() == SetPosition){
            setStatus(SetReferencePoint);
        }
    }
}

bool RS_ActionModifyOffset::isAllowTriggerOnEmptySelection() {
    return false;
}

void RS_ActionModifyOffset::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deletePreview();
    if (status == SetReferencePoint){
        if (selectionComplete) {
            selectionComplete = false;
        }
        else{
            initPrevious(status);
        }
    }
    else{
        initPrevious(status);
    }
}

void RS_ActionModifyOffset::updateMouseButtonHintsForSelected(int status) {
    switch (status) {
        case SetReferencePoint:
            if (distanceIsFixed){
                updateMouseWidgetTRBack(tr("Specify direction of offset"));
            }
            else {
                updateMouseWidgetTRBack(tr("Specify reference point for direction of offset"));
            }
            break;
        case SetPosition:
            updateMouseWidgetTRBack(tr("Specify direction of offset"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

void RS_ActionModifyOffset::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select line, polyline, circle or arc to create offset (Enter to complete)"), MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Offset immediately after selection")));
}

LC_ModifyOperationFlags* RS_ActionModifyOffset::getModifyOperationFlags() {
    return data.get();
}
