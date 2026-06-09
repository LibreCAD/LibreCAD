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

/**
 * Action draws circle with the same center and radius as selected arc.
 * Based on setting, original arc may remains in drawing (so both arc and circle will be present) or just be replaced by circle
 *
 */
class LC_ActionDrawCircleByArc : public LC_AbstractActionWithPreview {
    Q_OBJECT
public:
    explicit LC_ActionDrawCircleByArc(LC_ActionContext* actionContext);
    ~LC_ActionDrawCircleByArc() override;

    bool isReplaceArcByCircle() const {
        return m_replaceArcByCircle;
    }
    void setReplaceArcByCircle(bool value);

    void setPenMode(const int i) {
        m_penMode = i;
    }

    int getPenMode() const {
        return m_penMode;
    }

    double getRadiusShift() const {
        return m_radiusShift;
    }

    void setRadiusShift(const double shift) {
        m_radiusShift = shift;
    }

    void setLayerMode(const int mode) {
        m_layerMode = mode;
    }

    int getLayerMode() const {
        return m_layerMode;
    }

    void drawSnapper() override;

protected:
    enum {
        SetArc = InitialActionStatus
    };

    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    bool doCheckMayTrigger() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    void doOnLeftMouseButtonRelease(const LC_MouseEvent* e, int status, const RS_Vector& snapPoint) override;
    void doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector& snap, QList<RS_Entity*>& list, int status) override;
    RS_Vector doGetRelativeZeroAfterTrigger() override;
    void doAfterTrigger() override;
    bool doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx) override;
    bool doCheckMayDrawPreview(const LC_MouseEvent* event, int status) override;
    bool doCheckMayTriggerOnInit(int status) override;
    bool isAcceptSelectedEntityToTriggerOnInit(RS_Entity* pEntity) override;
    void doCreateEntitiesOnTrigger(RS_Entity* en, QList<RS_Entity*>& list) override;
    void doPerformOriginalEntitiesDeletionOnInitTrigger(QList<RS_Entity*>& list, LC_DocumentModificationBatch& ctx) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    void updateActionPrompt() override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doSaveOptions() override;
    void doLoadOptions() override;

private:
    /** Chosen arc or ellipse arc entity */
    RS_Entity* m_entity = nullptr;

    /**
     * controls whether original arc should be deleted or not
     */
    bool m_replaceArcByCircle = false;

    //list of entity types supported by current action
    const EntityTypeList m_circleType = EntityTypeList{RS2::EntityArc, RS2::EntityEllipse};

    /**
     * controls how to apply pen for new entity
     */
    int m_penMode = PEN_ACTIVE;
    /*
     * controls which layer should be set to new entity
     */
    int m_layerMode = LAYER_ACTIVE;

    /**
     * adjustment for radius
     */
    double m_radiusShift = 0.0;

    RS_CircleData createCircleData(const RS_Arc* arc) const;
    RS_EllipseData createEllipseData(const RS_Ellipse* ellipseArc) const;
    void deleteOriginalArcOrEllipse(RS_Entity* en, LC_DocumentModificationBatch& ctx) const;
};

#endif
