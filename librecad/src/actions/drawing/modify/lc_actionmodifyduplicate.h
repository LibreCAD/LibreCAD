/****************************************************************************
**
* Action that duplicates entities

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
#ifndef LC_ACTIONMODIFYDUPLICATE_H
#define LC_ACTIONMODIFYDUPLICATE_H

#include "lc_abstractactionwithpreview.h"

class LC_ActionModifyDuplicate:public LC_AbstractActionWithPreview{
    Q_OBJECT
public:
    enum{
        SelectEntity,
        SetOffsetDirection
    };


    LC_ActionModifyDuplicate(RS_EntityContainer& container,RS_GraphicView& graphicView);
    ~LC_ActionModifyDuplicate() override;

    double getOffsetX() const {return offsetX;};
    double getOffsetY() const {return offsetY;};

    void setOffsetX(double value) {offsetX = value;};
    void setOffsetY(double value){offsetY = value;};

    bool isDuplicateInPlace() const{return duplicateInplace;};
    void setDuplicateInPlace(bool value){duplicateInplace = value;};
    int getPenMode() const {return penMode;};
    void setPenMode(int value){penMode = value;};
    int getLayerMode() const{return layerMode;};
    void setLayerMode(int value){layerMode = value;};
protected:
    RS2::CursorType doGetMouseCursor(int status) override;
    RS_Vector doGetMouseSnapPoint(QMouseEvent *e) override;
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doAfterTrigger() override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayTrigger() override;
    void doPrepareTriggerEntities(QList<RS_Entity *> &list) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    bool doCheckMayTriggerOnInit(int status) override;
    bool isAcceptSelectedEntityToTriggerOnInit(RS_Entity *pEntity) override;
    bool doCheckMayDrawPreview(QMouseEvent *event, int status) override;
    void doCreateEntitiesOnTrigger(RS_Entity *entity, QList<RS_Entity *> &list) override;
    void updateMouseButtonHints() override;
private:
    /**
     * entity for which duplicate will be created
     */
    RS_Entity * selectedEntity = nullptr;
    /**
     * point in which offset direction was fixed (for alternative mode)
     */
     RS_Vector triggerPoint = RS_Vector(false);
    /**
     * offset for duplicated entity by X axis
     */
    double offsetX = 0.0;
    /*
     * offset for duplicated entity by Y axis
     */
    double offsetY = 0.0;
    /**
     * flag that indicates that duplicate should be created without offset on the same position as original. That is useful for creation
     * entity's copy on different layer.
     */
    bool duplicateInplace = false;
    /**
     * controls how to apply pen to created duplicate
     */
    int penMode = PEN_ACTIVE;
    /**
     * controls how to apply layer to created duplicate
     */
    int layerMode = LAYER_ACTIVE;

    RS_Vector determineOffset(RS_Vector& snap,const RS_Vector& center) const;
    RS_Vector getEntityCenterPoint(const RS_Entity *en) const;
};

#endif // LC_ACTIONMODIFYDUPLICATE_H
