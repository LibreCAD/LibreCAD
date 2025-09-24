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

#include "rs_previewactioninterface.h"

class RS_Layer;
class RS_Pen;
class RS_Polyline;

class LC_ActionSplineFromPolyline :public RS_PreviewActionInterface{
    Q_OBJECT
public:
    explicit LC_ActionSplineFromPolyline(LC_ActionContext *actionContext);
    ~LC_ActionSplineFromPolyline() override = default;
    bool isUseCurrentAttributes() const {return m_useCurrentAttributes;};
    void setUseCurrentAttributes(bool b) {m_useCurrentAttributes  = b;};
    bool isUseCurrentLayer() const {return m_useCurrentLayer;};
    void setUseCurrentLayer(bool b) {m_useCurrentLayer = b;}
    bool isKeepOriginals() const {return m_keepOriginals;};
    void setKeepOriginals(bool b) {m_keepOriginals = b;}
    void setSplineDegree(int degree){m_splineDegree = degree;};
    int getSplineDegree() const {return m_splineDegree;}
    int getSegmentPoints() const {return m_segmentMiddlePoints;}
    void setSegmentPoints(int val){m_segmentMiddlePoints = val;}
    void setUseFitPoints(bool val){m_vertexesAreFitPoints = val;}
    bool isUseFitPoints() const {return m_vertexesAreFitPoints;}
    void finish(bool updateTB) override;
protected:
    enum State {
        SetEntity = InitialActionStatus
    };

    RS_Polyline *m_entityToModify = nullptr;
    bool m_keepOriginals {false};
    bool m_useCurrentLayer {false};
    bool m_useCurrentAttributes {false};
    bool m_vertexesAreFitPoints {false};
    int m_splineDegree = 3;
    int m_segmentMiddlePoints = 1;

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    RS_Entity* createSplineForPolyline(RS_Entity *p);
    void fillControlPointsListFromPolyline(const RS_Polyline *polyline, std::vector<RS_Vector> &controlPoints) const;
    void setupAndAddCreatedEntity(RS_Entity *createdEntity, RS_Layer *layerToSet, const RS_Pen &penToUse);
    void onMouseLeftButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void setEntityToModify(RS_Entity* polyline);
    RS2::CursorType doGetMouseCursor(int status) override;
    void updateMouseButtonHints() override;
    LC_ActionOptionsWidget *createOptionsWidget() override;
    void doTrigger() override;
};

#endif // LC_ACTIONSPLINEFROMPOLYLINE_H
