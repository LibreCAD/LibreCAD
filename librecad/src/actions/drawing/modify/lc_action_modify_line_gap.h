/****************************************************************************
**
* Action that creates a gap in selected line and so creates two line segments

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

#ifndef LC_ACTIONMODIFYLINEGAP_H
#define LC_ACTIONMODIFYLINEGAP_H

#include "lc_abstractactionwithpreview.h"

class LC_ActionModifyLineGap:public LC_AbstractActionWithPreview{
    Q_OBJECT
    /**
     * entity states
     */
    enum{
        SetEntity = InitialActionStatus,
        SetGapEndPoint
    };

public:
    explicit LC_ActionModifyLineGap(LC_ActionContext *actionContext);

    double getGapSize()const{return m_gapSize;}
    void setGapSize(const double value){m_gapSize =value;}

    int getLineSnapMode() const{return m_lineSnapMode;}
    void setLineSnapMode(const int value){m_lineSnapMode = value;}

    double getGapSnapDistance() const{return m_gapSnapDistance;}
    void setGapSnapDistance(const double value){m_gapSnapDistance = value;}

    bool isFreeGapSize() const{return m_freeGapSize;}
    void setFreeGapSize(const bool value){m_freeGapSize = value;}

    int getGapSnapMode() const {
        return m_gapSnapMode;
    }
    void setGapSnapMode(const int mode){m_gapSnapMode = mode;}

    /**
    * Snapping for gap - if gap's size is fixed (not free)
    */
    enum GapSnap{
        GAP_SNAP_START, //start point of gap in line snap point
        GAP_SNAP_MIDDLE, // middle point of gap in line snap point
        GAP_SNAP_END // end point of gap is in line snap point
    };
protected:

    /**
     * length of gap for fixed size mode
     */
    double m_gapSize = 0.0;

    /**
     * snap mode that controls in which part of line the gap will be placed.
     * In alternative action mode (SHIFT pressed with mouse), snap mode for line
     * and gap is mirrored (so they are applied to another end of line)
     */
    int m_lineSnapMode = LINE_SNAP_START;

    /**
     * distance of gap snap point from line snap point (if not free gap mode)
     */
    double m_gapSnapDistance = 0.0;

    /**
     * if true - gap size is selected by the user, if false - it is defined by the option
     */
    bool m_freeGapSize = false;

    /**
     * controls how the gap is positioned to gap snap point
     */
    int m_gapSnapMode = GAP_SNAP_START;

    struct GapData{
        RS_Line* originalLine;
        RS_Vector startPoint;
        RS_Vector endPoint;
    };

    /**
     * information of gap needed for trigger
     */
    GapData* m_gapData = nullptr;

    void doInitWithContextEntity(RS_Entity* contextEntity, const RS_Vector& clickPos) override;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    LC_ActionOptionsPropertiesFiller* createOptionsFiller() override;
    void doPreparePreviewEntities(const LC_MouseEvent* e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    RS_Vector obtainLineSnapPointForMode(const RS_Line *targetLine, const RS_Vector &snap) const;
    GapData *prepareGapData(RS_Line *line, const RS_Vector &snap, const RS_Vector &startPoint) const;
    void doOnLeftMouseButtonRelease(const LC_MouseEvent* e, int status, const RS_Vector &snapPoint) override;
    bool doCheckMayTrigger() override;
    void doAfterTrigger() override;
    bool isSetActivePenAndLayerOnTrigger() override;
    bool doTriggerEntitiesPrepare(LC_DocumentModificationBatch& ctx)  override;
    void doFinish() override;
    void createPreviewEntities(const GapData *data, QList<RS_Entity *> &list, bool startPointNoSelected) const;
    void updateActionPrompt() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool doUpdateDistanceByInteractiveInput(const QString& tag, double distance) override;
    void doSaveOptions() override;
    void doLoadOptions() override;
    bool isInVisualSnapStatus(int status) override;
};

#endif
