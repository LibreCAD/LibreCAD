/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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

class QG_MainWindowInterface;
//class QG_CadToolBarSnap;
class QG_SnapToolBar;
//class QG_CadToolBarSelect;



/**
 * This class can trigger actions (from menus, buttons, ...).
 */
class QG_ActionHandler : public QObject {
    Q_OBJECT

public:
    QG_ActionHandler(QG_MainWindowInterface* mw);
    virtual ~QG_ActionHandler();

        RS_ActionInterface* getCurrentAction();
    RS_ActionInterface* setCurrentAction(RS2::ActionType id);
        void killSelectActions();

        bool keycode(const QString& code);
        bool command(const QString& cmd);
        QStringList getAvailableCommands();
    RS_SnapMode getSnaps();
   RS2::SnapRestriction getSnapRestriction();

public slots:
    /*void slotFileNew();*/
    void slotFileOpen();
    /*
          void slotFileOpen(const QString& fileName);
          void slotFileSave();
     */
    void slotFileSaveAs();
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
    void slotDrawCircle();
    void slotDrawCircleCR();
    void slotDrawCircle2P();
    void slotDrawCircle3P();
    void slotDrawCircleParallel();
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
    void slotModifyExplodeText();

    void slotSetSnaps(RS_SnapMode);
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
    void updateSnapMode(RS_SnapMode&);

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
    void slotLayersAdd();
    void slotLayersRemove();
    void slotLayersEdit();
    void slotLayersToggleView();
    void slotLayersToggleLock();

    void slotBlocksDefreezeAll();
    void slotBlocksFreezeAll();
    void slotBlocksAdd();
    void slotBlocksRemove();
    void slotBlocksAttributes();
    void slotBlocksEdit();
    void slotBlocksInsert();
    void slotBlocksToggleView();
    void slotBlocksCreate();
    void slotBlocksExplode();
    void slotOptionsDrawing();
    void slotFocusNormal();

    void setActionSnapFree(QAction* a) {
        snapFree = a;
    }
    void setActionSnapGrid(QAction* a) {
        snapGrid = a;
    }
    void setActionSnapEndpoint(QAction* a) {
        snapEndpoint = a;
    }
    void setActionSnapOnEntity(QAction* a) {
        snapOnEntity = a;
    }
    void setActionSnapCenter(QAction* a) {
        snapCenter = a;
    }
    void setActionSnapMiddle(QAction* a) {
        snapMiddle = a;
    }
    void setActionSnapDist(QAction* a) {
        snapDistance = a;
    }
    void setActionSnapIntersection(QAction* a) {
        snapIntersection = a;
    }
    void setActionSnapIntersectionManual(QAction* a) {
        snapIntersectionManual = a;
    }
    void setActionRestrictNothing(QAction* a) {
        restrictNothing = a;
    }
    void setActionRestrictOrthogonal(QAction* a) {
        restrictOrthogonal = a;
    }
    void setActionRestrictHorizontal(QAction* a) {
        restrictHorizontal = a;
    }
    void setActionRestrictVertical(QAction* a) {
        restrictVertical = a;
    }
    void setActionLockRelativeZero(QAction* a) {
        lockRelativeZero = a;
    }

    //not needed any more, will be removed
    void setSnapToolBar(QG_SnapToolBar* tb);

private:
    QG_MainWindowInterface* mainWindow;
    QG_SnapToolBar* snapToolBar;

    QAction* snapFree;
    QAction* snapGrid;
    QAction* snapEndpoint;
    QAction* snapOnEntity;
    QAction* snapCenter;
    QAction* snapMiddle;
    QAction* snapDistance;
    QAction* snapIntersection;
    QAction* snapIntersectionManual;

    QAction* restrictNothing;
    QAction* restrictOrthogonal;
    QAction* restrictHorizontal;
    QAction* restrictVertical;

    QAction* lockRelativeZero;
    bool lockedRelZero;
};

#endif

// EOF
