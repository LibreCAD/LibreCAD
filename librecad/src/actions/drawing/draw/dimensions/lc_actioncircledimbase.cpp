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

#include "rs_math.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_preview.h"
#include "lc_actioncircledimbase.h"
#include "qg_dimoptions.h"

LC_ActionCircleDimBase::LC_ActionCircleDimBase(const char* name, RS_EntityContainer &container, RS_GraphicView &graphicView, RS2::ActionType type)
  : RS_ActionDimension(name, container,  graphicView)
    , entity(nullptr)
    , lastStatus(SetEntity)
    , pos(std::make_unique<RS_Vector>()){
    actionType = type;
}

LC_ActionCircleDimBase::~LC_ActionCircleDimBase() = default;

void LC_ActionCircleDimBase::trigger() {
    RS_ActionDimension::trigger();

    if (entity != nullptr) {
        preparePreview(entity, *pos, alternateAngle);
        auto *newEntity = createDim(container);
        newEntity->setLayerToActive();
        newEntity->setPenToActive();
        newEntity->update();
        container->addEntity(newEntity);

        addToDocumentUndoable(newEntity);

        graphicView->redraw(RS2::RedrawDrawing);
        alternateAngle = false;
        RS_Snapper::finish();

    } else {
        RS_DEBUG->print("RS_ActionDimDiametric::trigger: Entity is nullptr\n");
    }
}

void LC_ActionCircleDimBase::mouseMoveEvent(QMouseEvent *e) {
    RS_DEBUG->print("LC_ActionCircleDimBase::mouseMoveEvent begin");
    RS_Vector snap = snapPoint(e);
    deleteHighlights();
    deletePreview();
    switch (getStatus()) {
        case SetEntity: {
            RS_Entity *en = catchEntity(e, RS2::ResolveAll);
            if (en != nullptr) {
                if (isArc(en) || isCircle(en)) {
                    highlightHover(en);
                    moveRelativeZero(en->getCenter());
                    if (previewShowsFullDimension) {
                        RS_Vector pointOnCircle = preparePreview(en, snap, isControl(e));
                        auto *d = createDim(preview.get());
                        d->update();
                        previewEntity(d);
                        previewRefSelectablePoint(pointOnCircle);
                    }
                }
            }
            break;
        }
        case SetPos: {
            if (entity != nullptr) {
                highlightSelected(entity);
                *pos = getSnapAngleAwarePoint(e, entity->getCenter(), snap, true);
                RS_Vector pointOnCircle = preparePreview(entity, *pos, false);

                auto *d = createDim(preview.get());
                currentAngle = entity->getCenter().angleTo(pointOnCircle);
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
    drawPreview();
    drawHighlights();
    RS_DEBUG->print("RS_ActionDimRadial::mouseMoveEvent end");
}

void LC_ActionCircleDimBase::onMouseLeftButtonRelease(int status, QMouseEvent *e) {
    switch (status) {
        case SetEntity: {
            RS_Entity *en = catchEntity(e, RS2::ResolveAll);
            if (en != nullptr) {
                if (isArc(en) || isCircle(en)) {
                    entity = en;
                    const RS_Vector &center = en->getCenter();
                    moveRelativeZero(center);
                    if (!isAngleIsFree()){
                        alternateAngle = isControl(e);
                        if (!pos->valid){
                            *pos = snapPoint(e);
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
            RS_Vector snap = snapPoint(e);
            snap = getSnapAngleAwarePoint(e, entity->getCenter(), snap);
            fireCoordinateEvent(snap);
            break;
        }
        default:
            break;
    }
}

void LC_ActionCircleDimBase::onMouseRightButtonRelease(int status, [[maybe_unused]] QMouseEvent *e) {
    deletePreview();
    initPrevious(status);
}

void LC_ActionCircleDimBase::onCoordinateEvent(int status, [[maybe_unused]] bool isZero, const RS_Vector &coord) {
    switch (status) {
        case SetPos: {
            *pos = coord;
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
        graphicView->enableCoordinateInput();
        setStatus(lastStatus);
        accept = true;
    } else if (checkCommand("text", c)) { // command: text
        lastStatus = (Status) status;
        graphicView->disableCoordinateInput();
        setStatus(SetText);
        accept = true;
    } else if (status == SetPos) {// setting angle
        bool ok;
        double a = RS_Math::eval(c, &ok);
        if (ok) {
            accept = true;
            currentAngle = RS_Math::deg2rad(a);
            pos->setPolar(1.0, currentAngle);
            *pos += data->definitionPoint;
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
            updateMouseWidgetTRCancel(tr("Select arc or circle entity"), angleIsFree ? MOD_NONE : MOD_CTRL(tr("Free angle")));
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

double LC_ActionCircleDimBase::getAngle() const {
    return angle;
}

void LC_ActionCircleDimBase::setAngle(double angle) {
    this->angle = angle;
}

bool LC_ActionCircleDimBase::isAngleIsFree() const {
    return angleIsFree;
}

void LC_ActionCircleDimBase::setAngleIsFree(bool angleIsFree) {
    this->angleIsFree = angleIsFree;
}
