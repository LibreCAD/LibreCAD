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

#ifndef LC_ACTIONSPLINEFROMPOLYLINE_H
#define LC_ACTIONSPLINEFROMPOLYLINE_H

#include <QObject>
#include "rs_entity.h"
#include "rs_previewactioninterface.h"

class LC_ActionSplineFromPolyline :public RS_PreviewActionInterface{
    Q_OBJECT

public:
    LC_ActionSplineFromPolyline(RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionSplineFromPolyline() override = default;
    bool isUseCurrentAttributes() {return useCurrentAttributes;};
    void setUseCurrentAttributes(bool b) {useCurrentAttributes  = b;};
    bool isUseCurrentLayer() {return useCurrentLayer;};
    void setUseCurrentLayer(bool b) {useCurrentLayer = b;}
    bool isKeepOriginals() {return keepOriginals;};
    void setKeepOriginals(bool b) {keepOriginals = b;}
    void setSplineDegree(int degree){splineDegree = degree;};
    int getSplineDegree(){return splineDegree;}
    int getSegmentPoints(){return segmentMiddlePoints;}
    void setSegmentPoints(int val){segmentMiddlePoints = val;}
    void setUseFitPoints(bool val){vertexesAreFitPoints = val;}
    bool isUseFitPoints(){return vertexesAreFitPoints;}
    void finish(bool updateTB) override;
    void mouseMoveEvent(QMouseEvent *event) override;;
protected:
    enum State {
        SetEntity
    };

    RS_Polyline *entityToModify = nullptr;

    bool keepOriginals {false};
    bool useCurrentLayer {false};
    bool useCurrentAttributes {false};
    bool vertexesAreFitPoints {false};
    int splineDegree = 3;
    int segmentMiddlePoints = 1;

    RS_Entity* createSplineForPolyline(RS_Entity *p);
    void fillControlPointsListFromPolyline(const RS_Polyline *polyline, std::vector<RS_Vector> &controlPoints) const;
    void setupAndAddCreatedEntity(RS_Entity *createdEntity, RS_Layer *layerToSet, const RS_Pen &penToUse);
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void doTrigger() override;
};

#endif // LC_ACTIONSPLINEFROMPOLYLINE_H
