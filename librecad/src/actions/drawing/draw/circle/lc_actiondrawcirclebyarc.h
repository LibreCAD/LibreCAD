/****************************************************************************
**
* Action that creates a circle by given arc or ellipse by ellipse arc

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
#ifndef LC_ACTIONDRAWCIRCLEBYARC_H
#define LC_ACTIONDRAWCIRCLEBYARC_H

#include "lc_abstractactionwithpreview.h"
#include "rs_circle.h"
#include "rs_ellipse.h"

/**
 * Action draws circle with the same center and radius as selected arc.
 * Based on setting, original arc may remains in drawing (so both arc and circle will be present) or just be replaced by circle
 *
 */
class LC_ActionDrawCircleByArc:public LC_AbstractActionWithPreview {
    Q_OBJECT

public:
    LC_ActionDrawCircleByArc(RS_EntityContainer& container,RS_GraphicView& graphicView);
    ~LC_ActionDrawCircleByArc() override;



    bool isReplaceArcByCircle() const{return replaceArcByCircle;};
    void setReplaceArcByCircle(bool value);
    void setPenMode(int i) {penMode = i;};
    int getPenMode() const{return penMode;};

    double getRadiusShift() const{return radiusShift;};
    void setRadiusShift(double shift){radiusShift = shift;};

    void setLayerMode(int mode){layerMode = mode;};
    int getLayerMode() const{return layerMode;}
protected:
    enum{
        SetArc
    };
    LC_ActionOptionsWidget* createOptionsWidget() override;
    bool doCheckMayTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doAfterTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    void performTriggerDeletions() override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    bool doCheckMayTriggerOnInit(int status) override;
    bool isAcceptSelectedEntityToTriggerOnInit(RS_Entity *pEntity) override;
    void doCreateEntitiesOnTrigger(RS_Entity *entity, QList<RS_Entity *> &list) override;
    void doPerformOriginalEntitiesDeletionOnInitTrigger(QList<RS_Entity *> &list) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    void updateMouseButtonHints() override;
private:
    /** Chosen arc or ellipse arc entity */
    RS_Entity *entity = nullptr;

    /**
     * controls whether original arc should be deleted or not
     */
    bool replaceArcByCircle = false;

    //list of entity types supported by current action
    const EntityTypeList circleType = EntityTypeList{RS2::EntityArc, RS2::EntityEllipse};

    /**
     * controls how to apply pen for new entity
     */
    int penMode = PEN_ACTIVE;
    /*
     * controls which layer should be set to new entity
     */
    int layerMode = LAYER_ACTIVE;

    /**
     * adjustment for radius
     */
    double radiusShift = 0.0;

    RS_CircleData createCircleData(RS_Arc* arc);
    RS_EllipseData createEllipseData(RS_Ellipse *pEllipse);
    void deleteOriginalArcOrEllipse(RS_Entity *en);
};

#endif // LC_ACTIONDRAWCIRCLEBYARC_H
