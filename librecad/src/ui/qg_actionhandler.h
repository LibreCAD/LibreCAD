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
class QG_ActionHandler : public QObject {
	Q_OBJECT

public:
	QG_ActionHandler(QObject* parent);
	virtual ~QG_ActionHandler() = default;

	RS_ActionInterface* getCurrentAction();
	RS_ActionInterface* setCurrentAction(RS2::ActionType id);

	/**
	 * Kills all running selection actions. Called when a selection action
	  * is launched to reduce confusion.
	   */
	void killSelectActions();
	/**
		 * @brief killAllActions kill all actions
		 */
	void killAllActions();

	bool keycode(const QString& code);
	//special handling of actions issued from command line, currently used for snap actions
	//return true if handled
	bool commandLineActions(RS2::ActionType id);
	bool command(const QString& cmd);
	QStringList getAvailableCommands();
	RS_SnapMode getSnaps();
	RS2::SnapRestriction getSnapRestriction();
    void set_view(RS_GraphicView* graphic_view);
    void set_document(RS_Document* document);
    void set_snap_toolbar(QG_SnapToolBar* snap_toolbar);

public slots:
	/*void slotFileNew();*/
	void slotFileNewTemplate();
	void slotFileOpen();
	/*
		  void slotFileOpen(const QString& fileName);
		  void slotFileSave();
	 */
	void slotFileSaveAs();
	void slotFileExportMakerCam();
	/*
		 void slotFileClose();
		 void slotFilePrint();
	  */

	void slotZoomIn();
	void slotZoomOut();
	void slotZoomAuto();
	void slotZoomWindow();
	void slotZoomPan();
	void slotZoomPrevious();
	void slotZoomRedraw();

	void slotToolRegenerateDimensions();

	void slotEditKillAllActions();
	void slotEditUndo();
	void slotEditRedo();
	void slotEditCut();
	void slotEditCopy();
	void slotEditPaste();
	void slotOrderBottom();
	void slotOrderLower();
	void slotOrderRaise();
	void slotOrderTop();

	void slotSelectSingle();
	void slotSelectContour();
	void slotSelectWindow();
	void slotDeselectWindow();
	void slotSelectAll();
	void slotDeselectAll();
	void slotSelectInvert();
	void slotSelectIntersected();
	void slotDeselectIntersected();
	void slotSelectLayer();

	void slotDrawPoint();
	void slotDrawLine();
	void slotDrawLineAngle();
	void slotDrawLineHorizontal();
	void slotDrawLineVertical();
	void slotDrawLineFree();
	void slotDrawLineHorVert();
	void slotDrawLineParallel();
	void slotDrawLineParallelThrough();
	void slotDrawLineRectangle();
	void slotDrawLineBisector();
	void slotDrawLineTangent1();
	void slotDrawLineTangent2();
	void slotDrawLineOrthogonal();
	void slotDrawLineOrthTan();
	void slotDrawLineRelAngle();
	void slotDrawLinePolygon();
	void slotDrawLinePolygon2();
    void slotDrawLinePolygon3();//added by txmy
	void slotDrawCircle();
	void slotDrawCircleCR();
	void slotDrawCircle2P();
	void slotDrawCircle2PR();
	void slotDrawCircle3P();
	void slotDrawCircleParallel();
	void slotDrawCircleInscribe();
	void slotDrawCircleTan2();
	void slotDrawCircleTan3();
	void slotDrawCircleTan1_2P();
	void slotDrawCircleTan2_1P();
	void slotDrawArc();
	void slotDrawArc3P();
	void slotDrawArcParallel();
	void slotDrawArcTangential();
	void slotDrawEllipseAxis();
	void slotDrawEllipseArcAxis();
	void slotDrawEllipseFociPoint();
	void slotDrawEllipse4Points();
	void slotDrawEllipseCenter3Points();
	void slotDrawEllipseInscribe();
	void slotDrawSpline();
	void slotDrawSplinePoints();
	void slotDrawMText();
	void slotDrawText();
	void slotDrawHatch();
	void slotDrawImage();
	void slotDrawPolyline();
	void slotPolylineAdd();
	void slotPolylineAppend();
	void slotPolylineDel();
	void slotPolylineDelBetween();
	void slotPolylineTrim();
	void slotPolylineEquidistant();
	void slotPolylineSegment();

	void slotDimAligned();
	void slotDimLinear();
	void slotDimLinearHor();
	void slotDimLinearVer();
	void slotDimRadial();
	void slotDimDiametric();
	void slotDimAngular();
	void slotDimLeader();

	void slotModifyAttributes();
	void slotModifyDelete();
	void slotModifyDeleteQuick();
	void slotModifyDeleteFree();
	void slotModifyMove();
	void slotModifyScale();
	void slotModifyRevertDirection();
	void slotModifyRotate();
	void slotModifyMirror();
	void slotModifyMoveRotate();
	void slotModifyRotate2();
	void slotModifyEntity();
	void slotModifyTrim();
	void slotModifyTrim2();
	void slotModifyTrimAmount();
	void slotModifyCut();
	void slotModifyStretch();
	void slotModifyBevel();
	void slotModifyRound();
	void slotModifyOffset();
	void slotModifyExplodeText();

	void slotSetSnaps(RS_SnapMode const& s);
	void slotSnapFree();
	void slotSnapGrid();
	void slotSnapEndpoint();
	void slotSnapOnEntity();
	void slotSnapCenter();
	void slotSnapMiddle();
	void slotSnapDist();
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

	void slotInfoInside();
	void slotInfoDist();
	void slotInfoDist2();
	void slotInfoAngle();
	void slotInfoTotalLength();
	void slotInfoArea();

	void slotLayersDefreezeAll();
	void slotLayersFreezeAll();
	void slotLayersUnlockAll();
	void slotLayersLockAll();
	void slotLayersAdd();
	void slotLayersRemove();
	void slotLayersEdit();
	void slotLayersToggleView();
	void slotLayersToggleLock();
	void slotLayersTogglePrint();
	void slotLayersToggleConstruction();

	void slotBlocksDefreezeAll();
	void slotBlocksFreezeAll();
	void slotBlocksAdd();
	void slotBlocksRemove();
	void slotBlocksAttributes();
	void slotBlocksEdit();
	void slotBlocksSave();
	void slotBlocksInsert();
	void slotBlocksToggleView();
	void slotBlocksCreate();
	void slotBlocksExplode();
	void slotOptionsDrawing();

    void toggleVisibility(RS_Layer* layer);
    void toggleLock(RS_Layer* layer);
    void togglePrint(RS_Layer* layer);
    void toggleConstruction(RS_Layer* layer);
private:

	// Type of draw order selected command
    RS2::ActionType orderType{RS2::ActionOrderTop};
    QG_SnapToolBar* snap_toolbar{nullptr};
    RS_GraphicView* view{nullptr};
    RS_Document*    document{nullptr};
};

#endif
// EOF
