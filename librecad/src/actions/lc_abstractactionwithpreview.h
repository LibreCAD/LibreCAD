/****************************************************************************
**
* Base class for actions that adds some common workflows and utility methods

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

#ifndef LC_ABSTRACTACTIONWITHPREVIEW_H
#define LC_ABSTRACTACTIONWITHPREVIEW_H

#include "qg_actionhandler.h"
#include "rs_previewactioninterface.h"
#include "rs_vector.h"
#include "rs_line.h"

class RS_Line;
class RS_Point;
class RS_Polyline;

/**
 * Utility base class for actions. It includes some basic logic and utilities, that simplifies creation of specific actions
 * and reduces repetitive code, as well as defines generic workflow for various action methods processing.
 */
class LC_AbstractActionWithPreview :public RS_PreviewActionInterface{
   Q_OBJECT

public:
    LC_AbstractActionWithPreview(const char *name, RS_EntityContainer &container, RS_GraphicView &graphicView);

   // inherited methods with basic template method implementation
    void init(int status) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void finish(bool updateTB) override;
    void trigger() override;
    void updateMouseButtonHints() override;
    /**
   * Mode that controls where from apply pen
   */
    enum {
        PEN_ACTIVE, // use active pen
        PEN_ORIGINAL, // use pen from original entity
        PEN_ORIGINAL_RESOLVED // use resolved pen from original entity
    };

    /**
     * controls how to apply layer to entity's duplicate
     */
    enum {
        LAYER_ACTIVE, // set layer to active layer
        LAYER_ORIGINAL // use the same layer as original
    };


    /**
     * Snapping mode for positioning within line
     */
    enum{
        LINE_SNAP_FREE, // angle line may be at any point
        LINE_SNAP_START, // angle line is in start point of original line
        LINE_SNAP_MIDDLE, // angle line is in the middle of original line
        LINE_SNAP_END // angle line is at the end of original line
    };


protected:


    /**
     * Entity that is highlighted as part of mouse selection operation (if any)
     */
    RS_Entity* highlightedEntity  = nullptr;

    /**
     * Last point of snap during mouse move
     */
    RS_Vector lastSnapPoint  = RS_Vector(false);

    /**
    * This is "major" status of action - it is used for determining, to which status state should be changed after various intermediate statuses (mostly,
    * this is needed for support of command events);
    */
    int mainStatus  = 0;

    /**
     * snap mode saved for further restored, convenient if the actions would like to manage current snap (say, for simpler selection of entities)
     */
    uint savedSnapMode  = 0;

    /**
     * reference to action handler. It is not initialized and it is up to inherited actions to initialize it (mostly via constructor)
     */
    QG_ActionHandler* actionhandler  = nullptr;

    /**
     * This is alternative mode of action that is invoked if SHIFT is pressed together with
     * mouse move or mouse click. Meaning of this flag depends on particular action.
     */
    bool alternativeActionMode = false;

    // functions starting with "do" prefix are the most probable candidates for overriding in inherited actions

