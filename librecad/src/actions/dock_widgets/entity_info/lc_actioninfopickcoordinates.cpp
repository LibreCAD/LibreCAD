/****************************************************************************
*
* Actions that collects coordinates by mouse click on drawing using current
* snap mode
*
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
#include <QMouseEvent>
#include "qc_applicationwindow.h"
#include "lc_actioninfopickcoordinates.h"
#include "lc_quickinfowidget.h"

LC_ActionInfoPickCoordinates::LC_ActionInfoPickCoordinates(RS_EntityContainer &container, RS_GraphicView &graphicView)
    :LC_AbstractActionWithPreview("Pick Coordinates", container, graphicView){
   actionType = RS2::ActionInfoPickCoordinates;
}

void LC_ActionInfoPickCoordinates::init(int status){
    LC_AbstractActionWithPreview::init(status);
    if (status == 0){
        // init points and settings
        updateCollectedPointsByWidget();
    }
}

void LC_ActionInfoPickCoordinates::resume(){
    RS_PreviewActionInterface::resume();
    updateCollectedPointsByWidget();
}

/**
 * retrieve coordinates from the list stored in action and copies them to own list
 */
void LC_ActionInfoPickCoordinates::updateCollectedPointsByWidget(){
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        int size = entityInfoWidget->getCollectedCoordinatesCount();
        points.clear();
        for (int i = 0; i < size; i++) {
            RS_Vector p = entityInfoWidget->getCollectedCoordinate(i);
            points << p;
        }
        entityInfoWidget->setWidgetMode(LC_QuickInfoWidget::MODE_COORDINATE_COLLECTING);
        entityInfoWidget->updateCollectedPointsView();

        drawPointsPath = entityInfoWidget->isDisplayPointsPathOnPreview();
    }
}

void LC_ActionInfoPickCoordinates::doFinish(bool updateTB){
    LC_AbstractActionWithPreview::doFinish(updateTB);
    points.clear();

    // notify widget that collection points is completed
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        entityInfoWidget->endAddingCoordinates();
    }
}

void LC_ActionInfoPickCoordinates::doOnLeftMouseButtonRelease([[maybe_unused]]QMouseEvent *e, [[maybe_unused]]int status, const RS_Vector &snapPoint){
    // add point
    points << snapPoint;
    updateQuickInfoWidget(snapPoint);
    drawPreviewForLastPoint();
}

RS_Vector LC_ActionInfoPickCoordinates::doGetMouseSnapPoint(QMouseEvent *e){
    bool freeSnap = isShift(e);
    if (freeSnap){ // let free point snap if shift pressed
        return toGraph(e);
    }
    else{
        return snapPoint(e);
    }
}

void LC_ActionInfoPickCoordinates::doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, [[maybe_unused]]int status){
    // preview for this point
    // todo - review - should we display normal or reference point?
    createPoint(snap, list);
    // preview for previously collected points
    int size = points.size();
    for (int i = 0; i < size ;i++){
        RS_Vector point = points.at(i);
        createPoint(point, list);
        if (drawPointsPath){ //optional points path
            if (i > 0){
                RS_Vector prevPoint = points.at(i - 1);
                createLine(prevPoint, point, list);
            }
        }
    }
}

void LC_ActionInfoPickCoordinates::updateQuickInfoWidget(const RS_Vector& coord){
    LC_QuickInfoWidget *entityInfoWidget = QC_ApplicationWindow::getAppWindow()->getEntityInfoWidget();
    if (entityInfoWidget != nullptr){
        entityInfoWidget->processCoordinate(coord);
    }
}

void LC_ActionInfoPickCoordinates::updateMouseButtonHints(){
    updateMouseWidgetTRCancel(tr("Select point"), MOD_SHIFT_FREE_SNAP);
}
