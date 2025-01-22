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
#include <QMouseEvent>

#include "lc_actionmodifyalign.h"
#include "lc_modifyalignoptions.h"
#include "rs_graphicview.h"
#include "rs_document.h"
#include "lc_align.h"

LC_ActionModifyAlign::LC_ActionModifyAlign(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :LC_ActionPreSelectionAwareBase("ModifyAlign", container, graphicView) {
    actionType = RS2::ActionModifyAlign;
}

void LC_ActionModifyAlign::init(int status) {
    showOptions();
    LC_ActionPreSelectionAwareBase::init(status);
}

void LC_ActionModifyAlign::selectionCompleted([[maybe_unused]]bool singleEntity, bool fromInit) {
    setSelectionComplete(isAllowTriggerOnEmptySelection(), fromInit);
    updateMouseButtonHints();
    updateSelectionWidget();
}

void LC_ActionModifyAlign::doTrigger([[maybe_unused]]bool keepSelected) {
    QList<RS_Entity *> entitiesToCreate;
    createAlignedEntities(entitiesToCreate, alignMin, alignMax, false);
    if (!entitiesToCreate.isEmpty()) {
        if (document) {
            undoCycleStart();

            for (auto e: entitiesToCreate) {
                // fixme - review!!
                // todo - this is a place to think about regarding usability and consistency. Other Modify actions does not clear "selected" status.
                // todo - however, from the usability point of view - if the user would like to continue aligning operation, not cleared selection is not convenient.
                e->setSelected(false);
                container->addEntity(e);
                undoableAdd(e);
            }

            for (auto e: selectedEntities) {
                undoableDeleteEntity(e);
            }

            undoCycleEnd();

            selectedEntities.clear();
            selectionComplete = false;
        }
    }
}

void LC_ActionModifyAlign::mouseMoveEventSelected(QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    deletePreview();
    deleteHighlights();
    RS_Vector min;
    RS_Vector max;
    bool showPreview = true;

    // defining boundaries
    switch (alignType) {
        case LC_Align::ENTITY: {
            RS2::ResolveLevel resolveLevel = isControl(e) ? RS2::ResolveAll : RS2::ResolveNone;
            RS_Entity *entity = catchEntity(snap, resolveLevel);
            if (entity != nullptr) {
                min = entity->getMin();
                max = entity->getMax();
                highlightHover(entity);
            } else {
                showPreview = false;
            }
            break;
        }
        case LC_Align::POSITION: {
            snap = getRelZeroAwarePoint(e, snap);
            min = snap;
            max = snap;
            break;
        }
        case LC_Align::DRAWING: {
            min = container->getMin();
            max = container->getMax();
            break;
        }
        default:
            break;
    }

    // reference points
    double verticalRef;
    bool drawVertical  = LC_Align::getVerticalRefCoordinate(min, max, hAlign, verticalRef);
    double horizontalRef;
    bool drawHorizontal = LC_Align::getHorizontalRefCoordinate(min, max, vAlign, horizontalRef);


    // preview entities
    if (showPreview) {
        QList<RS_Entity *> entitiesList;
        RS_Vector groupOffset = createAlignedEntities(entitiesList, min, max, true);
        for (auto ent: entitiesList) {
            previewEntity(ent);
        }
        if (showRefEntitiesOnPreview) {
            previewRefLines(drawVertical, verticalRef, drawHorizontal, horizontalRef);
        }

        // info cursor
        if (infoCursorOverlayPrefs->enabled){
            QString msg = tr("Align to ");
            switch (alignType) {
                case LC_Align::ENTITY:{
                    msg.append(tr("Entity"));
                    break;
                }
                case LC_Align::POSITION:{
                    msg.append(tr("Position"));
                    break;
                }
                case LC_Align::DRAWING:{
                    msg.append(tr("Drawing"));
                    break;
                }
            }
            LC_InfoMessageBuilder builder(msg);

            QString ref = tr("Reference: ");
            if (drawVertical){
                ref.append("X: ");
                ref.append(formatLinear(verticalRef));
                ref.append(" ");
            }

            if (drawHorizontal){
                ref.append("Y: ");
                ref.append(formatLinear(horizontalRef));
            }

            builder.add(ref);
            if (groupOffset.valid) {
                builder.add(tr("Offset:"));
                builder.add(formatRelative(groupOffset));
                builder.add(formatRelativePolar(groupOffset));
            }
            appendInfoCursorZoneMessage(builder.toString(), 2, false);
        }
    }

    drawHighlights();
    drawPreview();
}

void LC_ActionModifyAlign::previewRefLines(bool drawVertical, double verticalRef, bool drawHorizontal, double horizontalRef) {
    if (drawVertical) {
        double g0 = graphicView->toGraphY(0);
        double gHeight = graphicView->toGraphY(graphicView->getHeight());
        previewRefConstructionLine({verticalRef, g0}, {verticalRef, gHeight});
    }
    if (drawHorizontal) {
        double g0 = graphicView->toGraphX(0);
        double gWidth = graphicView->toGraphX(graphicView->getWidth());
        previewRefConstructionLine({g0, horizontalRef}, {gWidth, horizontalRef});
    }
}

void LC_ActionModifyAlign::mouseLeftButtonReleaseEventSelected([[maybe_unused]]int status, QMouseEvent *e) {
    RS_Vector snap = snapPoint(e);
    bool mayTrigger = true;
    switch (alignType) {
        case LC_Align::ENTITY: {
            RS2::ResolveLevel resolveLevel = isControl(e) ? RS2::ResolveAll : RS2::ResolveNone;
            RS_Entity *entity  = catchEntity(snap, resolveLevel);
            if (entity != nullptr) {
                alignMin = entity->getMin();
                alignMax = entity->getMax();
            } else {
                mayTrigger = false;
            }
            break;
        }
        case LC_Align::POSITION: {
            snap = getRelZeroAwarePoint(e, snap);
            alignMin = snap;
            alignMax = snap;
            break;
        }
        case LC_Align::DRAWING: {
            alignMin = container->getMin();
            alignMax = container->getMax();
            break;
        }
        default:
            break;
    }
    if (mayTrigger) {
        trigger();
    }
}

void LC_ActionModifyAlign::onCoordinateEvent([[maybe_unused]]int status, bool isZero, const RS_Vector &pos) {
    if (alignType == LC_Align::POSITION && !isZero) {
        alignMin = pos;
        alignMax = pos;
        trigger();
    }
    else{
        commandMessage(tr("Coordinate is accepted only for Align to \"Position\""));
    }
}

void LC_ActionModifyAlign::mouseRightButtonReleaseEventSelected(int status, [[maybe_unused]]QMouseEvent *pEvent) {
    deletePreview();
    if (selectionComplete) {
        selectionComplete = false;
    } else {
        initPrevious(status);
    }
}

void LC_ActionModifyAlign::updateMouseButtonHintsForSelected([[maybe_unused]]int status) {
    switch (alignType) {
        case LC_Align::ENTITY:
            updateMouseWidgetTRBack(tr("Select base alignment entity"), MOD_CTRL(tr("Select child entities of containers")));
            break;
        case LC_Align::POSITION:
            updateMouseWidgetTRBack(tr("Specify base alignment point"), MOD_SHIFT_RELATIVE_ZERO);
            break;
        case LC_Align::DRAWING:
            updateMouseWidgetTRBack(tr("Click or Enter to finish align"));
            break;
        default:
            updateMouseWidget();
    }
}

void LC_ActionModifyAlign::updateMouseButtonHintsForSelection() {
    updateMouseWidgetTRCancel(tr("Select entities to align (Enter to complete)"),  MOD_SHIFT_AND_CTRL(tr("Select contour"),tr("Select and align")));
}

RS2::CursorType LC_ActionModifyAlign::doGetMouseCursorSelected([[maybe_unused]]int status) {
    return RS2::CadCursor;
}

bool LC_ActionModifyAlign::isAllowTriggerOnEmptySelection() {
    return false;
}

void LC_ActionModifyAlign::setAlignType(int t) {
    alignType = t;
    updateMouseButtonHints();
}

LC_ActionOptionsWidget *LC_ActionModifyAlign::createOptionsWidget() {
    return new LC_ModifyAlignOptions();
}

RS_Vector LC_ActionModifyAlign::createAlignedEntities(QList<RS_Entity *> &list, RS_Vector min, RS_Vector max, bool previewOnly) {
    RS_Vector result =  RS_Vector(false);

    RS_Vector targetPoint = getReferencePoint(min, max);
    bool updateAttributes = !previewOnly;

    if (asGroup || selectedEntities.size() == 1) {
        RS_Vector selectionMin;
        RS_Vector selectionMax;

        LC_Align::collectSelectionBounds(selectedEntities, selectionMin, selectionMax);
        RS_Vector selectionRefPoint = getReferencePoint(selectionMin, selectionMax);
        RS_Vector offset = targetPoint - selectionRefPoint;

        result = offset;
        result.valid = true;

        for (auto e: selectedEntities) {
            RS_Entity* clone = LC_Align::createCloneMovedToOffset(e, offset, updateAttributes);
            list << clone;
        }
    } else {
        for (auto e: selectedEntities) {
            RS_Entity *clone = LC_Align::createCloneMovedToTarget(e, targetPoint, updateAttributes, hAlign, vAlign);
            list << clone;
        }
    }
    return result;
}

RS_Vector LC_ActionModifyAlign::getReferencePoint(const RS_Vector &min, const RS_Vector &max) {
    return LC_Align::getReferencePoint(min, max, hAlign, vAlign);
}