    /**
     * controls whether preview should be created for mouse event
     * @param event mouse event
     * @param status current status of action
     * @return true if we need preview
     */
    virtual bool doCheckMayDrawPreview([[maybe_unused]]QMouseEvent *event, int status);
    /**
     * method for handling
     * @param e
     * @param status
     * @param snapPoint
     */
    virtual void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint);
    virtual RS_Vector doGetMouseSnapPoint(QMouseEvent *e);

    /**
     * extension point for performing cleanup on action finish
     * @param updateTB
     */

    virtual void doFinish(bool updateTB);
    /**
     * extension point for processing of back to previous state operation
     * @param pEvent
     * @param status
     */
    virtual void doBack([[maybe_unused]]QMouseEvent *pEvent, int status);

    virtual bool onMouseMove([[maybe_unused]]QMouseEvent *e, RS_Vector snap, int status);
    virtual void doPreparePreviewEntities([[maybe_unused]]QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status);
    virtual bool doCheckMayTrigger();
    virtual void doAfterTrigger();
    RS2::CursorType doGetMouseCursor(int status) override;
    virtual RS_Vector doGetRelativeZeroAfterTrigger();
    virtual int doGetStatusForInitialSnapToRelativeZero();
    virtual void doInitialSnapToRelativeZero(RS_Vector relZero);
    virtual void doMouseMoveEnd(int status, [[maybe_unused]]QMouseEvent *e);
    virtual void doMouseMoveStart(int status, QMouseEvent *pEvent);
    virtual void doPrepareTriggerEntities(QList<RS_Entity *> &list);

    // additional setup methods, that may be overridden if necessary

    // additional triggering action support
    virtual bool isSetActivePenAndLayerOnTrigger();
    virtual bool isUndoableTrigger();
    virtual void performTrigger();
    virtual void performTriggerInsertions();
    virtual void performTriggerDeletions();
    virtual void finishAction();

    // trigger on init support (for selected entities)
    virtual bool doCheckMayTriggerOnInit(int status);
    virtual bool isAcceptSelectedEntityToTriggerOnInit(RS_Entity *pEntity);
    virtual void doCreateEntitiesOnTrigger(RS_Entity *entity, QList<RS_Entity *> &list);
    virtual void performTriggerOnInit(QList<RS_Entity*>  entities);
    virtual void doPerformOriginalEntitiesDeletionOnInitTrigger(QList<RS_Entity *> &list);

    // utility functions
    virtual void drawPreviewForPoint(QMouseEvent *e, RS_Vector& snap);

    // default implementation of right mouse release
    virtual void onRightMouseButtonRelease(QMouseEvent *e, int status);


    void unHighlightEntity();
    void highlightEntity(RS_Entity *en);
    void highlightEntityExplicit(RS_Entity *en, bool highlight);

    void drawPreviewForLastPoint();

    virtual void checkPreSnapToRelativeZero(int status, QMouseEvent *pEvent);
    virtual bool doCheckMouseEventValidForInitialSnap(QMouseEvent *e);

    // main status support
    void setMainStatus(int status) {mainStatus = status; setStatus(status);}
    void restoreMainStatus(){setStatus(mainStatus);}

    // snap control support
    void restoreSnapMode();
    void setFreeSnap();
    void setGlobalSnapMode(const RS_SnapMode &mode);
    void setupAndAddTriggerEntities(const QList<RS_Entity *> &entities);
    void unSelectEntities(const QList<RS_Entity *> &entities);
    virtual bool isUnselectEntitiesOnInitTrigger();
    void applyPenAndLayerBySourceEntity(const RS_Entity *source, RS_Entity *target, int penMode, int layerMode) const;
    bool checkMayExpandEntity(const RS_Entity *e, const QString &entityName) const;
    RS_Point* createPoint(const RS_Vector &coord, QList<RS_Entity *> &list) const;
    RS_Line* createLine(const RS_Vector &startPoint, const RS_Vector &endPoint, QList<RS_Entity *> &list) const;
    RS_Line *createLine(const RS_LineData &lineData, QList<RS_Entity *> &list) const;
    virtual void checkAlternativeActionMode(const QMouseEvent *e, int status, bool shiftPressed);
    virtual void clearAlternativeActionMode();
    void updateSnapperAndCoordinateWidget(QMouseEvent *e, int status);
    void doUpdateCoordinateWidgetByMouse(QMouseEvent *e);
    void createRefLine(const RS_Vector &startPoint, const RS_Vector &endPoint, QList<RS_Entity *> &list) const;
    void createRefPoint(const RS_Vector &coord, QList<RS_Entity *> &list) const;
    void createRefSelectablePoint(const RS_Vector &coord, QList<RS_Entity *> &list) const;
    static bool isMouseMove(QMouseEvent* e);
    void createRefArc(const RS_ArcData &data, QList<RS_Entity *> &list) const;
};
#endif // LC_ABSTRACTACTIONWITHPREVIEW_H
