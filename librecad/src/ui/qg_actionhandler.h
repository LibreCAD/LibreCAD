/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef QG_ACTIONHANDLER_H
#define QG_ACTIONHANDLER_H

#include "rs_actioninterface.h"

class QG_SnapToolBar;
class RS_Layer;
/**
 * This class can trigger actions (from menus, buttons, ...).
 */
class QG_ActionHandler:public QObject {
Q_OBJECT

public:
    QG_ActionHandler(QObject *parent);
    virtual ~QG_ActionHandler() = default;
    RS_ActionInterface *getCurrentAction();
    RS_ActionInterface *setCurrentAction(RS2::ActionType id);
/**
 * Kills all running selection actions. Called when a selection action
  * is launched to reduce confusion.
   */
    void killSelectActions();
/**
  * @brief killAllActions kill all actions
  */
    void killAllActions();
    bool keycode(const QString &code);
//special handling of actions issued from command line, currently used for snap actions
//return true if handled
    bool commandLineActions(RS2::ActionType id);
    bool command(const QString &cmd);
    QStringList getAvailableCommands();
    RS_SnapMode getSnaps();
    RS2::SnapRestriction getSnapRestriction();
    void set_view(RS_GraphicView *graphic_view);
    void set_document(RS_Document *document);
    void set_snap_toolbar(QG_SnapToolBar *snap_toolbar);

public slots:
/*void slotFileNew();*/
    void slotFileNewTemplate();
    void slotFileOpen();
/*
   void slotFileOpen(const QString& fileName);
   void slotFileSave();
 */
    void slotFileSaveAs();
    void slotFileExportMakerCam(); // fixme - remove
/*
  void slotFileClose();
  void slotFilePrint();
  */

