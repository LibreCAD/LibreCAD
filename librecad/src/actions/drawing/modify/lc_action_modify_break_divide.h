/****************************************************************************
**
* Action that breaks line, arc or circle to segments by points of intersection
* with other entities.

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
#ifndef LC_ACTIONMODIFYBREAKOUTLINE_H
#define LC_ACTIONMODIFYBREAKOUTLINE_H

#include "lc_abstractactionwithpreview.h"

class RS_Layer;
class RS_Pen;

class LC_ActionModifyBreakDivide:public LC_AbstractActionWithPreview{
    Q_OBJECT
   /**
   * action state
   */
   enum{
        SetLine
    };

    /**
     * Structure used to pass data to trigger method
     */
    struct TriggerData{
        RS_Entity* entity;
        RS_Vector snapPoint;
        QList<RS_Entity *> entitiesToCreate;
    };

public:
    explicit LC_ActionModifyBreakDivide(LC_ActionContext *actionContext);

    bool isRemoveSegment() const{return m_removeSegments;}
    void setRemoveSegment(const bool value){m_removeSegments = value;}
    bool isRemoveSelected() const{return m_removeSelected;}
    void setRemoveSelected(const bool value){m_removeSelected = value;}
protected:
    /**
     * Flag that defines whether we should remove segments of entity or just divide entity
     */
    bool m_removeSegments = false;

    /**
     * For segments removal, specifies whether it is necessary to remove selected segment or remaining ones
     */
    bool m_removeSelected = false;

    TriggerData* m_triggerData = nullptr;

    bool doCheckMayDrawPreview(const LC_MouseEvent* event, int status) override;
    void doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void createEntitiesForLine(const RS_Line *line, const RS_Vector &snap, QList<RS_Entity *> &list, bool preview) const;
    void createEntitiesForCircle(const RS_Circle* circle, RS_Vector &snap, QList<RS_Entity *> &list, bool preview) const;
    void createEntitiesForArc(RS_Arc *arc, RS_Vector &snap, QList<RS_Entity *> &list, bool preview) const;
    void doOnLeftMouseButtonRelease(const LC_MouseEvent* e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayTrigger() override;
    bool doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx) override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool isSetActivePenAndLayerOnTrigger() override;
    void createArcEntity(const RS_ArcData &arcData, bool preview, const RS_Pen &pen, RS_Layer *layer, QList<RS_Entity *> &list) const;
    void createLineEntity(bool preview, const RS_Vector &start, const RS_Vector &end, const RS_Pen &pen, RS_Layer *layer, QList<RS_Entity *> &list) const;
    void doAfterTrigger() override;
    void doFinish() override;
    RS_Vector doGetMouseSnapPoint(const LC_MouseEvent* e) override;
    void updateActionPrompt() override;
    void doSaveOptions() override;
    void doLoadOptions() override;
};

#endif
