/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/
#include "lc_actioncircledimbase.h"

#include "qg_dimoptions.h"
#include "rs_debug.h"
#include "rs_dimension.h"
#include "rs_preview.h"

LC_ActionCircleDimBase::LC_ActionCircleDimBase(const char* name, LC_ActionContext *actionContext, RS2::ActionType actionType)
  : RS_ActionDimension(name, actionContext, actionType)
    , m_entity(nullptr)
    , m_lastStatus(SetEntity)
    , m_position(std::make_unique<RS_Vector>()){
}

LC_ActionCircleDimBase::~LC_ActionCircleDimBase() = default;

void LC_ActionCircleDimBase::doTrigger() {
    if (m_entity != nullptr) {
        preparePreview(m_entity, *m_position, m_alternateAngle);
        auto *newEntity = createDim(m_container);

        setPenAndLayerToActive(newEntity);
        newEntity->update();
        undoCycleAdd(newEntity);
        m_alternateAngle = false;
        RS_Snapper::finish();
    } else {
        RS_DEBUG->print("RS_ActionDimDiametric::trigger: Entity is nullptr\n");
    }
}

void LC_ActionCircleDimBase::onMouseMoveEvent(int status, LC_MouseEvent *e) {
    RS_Vector snap = e->snapPoint;
    switch (status) {
        case SetEntity: {
            RS_Entity *en = catchEntityByEvent(e, RS2::ResolveAll);
            if (en != nullptr) {
                if (isArc(en) || isCircle(en)) {
                    highlightHover(en);
                    moveRelativeZero(en->getCenter());
                    if (m_previewShowsFullDimension) {
                        RS_Vector pointOnCircle = preparePreview(en, snap, e->isControl);
                        auto *d = createDim(m_preview.get());
                        d->update();
                        previewEntity(d);
                        previewRefSelectablePoint(pointOnCircle);
                    }
                }
            }
            break;
        }
        case SetPos: {
            if (m_entity != nullptr) {
                highlightSelected(m_entity);
                *m_position = getSnapAngleAwarePoint(e, m_entity->getCenter(), snap, true);
                RS_Vector pointOnCircle = preparePreview(m_entity, *m_position, false);

                auto *d = createDim(m_preview.get());
                m_currentAngle = m_entity->getCenter().angleTo(pointOnCircle);
                m_ucsBasisAngleDegrees = toUCSBasisAngleDegrees(m_currentAngle);
                updateOptionsUI(QG_DimOptions::UI_UPDATE_CIRCLE_ANGLE);
                d->update();
                previewEntity(d);
                previewRefSelectablePoint(pointOnCircle);
            }
            break;
        }
        default:
            break;
    }
}

void LC_ActionCircleDimBase::onMouseLeftButtonRelease(int status, LC_MouseEvent *e) {
    switch (status) {
        case SetEntity: {
            RS_Entity *en = catchEntityByEvent(e, RS2::ResolveAll);
            if (en != nullptr) {
                if (isArc(en) || isCircle(en)) {
                    m_entity = en;
                    const RS_Vector &center = en->getCenter();
                    moveRelativeZero(center);
                    if (!isAngleIsFree()){
                        m_alternateAngle = e->isControl;
                        if (!m_position->valid){
                            *m_position = e->snapPoint;
                        }
                        trigger();
                        reset();
                    }
                    else {
                        setStatus(SetPos);
                    }
                } else {
                    commandMessage(tr("Not a circle or arc entity"));
                }
            }
            break;
        }
        case SetPos: {
            RS_Vector snap = e->snapPoint;
            snap = getSnapAngleAwarePoint(e, m_entity->getCenter(), snap);
            fireCoordinateEvent(snap);
            break;
        }
        default:
            break;
    }
}

void LC_ActionCircleDimBase::onMouseRightButtonRelease(int status, [[maybe_unused]] LC_MouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionCircleDimBase::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &coord) {
    switch (status) {
        case SetPos: {
            *m_position = coord;
            trigger();
            reset();
            setStatus(SetEntity);
            break;
        }
        default:
            break;
    }
}

bool LC_ActionCircleDimBase::doProcessCommand(int status, const QString &c) {
    // fixme - check whether the code is duplicated with other dim actions
    bool accept = false;
    // setting new text label:
    if (status == SetText) {
        setText(c);
        updateOptions();
        enableCoordinateInput();
        setStatus(m_lastStatus);
        accept = true;
    } else if (checkCommand("text", c)) { // command: text
        m_lastStatus = (Status) status;
        disableCoordinateInput();
        setStatus(SetText);
        accept = true;
    } else if (status == SetPos) {// setting angle
        double angle;
        bool ok = parseToUCSBasisAngle(c, angle);
        if (ok) {
            accept = true;
            m_ucsBasisAngleDegrees = angle;
            m_currentAngle = toWorldAngleFromUCSBasisDegrees(angle);
            m_position->setPolar(1.0, m_currentAngle);
            *m_position += m_dimensionData->definitionPoint;
            updateOptionsUI(QG_DimOptions::UI_UPDATE_CIRCLE_ANGLE);
            trigger();
            reset();
            setStatus(SetEntity);
        } else {
            commandMessage(tr("Not a valid expression"));
        }
    }
    return accept;
}

QStringList LC_ActionCircleDimBase::getAvailableCommands() {
    QStringList cmd;

    switch (getStatus()) {
        case SetEntity:
        case SetPos:
            cmd += command("text");
            break;
        default:
            break;
    }
    return cmd;
}

void LC_ActionCircleDimBase::updateMouseButtonHints() {
    switch (getStatus()) {
        case SetEntity:
            updateMouseWidgetTRCancel(tr("Select arc or circle entity"), m_angleIsFree ? MOD_NONE : MOD_CTRL(tr("Free angle")));
            break;
        case SetPos:
            updateMouseWidgetTRCancel(tr("Specify dimension line position or enter angle:"), MOD_SHIFT_ANGLE_SNAP);
            break;
        case SetText:
            updateMouseWidget(tr("Enter dimension text:"));
            break;
        default:
            updateMouseWidget();
            break;
    }
}

double LC_ActionCircleDimBase::getUcsAngleDegrees() const {
    return m_ucsBasisAngleDegrees;
}

void LC_ActionCircleDimBase::setUcsAngleDegrees(double ucsRelAngleDegrees) {
    m_ucsBasisAngleDegrees = ucsRelAngleDegrees;
    m_currentAngle = toWorldAngleFromUCSBasisDegrees(ucsRelAngleDegrees);
}

bool LC_ActionCircleDimBase::isAngleIsFree() const {
    return m_angleIsFree;
}

void LC_ActionCircleDimBase::setAngleIsFree(bool angleIsFree) {
    this->m_angleIsFree = angleIsFree;
}

double LC_ActionCircleDimBase::getCurrentAngle() {
    double angleDeg = toUCSBasisAngleDegrees(m_currentAngle);
    return angleDeg;
}