    void slotZoomIn(); // fixme - remove
    void slotZoomOut();// fixme - remove
    void slotZoomAuto();// fixme - remove
    void slotZoomWindow();// fixme - remove
    void slotZoomPan(); // fixme - remove
    void slotZoomPrevious();// fixme - remove
    void slotZoomRedraw();// fixme - remove
    void slotToolRegenerateDimensions();
    void slotEditKillAllActions();// fixme - remove
    void slotEditUndo();// fixme - remov
    void slotEditRedo();// fixme - remov
    void slotEditCut();// fixme - remov
    void slotEditCopy();// fixme - remov
    void slotEditPaste();// fixme - remov
    void slotOrderBottom();// fixme - remove
    void slotOrderLower();// fixme - remove
    void slotOrderRaise();// fixme - remove
    void slotOrderTop();// fixme - remove
    void slotSelectSingle(); // fixme - remove
    void slotSelectContour(); // fixme - remove
    void slotSelectWindow(); // fixme - remove
    void slotDeselectWindow(); // fixme - remove
    void slotSelectAll(); // fixme - remove
    void slotDeselectAll(); // fixme - remove
    void slotSelectInvert(); // fixme - remove
    void slotSelectIntersected(); // fixme - remove
    void slotDeselectIntersected(); // fixme - remove
    void slotSelectLayer();  // fixme - remove
//    void slotDrawPoint();// fixme - remove
    void slotDrawLine(); // fixme - remove
    void slotDrawLineAngle(); // fixme - remove
    void slotDrawLineHorizontal();// fixme - remove
    void slotDrawLineVertical();// fixme - remove
    void slotDrawLineFree(); // fixme - remove
    void slotDrawLineHorVert();
    void slotDrawLineParallel(); // fixme - remove
    void slotDrawLineParallelThrough();// fixme - remove
    void slotDrawLineRectangle();// fixme - remove
    void slotDrawLineRectangle3Points(); // fixme - remove
    void slotDrawLineRectangle1Point(); // fixme - remove
    void slotDrawLineRectangle2Points(); // fixme - remove
    void slotDrawLineBisector(); // fixme - remove
    void slotDrawLineTangent1(); // fixme - remove
    void slotDrawLineTangent2();// fixme - remove
    void slotDrawLineOrthogonal(); // fixme - remove
    void slotDrawLineOrthTan(); // fixme - remove
    void slotDrawLineRelAngle(); // fixme - remove
    void slotDrawLinePolygon(); // fixme - remove
    void slotDrawLinePolygon2(); // fixme - remove
    void slotDrawLinePolygon3();//added by txmy // fixme - remove
    void slotDrawLinePoints(); // fixme - remove
    void slotDrawCircle(); // fixme - remove
    void slotDrawCircleCross(); // fixme - remove
    void slotDrawLineSnake(); // fixme - remove
    void slotDrawLineSnakeX(); // fixme - remove
    void slotDrawLineSnakeY(); // fixme - remove
    void slotDrawLineOrthogonalRel(); // fixme - remove
    void slotDrawLineOrthogonalTo(); // fixme - remove
    void slotDrawLineAngleRel(); // fixme - remove
    void slotDrawSliceDivideLine(); // fixme - remove
    void slotDrawSliceDivideCircle(); // fixme - remove
    void slotDrawCircleCR(); // fixme - remove
    void slotDrawCircle2P(); // fixme - remove
    void slotDrawCircle2PR(); // fixme - remove
    void slotDrawCircle3P(); // fixme - remove
    void slotDrawCircleParallel(); // fixme - remove
    void slotDrawCircleInscribe(); // fixme - remove
    void slotDrawCircleTan2(); // fixme - remove
    void slotDrawCircleTan3(); // fixme - remove
    void slotDrawCircleTan1_2P(); // fixme - remove
    void slotDrawCircleTan2_1P(); // fixme - remove
    void slotDrawCircleByArc(); // fixme - remove
    void slotDrawArc(); // fixme - remove
    void slotDrawArc3P(); // fixme - remove
    void slotDrawArcParallel(); // fixme - remove
    void slotDrawArcTangential(); // fixme - remove
    void slotDrawEllipseAxis(); // fixme - remove
    void slotDrawEllipseArcAxis(); // fixme - remove
    void slotDrawEllipseFociPoint(); // fixme - remove
    void slotDrawEllipse4Points(); // fixme - remove
    void slotDrawEllipseCenter3Points(); // fixme - remove
    void slotDrawEllipseInscribe(); // fixme - remove
    void slotDrawParabola4Points(); // fixme - remove
    void slotDrawParabolaFD(); // fixme - remove
    void slotDrawSpline(); // fixme - remove
    void slotDrawSplinePoints(); // fixme - remove
    void slotDrawMText(); // fixme  - remove
    void slotDrawText(); // fixme - remove
    void slotDrawHatch(); // fixme - remove
    void slotDrawImage(); // fixme - remove
    void slotDrawPolyline(); // fixme - remove
    void slotPolylineAdd(); // fixme - remove
    void slotPolylineAppend(); // fixme - remove
    void slotPolylineDel(); // fixme - remove
    void slotPolylineDelBetween(); // fixme - remove
    void slotPolylineTrim(); // fixme - remove
    void slotPolylineEquidistant(); // fixme - remove
    void slotPolylineSegment(); // fixme - remove
    void slotDimAligned(); // fixme - remove
    void slotDimLinear(); // fixme - remove
    void slotDimLinearHor(); // fixme - remove
    void slotDimLinearVer(); // fixme - remove
    void slotDimRadial(); // fixme - remove
    void slotDimDiametric(); // fixme - remove
    void slotDimAngular(); // fixme - remove
    void slotDimArc(); // fixme - remove
    void slotDimLeader(); // fixme - remove
    void slotModifyAttributes(); // fixme - remove
    void slotModifyDelete(); // fixme - remove
    void slotModifyDeleteQuick();
    void slotModifyDeleteFree();
    void slotModifyMove();  // fixme - remove
    void slotModifyScale(); // fixme - remove
    void slotModifyRevertDirection(); // fixme - remove
    void slotModifyRotate(); // fixme - remove
    void slotModifyMirror(); // fixme - remove
    void slotModifyMoveRotate(); // fixme - remove
    void slotModifyRotate2(); // fixme - remove
    void slotModifyEntity(); // fixme - remove
    void slotModifyTrim(); // fixme - remove
    void slotModifyTrim2(); // fixme - remove
    void slotModifyTrimAmount(); // fixme - remove
    void slotModifyCut(); // fixme - remove
    void slotModifyStretch(); // fixme - remove
    void slotModifyBevel(); // fixme - remove
    void slotModifyRound(); // fixme - remove
    void slotModifyOffset(); // fixme - remove
    void slotModifyExplodeText(); // fixme - remove
    void slotSetSnaps(RS_SnapMode const &s);
    void slotSnapFree();
    void slotSnapGrid();
    void slotSnapEndpoint();
    void slotSnapOnEntity();
    void slotSnapCenter();
    void slotSnapMiddle();
    void slotSnapDist();
    void slotSnapMiddleManual();
    void slotSnapIntersection();
    void slotSnapIntersectionManual();
    void slotRestrictNothing();
    void slotRestrictOrthogonal();
    void slotRestrictHorizontal();
    void slotRestrictVertical();
    void disableSnaps();
    void disableRestrictions();
    void slotSetRelativeZero();
    void slotLockRelativeZero(bool on);
    void slotInfoInside(); // fixme - remove
    void slotInfoDist();// fixme - remove
    void slotInfoDist2();// fixme - remove
    void slotInfoDist3();// fixme - remove
    void slotInfoAngle();// fixme - remove
    void slotInfoTotalLength();// fixme - remove
    void slotInfoArea();// fixme - remove
    void slotEntityInfo();// fixme - remove
    void slotPickCoordinates();
    void slotLayersDefreezeAll(); // fixme - remove
    void slotLayersFreezeAll();// fixme - remove
    void slotLayersUnlockAll();// fixme - remove
    void slotLayersLockAll();// fixme - remove
    void slotLayersAdd();// fixme - remove
    void slotLayersRemove();// fixme - remove
    void slotLayersEdit();// fixme - remove
    void slotLayersToggleView();// fixme - remove
    void slotLayersToggleLock();// fixme - remove
    void slotLayersTogglePrint();// fixme - remove
    void slotLayersToggleConstruction();// fixme - remove
    void slotLayersExportSelected();// fixme - remove
    void slotLayersExportVisible();// fixme - remove
    void slotBlocksDefreezeAll(); // fixme - remove
    void slotBlocksFreezeAll(); // fixme - remove
    void slotBlocksAdd(); // fixme - remove
    void slotBlocksRemove(); // fixme - remove
    void slotBlocksAttributes(); // fixme - remove
    void slotBlocksEdit(); // fixme - remove
    void slotBlocksSave(); // fixme - remove
    void slotBlocksInsert(); // fixme - remove
    void slotBlocksToggleView(); // fixme - remove
    void slotBlocksCreate(); // fixme - remove
    void slotBlocksExplode(); // fixme - remove
    void slotModifyLineJoin(); // fixme - remove
    void slotModifyDuplicate(); // fixme - remove
    void slotDrawStar(); // fixme - remove
    void slotModifyBreakDivide(); // fixme - remove
    void slotModifyLineGap(); // fixme - remove
    void slotOptionsDrawing(); // fixme - remove
    void toggleVisibility(RS_Layer *layer);
    void toggleLock(RS_Layer *layer);
    void togglePrint(RS_Layer *layer);
    void toggleConstruction(RS_Layer *layer);
    void slotPenPick(); // fixme - remove
    void slotPenPickResolved(); // fixme - remove
    void slotPenApply(); // fixme - remove
    void slotPenCopy(); // fixme - remove
    void slotPenSyncFromLayer(); // fixme - remove
private:
    // Type of draw order selected command
    RS2::ActionType orderType{RS2::ActionOrderTop};
    QG_SnapToolBar *snap_toolbar{nullptr};
    RS_GraphicView *view{nullptr};
    RS_Document *document{nullptr};
};

#endif