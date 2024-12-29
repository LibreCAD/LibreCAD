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

#ifndef LC_ACTIONSPLINEEXPLODE_H
#define LC_ACTIONSPLINEEXPLODE_H

#include <QObject>
#include "lc_actionsplinemodifybase.h"


class LC_ActionSplineExplode:public LC_ActionSplineModifyBase{
Q_OBJECT
public:
    LC_ActionSplineExplode(RS_EntityContainer &container, RS_GraphicView &graphicView);
    ~LC_ActionSplineExplode() override = default;

    int getSegmentsCountFromDrawing();
    bool isUseCurrentAttributes() {return useCurrentAttributes;};
    void setUseCurrentAttributes(bool b) {useCurrentAttributes  = b;};
    bool isUseCurrentLayer() {return useCurrentLayer;};
    void setUseCurrentLayer(bool b) {useCurrentLayer = b;}
    bool isKeepOriginals() {return keepOriginals;};
    void setKeepOriginals(bool b) {keepOriginals = b;};
    bool isToPolyline() {return createPolyline;};
    void setUsePolyline(bool b) {createPolyline = b;};
    int getCustomSegmentsCount() {return customSegmentsCount;};
    void setSegmentsCountValue(int i) {customSegmentsCount = i;};
    bool isUseCustomSegmentsCount() {return useCustomSegmentsCount;};
    void setUseCustomSegmentsCount(bool b) {useCustomSegmentsCount = b;}
protected:
    bool createPolyline {false};
    bool keepOriginals {false};
    bool useCurrentLayer {false};
    bool useCurrentAttributes {false};
    bool useCustomSegmentsCount {false};
    int customSegmentsCount = 8;

    RS_Entity *createPolylineByVertexes(const std::vector<RS_Vector> &strokePoints, bool closed) const;
    int obtainSegmentsCount();
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void fillStrokePoints(RS_Entity *e, int segmentsCount, std::vector<RS_Vector> &strokePoints, bool &closed) const;
    void setupAndAddCreatedEntity(RS_Entity *createdEntity, RS_Layer *layerToSet, const RS_Pen &penToUse);
    void updateMouseButtonHints() override;
    RS_Entity *createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool startDirection) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void onMouseMove(RS_Vector mouse, int status, QMouseEvent *e) override;
    void doTrigger() override;
};

#endif // LC_ACTIONSPLINEEXPLODE_H
