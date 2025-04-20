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

#include "lc_actionsplinemodifybase.h"

class RS_Pen;
class RS_Layer;

class LC_ActionSplineExplode:public LC_ActionSplineModifyBase{
Q_OBJECT
public:
    LC_ActionSplineExplode(LC_ActionContext *actionContext);
    ~LC_ActionSplineExplode() override = default;
    int getSegmentsCountFromDrawing();
    bool isUseCurrentAttributes() {return m_useCurrentAttributes;};
    void setUseCurrentAttributes(bool b) {m_useCurrentAttributes  = b;};
    bool isUseCurrentLayer() {return m_useCurrentLayer;};
    void setUseCurrentLayer(bool b) {m_useCurrentLayer = b;}
    bool isKeepOriginals() {return m_keepOriginals;};
    void setKeepOriginals(bool b) {m_keepOriginals = b;};
    bool isToPolyline() {return m_createPolyline;};
    void setUsePolyline(bool b) {m_createPolyline = b;};
    int getCustomSegmentsCount() {return m_customSegmentsCount;};
    void setSegmentsCountValue(int i) {m_customSegmentsCount = i;};
    bool isUseCustomSegmentsCount() {return m_useCustomSegmentsCount;};
    void setUseCustomSegmentsCount(bool b) {m_useCustomSegmentsCount = b;}
protected:
    bool m_createPolyline {false};
    bool m_keepOriginals {false};
    bool m_useCurrentLayer {false};
    bool m_useCurrentAttributes {false};
    bool m_useCustomSegmentsCount {false};
    int m_customSegmentsCount = 8;

    RS_Entity *createPolylineByVertexes(const std::vector<RS_Vector> &strokePoints, bool closed) const;
    int obtainSegmentsCount();
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMove(RS_Vector mouse, int status, LC_MouseEvent *e) override;
    void fillStrokePoints(RS_Entity *e, int segmentsCount, std::vector<RS_Vector> &strokePoints, bool &closed) const;
    void setupAndAddCreatedEntity(RS_Entity *createdEntity, RS_Layer *layerToSet, const RS_Pen &penToUse);
    void updateMouseButtonHints() override;
    RS_Entity *createModifiedSplineEntity(RS_Entity *e, RS_Vector controlPoint, bool startDirection) override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void doTrigger() override;
};

#endif // LC_ACTIONSPLINEEXPLODE_H
