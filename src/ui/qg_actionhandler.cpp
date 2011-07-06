/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#include "qg_actionhandler.h"

#include "rs_dialogfactory.h"
#include "rs_commandevent.h"
#include "rs_commands.h"

#include "rs_actionblocksadd.h"
#include "rs_actionblocksattributes.h"
#include "rs_actionblockscreate.h"
#include "rs_actionblocksedit.h"
#include "rs_actionblocksexplode.h"
#include "rs_actionblocksfreezeall.h"
#include "rs_actionblocksinsert.h"
#include "rs_actionblocksremove.h"
#include "rs_actionblockstoggleview.h"
#include "rs_actiondimaligned.h"
#include "rs_actiondimangular.h"
#include "rs_actiondimdiametric.h"
#include "rs_actiondimleader.h"
#include "rs_actiondimlinear.h"
#include "rs_actiondimradial.h"
#include "rs_actiondrawarc.h"
#include "rs_actiondrawarc3p.h"
#include "rs_actiondrawarctangential.h"
#include "rs_actiondrawcircle.h"
#include "rs_actiondrawcircle2p.h"
#include "rs_actiondrawcircle3p.h"
#include "rs_actiondrawcirclecr.h"
#include "rs_actiondrawellipseaxis.h"
#include "rs_actiondrawhatch.h"
#include "rs_actiondrawimage.h"
#include "rs_actiondrawline.h"
#include "rs_actiondrawlineangle.h"
#include "rs_actiondrawlinebisector.h"
#include "rs_actiondrawlinefree.h"
#include "rs_actiondrawlinehorvert.h"
#include "rs_actiondrawlineparallel.h"
#include "rs_actiondrawlineparallelthrough.h"
#include "rs_actiondrawlinepolygon.h"
#include "rs_actiondrawlinepolygon2.h"
#include "rs_actiondrawlinerectangle.h"
#include "rs_actiondrawlinerelangle.h"
#include "rs_actiondrawlinetangent1.h"
#include "rs_actiondrawlinetangent2.h"
#include "rs_actiondrawpoint.h"
#include "rs_actiondrawspline.h"
#include "rs_actiondrawtext.h"
#include "rs_actioneditcopy.h"
#include "rs_actioneditpaste.h"
#include "rs_actioneditundo.h"
#include "rs_actionfileopen.h"
#include "rs_actionfilesaveas.h"
#include "rs_actioninfoangle.h"
#include "rs_actioninfoarea.h"
#include "rs_actioninfodist.h"
#include "rs_actioninfodist2.h"
#include "rs_actioninfoinside.h"
#include "rs_actioninfototallength.h"
#include "rs_actionlayersadd.h"
#include "rs_actionlayersedit.h"
#include "rs_actionlayersfreezeall.h"
#include "rs_actionlayersremove.h"
#include "rs_actionlayerstogglelock.h"
#include "rs_actionlayerstoggleview.h"
#include "rs_actionlibraryinsert.h"
#include "rs_actionlockrelativezero.h"
#include "rs_actionmodifyattributes.h"
#include "rs_actionmodifybevel.h"
#include "rs_actionmodifycut.h"
#include "rs_actionmodifydelete.h"
#include "rs_actionmodifydeletefree.h"
#include "rs_actionmodifydeletequick.h"
#include "rs_actionmodifyentity.h"
#include "rs_actionmodifyexplodetext.h"
#include "rs_actionmodifymirror.h"
#include "rs_actionmodifymove.h"
#include "rs_actionmodifymoverotate.h"
#include "rs_actionmodifyrotate.h"
#include "rs_actionmodifyrotate2.h"
#include "rs_actionmodifyround.h"
#include "rs_actionmodifyscale.h"
#include "rs_actionmodifystretch.h"
#include "rs_actionmodifytrim.h"
#include "rs_actionmodifytrimamount.h"
#include "rs_actionoptionsdrawing.h"
#include "rs_actionselect.h"
#include "rs_actionselectall.h"
#include "rs_actionselectcontour.h"
#include "rs_actionselectintersected.h"
#include "rs_actionselectinvert.h"
#include "rs_actionselectlayer.h"
#include "rs_actionselectsingle.h"
#include "rs_actionselectwindow.h"
#include "rs_actionsetrelativezero.h"
#include "rs_actionsetsnapmode.h"
#include "rs_actionsetsnaprestriction.h"
#include "rs_actionsnapintersectionmanual.h"
#include "rs_actiontoolregeneratedimensions.h"
#include "rs_actionzoomauto.h"
#include "rs_actionzoomin.h"
#include "rs_actionzoompan.h"
#include "rs_actionzoomprevious.h"
#include "rs_actionzoomredraw.h"
#include "rs_actionzoomwindow.h"

#include "rs_actiondrawpolyline.h"
#include "rs_actionpolylineadd.h"
#include "rs_actionpolylineappend.h"
#include "rs_actionpolylinedel.h"
#include "rs_actionpolylinedelbetween.h"
#include "rs_actionpolylinetrim.h"
#include "rs_selection.h"

#include "qg_mainwindowinterface.h"
#include "qg_cadtoolbarsnap.h"

/**
 * Constructor
 */
QG_ActionHandler::QG_ActionHandler(QG_MainWindowInterface* mw) {
    RS_DEBUG->print("QG_ActionHandler::QG_ActionHandler");
    mainWindow = mw;

    snapFree = NULL;
    snapGrid = NULL;
    snapEndpoint = NULL;
    snapOnEntity = NULL;
    snapCenter = NULL;
    snapMiddle = NULL;
    snapDist = NULL;

    restrictNothing = NULL;
    restrictOrthogonal = NULL;
    restrictHorizontal = NULL;
    restrictVertical = NULL;

    lockRelativeZero = NULL;
    RS_DEBUG->print("QG_ActionHandler::QG_ActionHandler: OK");
}



/**
 * Destructor
 */
QG_ActionHandler::~QG_ActionHandler() {
    RS_DEBUG->print("QG_ActionHandler::~QG_ActionHandler");
    RS_DEBUG->print("QG_ActionHandler::~QG_ActionHandler: OK");
}



/**
 * Kills all running selection actions. Called when a selection action
  * is launched to reduce confusion.
   */
void QG_ActionHandler::killSelectActions() {
    RS_GraphicView* gv = mainWindow->getGraphicView();

    if (gv!=NULL) {
        gv->killSelectActions();
    }
}



/**
 * @return Current action or NULL.
 */
RS_ActionInterface* QG_ActionHandler::getCurrentAction() {
    RS_GraphicView* gv = mainWindow->getGraphicView();

    if (gv!=NULL) {
        return gv->getCurrentAction();
    } else {
        return NULL;
    }
}



/**
 * Sets current action.
 *
 * @return Pointer to the created action or NULL.
 */
RS_ActionInterface* QG_ActionHandler::setCurrentAction(RS2::ActionType id) {
    RS_DEBUG->print("QG_ActionHandler::setCurrentAction()");
	RS_GraphicView* gv = mainWindow->getGraphicView();
    RS_Document* doc = mainWindow->getDocument();
    RS_ActionInterface* a = NULL;

    // only global options are allowed without a document:
    if (gv==NULL || doc==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
        	"QG_ActionHandler::setCurrentAction: graphic view or "
        	"document is NULL");
        return NULL;
    }

    switch (id) {
        //case RS2::ActionFileNew:
        //    a = new RS_ActionFileNew(*doc, *gv);
        //	break;
        //case RS2::ActionFileSave:
        //    a = new RS_ActionFileSave(*doc, *gv);
        //	break;
        //case RS2::ActionFileClose:
        //    //a = new RS_ActionFileClose(*doc, *gv);
        //	break;
        //case RS2::ActionFileQuit:
        //    //a = new RS_ActionFileQuit(*doc, *gv);
        //	break;
    case RS2::ActionFileOpen:
        a = new RS_ActionFileOpen(*doc, *gv);
        break;
    case RS2::ActionFileSaveAs:
        a = new RS_ActionFileSaveAs(*doc, *gv);
        break;

        // Editing actions:
        //
    case RS2::ActionEditKillAllActions:
        if (gv!=NULL) {
            // DO we need to call some form of a 'clean' function?
            gv->killAllActions();
            RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarMain);

            RS_Selection s((RS_EntityContainer&)*doc, gv);
            s.selectAll(false);
            RS_DIALOGFACTORY->updateSelectionWidget(doc->countSelected());
        }
        break;
    case RS2::ActionEditUndo:
        a = new RS_ActionEditUndo(true, *doc, *gv);
        break;
    case RS2::ActionEditRedo:
        a = new RS_ActionEditUndo(false, *doc, *gv);
        break;
    case RS2::ActionEditCut:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionEditCutNoSelect);
        break;
    case RS2::ActionEditCutNoSelect:
        a = new RS_ActionEditCopy(false, *doc, *gv);
        break;
    case RS2::ActionEditCopy:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionEditCopyNoSelect);
        break;
    case RS2::ActionEditCopyNoSelect:
        a = new RS_ActionEditCopy(true, *doc, *gv);
        break;
    case RS2::ActionEditPaste:
        a = new RS_ActionEditPaste(*doc, *gv);
        break;

        // Selecting actions:
        //
    case RS2::ActionSelectSingle:
        gv->killSelectActions();
        a = new RS_ActionSelectSingle(*doc, *gv);
        break;
    case RS2::ActionSelectContour:
        gv->killSelectActions();
        a = new RS_ActionSelectContour(*doc, *gv);
        break;
    case RS2::ActionSelectAll:
        a = new RS_ActionSelectAll(*doc, *gv, true);
        break;
    case RS2::ActionDeselectAll:
        a = new RS_ActionSelectAll(*doc, *gv, false);
        break;
    case RS2::ActionSelectWindow:
        gv->killSelectActions();
        a = new RS_ActionSelectWindow(*doc, *gv, true);
        break;
    case RS2::ActionDeselectWindow:
        gv->killSelectActions();
        a = new RS_ActionSelectWindow(*doc, *gv, false);
        break;
    case RS2::ActionSelectInvert:
        a = new RS_ActionSelectInvert(*doc, *gv);
        break;
    case RS2::ActionSelectIntersected:
        gv->killSelectActions();
        a = new RS_ActionSelectIntersected(*doc, *gv, true);
        break;
    case RS2::ActionDeselectIntersected:
        gv->killSelectActions();
        a = new RS_ActionSelectIntersected(*doc, *gv, false);
        break;
    case RS2::ActionSelectLayer:
        gv->killSelectActions();
        a = new RS_ActionSelectLayer(*doc, *gv);
        break;
		
        // Tool actions:
        //
    case RS2::ActionToolRegenerateDimensions:
        a = new RS_ActionToolRegenerateDimensions(*doc, *gv);
        break;

        // Zooming actions:
        //
    case RS2::ActionZoomIn:
        a = new RS_ActionZoomIn(*doc, *gv, RS2::In, RS2::Both);
        break;
    case RS2::ActionZoomOut:
        a = new RS_ActionZoomIn(*doc, *gv, RS2::Out, RS2::Both);
        break;
    case RS2::ActionZoomAuto:
        a = new RS_ActionZoomAuto(*doc, *gv);
        break;
    case RS2::ActionZoomWindow:
        a = new RS_ActionZoomWindow(*doc, *gv);
        break;
    case RS2::ActionZoomPan:
        a = new RS_ActionZoomPan(*doc, *gv);
        break;
    case RS2::ActionZoomPrevious:
        a = new RS_ActionZoomPrevious(*doc, *gv);
        break;
    case RS2::ActionZoomRedraw:
        a = new RS_ActionZoomRedraw(*doc, *gv);
        break;

        // Drawing actions:
        //
    case RS2::ActionDrawPoint:
        a = new RS_ActionDrawPoint(*doc, *gv);
        break;
    case RS2::ActionDrawLine:
        a = new RS_ActionDrawLine(*doc, *gv);
        break;
    case RS2::ActionDrawLineAngle:
        a = new RS_ActionDrawLineAngle(*doc, *gv, 0.0, false);
        break;
    case RS2::ActionDrawLineHorizontal:
        a = new RS_ActionDrawLineAngle(*doc, *gv, 0.0, true);
        break;
    case RS2::ActionDrawLineHorVert:
        a = new RS_ActionDrawLineHorVert(*doc, *gv);
        break;
    case RS2::ActionDrawLineVertical:
        a = new RS_ActionDrawLineAngle(*doc, *gv, M_PI/2.0, true);
        break;
    case RS2::ActionDrawLineFree:
        a = new RS_ActionDrawLineFree(*doc, *gv);
        break;
    case RS2::ActionDrawLineParallel:
        a = new RS_ActionDrawLineParallel(*doc, *gv);
        break;
    case RS2::ActionDrawLineParallelThrough:
        a = new RS_ActionDrawLineParallelThrough(*doc, *gv);
        break;
    case RS2::ActionDrawLineRectangle:
        a = new RS_ActionDrawLineRectangle(*doc, *gv);
        break;
    case RS2::ActionDrawLineBisector:
        a = new RS_ActionDrawLineBisector(*doc, *gv);
        break;
    case RS2::ActionDrawLineTangent1:
        a = new RS_ActionDrawLineTangent1(*doc, *gv);
        break;
    case RS2::ActionDrawLineTangent2:
        a = new RS_ActionDrawLineTangent2(*doc, *gv);
        break;
    case RS2::ActionDrawLineOrthogonal:
        a = new RS_ActionDrawLineRelAngle(*doc, *gv, M_PI/2.0, true);
        break;
    case RS2::ActionDrawLineRelAngle:
        a = new RS_ActionDrawLineRelAngle(*doc, *gv, M_PI/2.0, false);
        break;
    case RS2::ActionDrawPolyline:
        a = new RS_ActionDrawPolyline(*doc, *gv);
        break;
    case RS2::ActionPolylineAdd:
        a = new RS_ActionPolylineAdd(*doc, *gv);
        break;
    case RS2::ActionPolylineAppend:
        a = new RS_ActionPolylineAppend(*doc, *gv);
        break;
    case RS2::ActionPolylineDel:
        a = new RS_ActionPolylineDel(*doc, *gv);
        break;
    case RS2::ActionPolylineDelBetween:
        a = new RS_ActionPolylineDelBetween(*doc, *gv);
        break;
    case RS2::ActionPolylineTrim:
        a = new RS_ActionPolylineTrim(*doc, *gv);
        break;
    case RS2::ActionDrawLinePolygon:
        a = new RS_ActionDrawLinePolygon(*doc, *gv);
        break;
    case RS2::ActionDrawLinePolygon2:
        a = new RS_ActionDrawLinePolygon2(*doc, *gv);
        break;
    case RS2::ActionDrawCircle:
        a = new RS_ActionDrawCircle(*doc, *gv);
        break;
    case RS2::ActionDrawCircleCR:
        a = new RS_ActionDrawCircleCR(*doc, *gv);
        break;
    case RS2::ActionDrawCircle2P:
        a = new RS_ActionDrawCircle2P(*doc, *gv);
        break;
    case RS2::ActionDrawCircle3P:
        a = new RS_ActionDrawCircle3P(*doc, *gv);
        break;
    case RS2::ActionDrawCircleParallel:
        a = new RS_ActionDrawLineParallel(*doc, *gv);
        break;
    case RS2::ActionDrawArc:
        a = new RS_ActionDrawArc(*doc, *gv);
        break;
    case RS2::ActionDrawArc3P:
        a = new RS_ActionDrawArc3P(*doc, *gv);
        break;
    case RS2::ActionDrawArcParallel:
        a = new RS_ActionDrawLineParallel(*doc, *gv);
        break;
    case RS2::ActionDrawArcTangential:
        a = new RS_ActionDrawArcTangential(*doc, *gv);
        break;
    case RS2::ActionDrawEllipseAxis:
        a = new RS_ActionDrawEllipseAxis(*doc, *gv, false);
        break;
    case RS2::ActionDrawEllipseArcAxis:
        a = new RS_ActionDrawEllipseAxis(*doc, *gv, true);
        break;
    case RS2::ActionDrawSpline:
        a = new RS_ActionDrawSpline(*doc, *gv);
        break;
    case RS2::ActionDrawText:
        a = new RS_ActionDrawText(*doc, *gv);
        break;
    case RS2::ActionDrawHatch:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionDrawHatchNoSelect);
        break;
    case RS2::ActionDrawHatchNoSelect:
        a = new RS_ActionDrawHatch(*doc, *gv);
        break;
    case RS2::ActionDrawImage:
        a = new RS_ActionDrawImage(*doc, *gv);
        break;

        // Dimensioning actions:
        //
    case RS2::ActionDimAligned:
        a = new RS_ActionDimAligned(*doc, *gv);
        break;
    case RS2::ActionDimLinear:
        a = new RS_ActionDimLinear(*doc, *gv);
        break;
    case RS2::ActionDimLinearHor:
        a = new RS_ActionDimLinear(*doc, *gv, 0.0, true);
        break;
    case RS2::ActionDimLinearVer:
        a = new RS_ActionDimLinear(*doc, *gv, M_PI/2.0, true);
        break;
    case RS2::ActionDimRadial:
        a = new RS_ActionDimRadial(*doc, *gv);
        break;
    case RS2::ActionDimDiametric:
        a = new RS_ActionDimDiametric(*doc, *gv);
        break;
    case RS2::ActionDimAngular:
        a = new RS_ActionDimAngular(*doc, *gv);
        break;
    case RS2::ActionDimLeader:
        a = new RS_ActionDimLeader(*doc, *gv);
        break;

        // Modifying actions:
        //
    case RS2::ActionModifyAttributes:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyAttributesNoSelect);
        break;
    case RS2::ActionModifyAttributesNoSelect:
        a = new RS_ActionModifyAttributes(*doc, *gv);
        break;
    case RS2::ActionModifyDelete:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyDeleteNoSelect);
        break;
    case RS2::ActionModifyDeleteNoSelect:
        a = new RS_ActionModifyDelete(*doc, *gv);
        break;
    case RS2::ActionModifyDeleteQuick:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyDeleteQuick);
        break;
    case RS2::ActionModifyDeleteFree:
        a = new RS_ActionModifyDeleteFree(*doc, *gv);
        break;
    case RS2::ActionModifyMove:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyMoveNoSelect);
        break;
    case RS2::ActionModifyMoveNoSelect:
        a = new RS_ActionModifyMove(*doc, *gv);
        break;
    case RS2::ActionModifyRotate:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyRotateNoSelect);
        break;
    case RS2::ActionModifyRotateNoSelect:
        a = new RS_ActionModifyRotate(*doc, *gv);
        break;
    case RS2::ActionModifyScale:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyScaleNoSelect);
        break;
    case RS2::ActionModifyScaleNoSelect:
        a = new RS_ActionModifyScale(*doc, *gv);
        break;
    case RS2::ActionModifyMirror:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyMirrorNoSelect);
        break;
    case RS2::ActionModifyMirrorNoSelect:
        a = new RS_ActionModifyMirror(*doc, *gv);
        break;
    case RS2::ActionModifyMoveRotate:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyMoveRotateNoSelect);
        break;
    case RS2::ActionModifyMoveRotateNoSelect:
        a = new RS_ActionModifyMoveRotate(*doc, *gv);
        break;
    case RS2::ActionModifyRotate2:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyRotate2NoSelect);
        break;
    case RS2::ActionModifyRotate2NoSelect:
        a = new RS_ActionModifyRotate2(*doc, *gv);
        break;
    case RS2::ActionModifyEntity:
        a = new RS_ActionModifyEntity(*doc, *gv);
        break;
    case RS2::ActionModifyTrim:
        a = new RS_ActionModifyTrim(*doc, *gv, false);
        break;
    case RS2::ActionModifyTrim2:
        a = new RS_ActionModifyTrim(*doc, *gv, true);
        break;
    case RS2::ActionModifyTrimAmount:
        a = new RS_ActionModifyTrimAmount(*doc, *gv);
        break;
    case RS2::ActionModifyCut:
        a = new RS_ActionModifyCut(*doc, *gv);
        break;
    case RS2::ActionModifyStretch:
        a = new RS_ActionModifyStretch(*doc, *gv);
        break;
    case RS2::ActionModifyBevel:
        a = new RS_ActionModifyBevel(*doc, *gv);
        break;
    case RS2::ActionModifyRound:
        a = new RS_ActionModifyRound(*doc, *gv);
        break;
    case RS2::ActionModifyExplodeText:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionModifyExplodeTextNoSelect);
        break;
    case RS2::ActionModifyExplodeTextNoSelect:
        a = new RS_ActionModifyExplodeText(*doc, *gv);
        break;

        // Snapping actions:
        //
    case RS2::ActionSnapFree:
        a = new RS_ActionSetSnapMode(*doc, *gv, RS2::SnapFree);
        break;
    case RS2::ActionSnapGrid:
        a = new RS_ActionSetSnapMode(*doc, *gv, RS2::SnapGrid);
        break;
    case RS2::ActionSnapEndpoint:
        a = new RS_ActionSetSnapMode(*doc, *gv, RS2::SnapEndpoint);
        break;
    case RS2::ActionSnapOnEntity:
        a = new RS_ActionSetSnapMode(*doc, *gv, RS2::SnapOnEntity);
        break;
    case RS2::ActionSnapCenter:
        a = new RS_ActionSetSnapMode(*doc, *gv, RS2::SnapCenter);
        break;
    case RS2::ActionSnapMiddle:
        a = new RS_ActionSetSnapMode(*doc, *gv, RS2::SnapMiddle);
        break;
    case RS2::ActionSnapDist:
        a = new RS_ActionSetSnapMode(*doc, *gv, RS2::SnapDist);
        break;
    case RS2::ActionSnapIntersection:
        a = new RS_ActionSetSnapMode(*doc, *gv, RS2::SnapIntersection);
        break;
    case RS2::ActionSnapIntersectionManual:
        a = new RS_ActionSnapIntersectionManual(*doc, *gv);
        break;

        // Snap restriction actions:
        //
    case RS2::ActionRestrictNothing:
        a = new RS_ActionSetSnapRestriction(*doc, *gv, RS2::RestrictNothing);
        break;
    case RS2::ActionRestrictOrthogonal:
        a = new RS_ActionSetSnapRestriction(*doc, *gv,
                                            RS2::RestrictOrthogonal);
        break;
    case RS2::ActionRestrictHorizontal:
        a = new RS_ActionSetSnapRestriction(*doc, *gv,
                                            RS2::RestrictHorizontal);
        break;
    case RS2::ActionRestrictVertical:
        a = new RS_ActionSetSnapRestriction(*doc, *gv,
                                            RS2::RestrictVertical);
        break;

        // Relative zero:
        //
    case RS2::ActionSetRelativeZero:
        a = new RS_ActionSetRelativeZero(*doc, *gv);
        break;
    case RS2::ActionLockRelativeZero:
        a = new RS_ActionLockRelativeZero(*doc, *gv, true);
        break;
    case RS2::ActionUnlockRelativeZero:
        a = new RS_ActionLockRelativeZero(*doc, *gv, false);
        break;

        // Info actions:
        //
    case RS2::ActionInfoInside:
        a = new RS_ActionInfoInside(*doc, *gv);
        break;
    case RS2::ActionInfoDist:
        a = new RS_ActionInfoDist(*doc, *gv);
        break;
    case RS2::ActionInfoDist2:
        a = new RS_ActionInfoDist2(*doc, *gv);
        break;
    case RS2::ActionInfoAngle:
        a = new RS_ActionInfoAngle(*doc, *gv);
        break;
    case RS2::ActionInfoTotalLength:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionInfoTotalLengthNoSelect);
        break;
    case RS2::ActionInfoTotalLengthNoSelect:
        a = new RS_ActionInfoTotalLength(*doc, *gv);
        break;
    case RS2::ActionInfoArea:
        a = new RS_ActionInfoArea(*doc, *gv);
        break;

        // Layer actions:
        //
    case RS2::ActionLayersDefreezeAll:
        a = new RS_ActionLayersFreezeAll(false, *doc, *gv);
        break;
    case RS2::ActionLayersFreezeAll:
        a = new RS_ActionLayersFreezeAll(true, *doc, *gv);
        break;
    case RS2::ActionLayersAdd:
        a = new RS_ActionLayersAdd(*doc, *gv);
        break;
    case RS2::ActionLayersRemove:
        a = new RS_ActionLayersRemove(*doc, *gv);
        break;
    case RS2::ActionLayersEdit:
        a = new RS_ActionLayersEdit(*doc, *gv);
        break;
    case RS2::ActionLayersToggleView:
        a = new RS_ActionLayersToggleView(*doc, *gv);
        break;
    case RS2::ActionLayersToggleLock:
        a = new RS_ActionLayersToggleLock(*doc, *gv);
        break;

        // Block actions:
        //
    case RS2::ActionBlocksDefreezeAll:
        a = new RS_ActionBlocksFreezeAll(false, *doc, *gv);
        break;
    case RS2::ActionBlocksFreezeAll:
        a = new RS_ActionBlocksFreezeAll(true, *doc, *gv);
        break;
    case RS2::ActionBlocksAdd:
        a = new RS_ActionBlocksAdd(*doc, *gv);
        break;
    case RS2::ActionBlocksRemove:
        a = new RS_ActionBlocksRemove(*doc, *gv);
        break;
    case RS2::ActionBlocksAttributes:
        a = new RS_ActionBlocksAttributes(*doc, *gv);
        break;
    case RS2::ActionBlocksEdit:
        a = new RS_ActionBlocksEdit(*doc, *gv);
        break;
    case RS2::ActionBlocksInsert:
        a = new RS_ActionBlocksInsert(*doc, *gv);
        break;
    case RS2::ActionBlocksToggleView:
        a = new RS_ActionBlocksToggleView(*doc, *gv);
        break;
    case RS2::ActionBlocksCreate:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionBlocksCreateNoSelect);
        break;
    case RS2::ActionBlocksCreateNoSelect:
        a = new RS_ActionBlocksCreate(*doc, *gv);
        break;
    case RS2::ActionBlocksExplode:
        a = new RS_ActionSelect(*doc, *gv, RS2::ActionBlocksExplodeNoSelect);
        break;
    case RS2::ActionBlocksExplodeNoSelect:
        a = new RS_ActionBlocksExplode(*doc, *gv);
        break;
		

        // library browser:
        //
    case RS2::ActionLibraryInsert:
        a = new RS_ActionLibraryInsert(*doc, *gv);
        break;

        // options:
        //
        //case RS2::ActionOptionsGeneral:
        //    a = new RS_ActionOptionsGeneral(*doc, *gv);
        //	break;

    case RS2::ActionOptionsDrawing:
        a = new RS_ActionOptionsDrawing(*doc, *gv);
        break;
    default:
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QG_ActionHandler::setCurrentAction():"
                        "No such action found.");
        break;
    }

    if (a!=NULL) {
        gv->setCurrentAction(a);
    }
	
    RS_DEBUG->print("QG_ActionHandler::setCurrentAction(): OK");
    return a;
}



/**
 * @return Available commands of the application or the current action.
 */
RS_StringList QG_ActionHandler::getAvailableCommands() {
    RS_ActionInterface* currentAction = getCurrentAction();

    if (currentAction!=NULL) {
        return currentAction->getAvailableCommands();
    } else {
        RS_StringList cmd;
        cmd += "line";
        cmd += "rectangle";
        return cmd;
    }
}



/**
 * Launches the command represented by the given keycode if possible.
 *
 * @return true: the command was recognized. 
 *         false: the command is not known and was probably intended for a
 *            running action.
 */
bool QG_ActionHandler::keycode(const QString& code) {
    QString c = code.lower();

    // pass keycode on to running action:
    //RS_keycodeEvent e(cmd);

    //RS_GraphicView* gv = mainWindow->getGraphicView();
    //if (gv!=NULL) {
    //    gv->keycodeEvent(&e);
    //}

    // if the current action can't deal with the keycode,
    //   it might be intended to launch a new keycode
    //if (!e.isAccepted()) {
    // keycode for new action:
    RS2::ActionType type = RS_COMMANDS->keycodeToAction(code);
    if (type!=RS2::ActionNone) {
        // some actions require special handling (GUI update):
        switch (type) {
        case RS2::ActionSnapFree:
            slotSnapFree();
            break;
        case RS2::ActionSnapGrid:
            slotSnapGrid();
            break;
        case RS2::ActionSnapEndpoint:
            slotSnapEndpoint();
            break;
        case RS2::ActionSnapOnEntity:
            slotSnapOnEntity();
            break;
        case RS2::ActionSnapCenter:
            slotSnapCenter();
            break;
        case RS2::ActionSnapMiddle:
            slotSnapMiddle();
            break;
        case RS2::ActionSnapDist:
            slotSnapDist();
            break;
        case RS2::ActionSnapIntersection:
            slotSnapIntersection();
            break;
        case RS2::ActionSnapIntersectionManual:
            slotSnapIntersectionManual();
            break;

        case RS2::ActionRestrictNothing:
            slotRestrictNothing();
            break;
        case RS2::ActionRestrictOrthogonal:
            slotRestrictOrthogonal();
            break;
        case RS2::ActionRestrictHorizontal:
            slotRestrictHorizontal();
            break;
        case RS2::ActionRestrictVertical:
            slotRestrictVertical();
            break;

        default:
            setCurrentAction(type);
            break;
        }
        return true;
    }
    //}

    return false;
}



/**
 * Launches the given command if possible.
 *
 * @return true: the command was recognized. 
 *         false: the command is not known and was probably intended for a
 *            running action.
 */
bool QG_ActionHandler::command(const QString& cmd) {
	RS_DEBUG->print("QG_ActionHandler::command: %s", cmd.latin1());
    QString c = cmd.lower();

    if (c=="\n") {
        RS_GraphicView* gv = mainWindow->getGraphicView();
        if (gv!=NULL) {
            gv->back();
        }
		RS_DEBUG->print("QG_ActionHandler::command: back");
        return true;
    }

    // pass command on to running action:
    RS_CommandEvent e(cmd);

    RS_GraphicView* gv = mainWindow->getGraphicView();
    if (gv!=NULL) {
		RS_DEBUG->print("QG_ActionHandler::command: trigger command event in "
		" graphic view");
        gv->commandEvent(&e);
    }

    // if the current action can't deal with the command,
    //   it might be intended to launch a new command
    if (!e.isAccepted()) {
		RS_DEBUG->print("QG_ActionHandler::command: convert cmd to action type");
        // command for new action:
        RS2::ActionType type = RS_COMMANDS->cmdToAction(cmd);
        if (type!=RS2::ActionNone) {
			RS_DEBUG->print("QG_ActionHandler::command: setting current action");
            setCurrentAction(type);
			RS_DEBUG->print("QG_ActionHandler::command: current action set");
            return true;
        }
    }

	RS_DEBUG->print("QG_ActionHandler::command: current action not set");
    return false;
}




//void QG_ActionHandler::slotFileNew() {
//	setCurrentAction(RS2::ActionFileNew);
//}

void QG_ActionHandler::slotFileOpen() {
    setCurrentAction(RS2::ActionFileOpen);
}
/*
void QG_ActionHandler::slotFileSave() {
	setCurrentAction(RS2::ActionFileSave);
}
*/

void QG_ActionHandler::slotFileSaveAs() {
    setCurrentAction(RS2::ActionFileSaveAs);
}

/*
void QG_ActionHandler::slotFileClose() {
	setCurrentAction(RS2::ActionFileClose);
}
 
void QG_ActionHandler::slotFilePrint() {
	setCurrentAction(RS2::ActionFilePrint);
}
*/

void QG_ActionHandler::slotZoomIn() {
    setCurrentAction(RS2::ActionZoomIn);
}

void QG_ActionHandler::slotZoomOut() {
    setCurrentAction(RS2::ActionZoomOut);
}

void QG_ActionHandler::slotZoomAuto() {
    setCurrentAction(RS2::ActionZoomAuto);
}

void QG_ActionHandler::slotZoomWindow() {
    setCurrentAction(RS2::ActionZoomWindow);
}

void QG_ActionHandler::slotZoomPan() {
    setCurrentAction(RS2::ActionZoomPan);
}

void QG_ActionHandler::slotZoomPrevious() {
    setCurrentAction(RS2::ActionZoomPrevious);
}

void QG_ActionHandler::slotZoomRedraw() {
    setCurrentAction(RS2::ActionZoomRedraw);
}

void QG_ActionHandler::slotToolRegenerateDimensions() {
    setCurrentAction(RS2::ActionToolRegenerateDimensions);
}

void QG_ActionHandler::slotEditKillAllActions() {
    setCurrentAction(RS2::ActionEditKillAllActions);
}
void QG_ActionHandler::slotEditUndo() {
    setCurrentAction(RS2::ActionEditUndo);
}

void QG_ActionHandler::slotEditRedo() {
    setCurrentAction(RS2::ActionEditRedo);
}

void QG_ActionHandler::slotEditCut() {
    setCurrentAction(RS2::ActionEditCut);
}

void QG_ActionHandler::slotEditCopy() {
    setCurrentAction(RS2::ActionEditCopy);
}

void QG_ActionHandler::slotEditPaste() {
    setCurrentAction(RS2::ActionEditPaste);
}

void QG_ActionHandler::slotSelectSingle() {
    setCurrentAction(RS2::ActionSelectSingle);
}

void QG_ActionHandler::slotSelectContour() {
    setCurrentAction(RS2::ActionSelectContour);
}

void QG_ActionHandler::slotSelectWindow() {
    setCurrentAction(RS2::ActionSelectWindow);
}

void QG_ActionHandler::slotDeselectWindow() {
    setCurrentAction(RS2::ActionDeselectWindow);
}

void QG_ActionHandler::slotSelectAll() {
    setCurrentAction(RS2::ActionSelectAll);
}

void QG_ActionHandler::slotDeselectAll() {
    setCurrentAction(RS2::ActionDeselectAll);
}

void QG_ActionHandler::slotSelectInvert() {
    setCurrentAction(RS2::ActionSelectInvert);
}

void QG_ActionHandler::slotSelectIntersected() {
    setCurrentAction(RS2::ActionSelectIntersected);
}

void QG_ActionHandler::slotDeselectIntersected() {
    setCurrentAction(RS2::ActionDeselectIntersected);
}

void QG_ActionHandler::slotSelectLayer() {
    setCurrentAction(RS2::ActionSelectLayer);
}

void QG_ActionHandler::slotDrawPoint() {
    setCurrentAction(RS2::ActionDrawPoint);
}

void QG_ActionHandler::slotDrawLine() {
    setCurrentAction(RS2::ActionDrawLine);
}

void QG_ActionHandler::slotDrawLineAngle() {
    setCurrentAction(RS2::ActionDrawLineAngle);
}

void QG_ActionHandler::slotDrawLineHorizontal() {
    setCurrentAction(RS2::ActionDrawLineHorizontal);
}

void QG_ActionHandler::slotDrawLineHorVert() {
    setCurrentAction(RS2::ActionDrawLineHorVert);
}

void QG_ActionHandler::slotDrawLineVertical() {
    setCurrentAction(RS2::ActionDrawLineVertical);
}

void QG_ActionHandler::slotDrawLineFree() {
    setCurrentAction(RS2::ActionDrawLineFree);
}

void QG_ActionHandler::slotDrawLineParallel() {
    setCurrentAction(RS2::ActionDrawLineParallel);
}

void QG_ActionHandler::slotDrawLineParallelThrough() {
    setCurrentAction(RS2::ActionDrawLineParallelThrough);
}

void QG_ActionHandler::slotDrawLineRectangle() {
    setCurrentAction(RS2::ActionDrawLineRectangle);
}

void QG_ActionHandler::slotDrawLineBisector() {
    setCurrentAction(RS2::ActionDrawLineBisector);
}

void QG_ActionHandler::slotDrawLineTangent1() {
    setCurrentAction(RS2::ActionDrawLineTangent1);
}

void QG_ActionHandler::slotDrawLineTangent2() {
    setCurrentAction(RS2::ActionDrawLineTangent2);
}

void QG_ActionHandler::slotDrawLineOrthogonal() {
    setCurrentAction(RS2::ActionDrawLineOrthogonal);
}

void QG_ActionHandler::slotDrawLineRelAngle() {
    setCurrentAction(RS2::ActionDrawLineRelAngle);
}

void QG_ActionHandler::slotDrawPolyline() {
    setCurrentAction(RS2::ActionDrawPolyline);
}

void QG_ActionHandler::slotPolylineAdd() {
    setCurrentAction(RS2::ActionPolylineAdd);
}

void QG_ActionHandler::slotPolylineAppend() {
    setCurrentAction(RS2::ActionPolylineAppend);
}

void QG_ActionHandler::slotPolylineDel() {
    setCurrentAction(RS2::ActionPolylineDel);
}

void QG_ActionHandler::slotPolylineDelBetween() {
    setCurrentAction(RS2::ActionPolylineDelBetween);
}

void QG_ActionHandler::slotPolylineTrim() {
    setCurrentAction(RS2::ActionPolylineTrim);
}

void QG_ActionHandler::slotDrawLinePolygon() {
    setCurrentAction(RS2::ActionDrawLinePolygon);
}

void QG_ActionHandler::slotDrawLinePolygon2() {
    setCurrentAction(RS2::ActionDrawLinePolygon2);
}

void QG_ActionHandler::slotDrawCircle() {
    setCurrentAction(RS2::ActionDrawCircle);
}

void QG_ActionHandler::slotDrawCircleCR() {
    setCurrentAction(RS2::ActionDrawCircleCR);
}

void QG_ActionHandler::slotDrawCircle2P() {
    setCurrentAction(RS2::ActionDrawCircle2P);
}

void QG_ActionHandler::slotDrawCircle3P() {
    setCurrentAction(RS2::ActionDrawCircle3P);
}

void QG_ActionHandler::slotDrawCircleParallel() {
    setCurrentAction(RS2::ActionDrawCircleParallel);
}

void QG_ActionHandler::slotDrawArc() {
    setCurrentAction(RS2::ActionDrawArc);
}

void QG_ActionHandler::slotDrawArc3P() {
    setCurrentAction(RS2::ActionDrawArc3P);
}

void QG_ActionHandler::slotDrawArcParallel() {
    setCurrentAction(RS2::ActionDrawArcParallel);
}

void QG_ActionHandler::slotDrawArcTangential() {
    setCurrentAction(RS2::ActionDrawArcTangential);
}

void QG_ActionHandler::slotDrawEllipseAxis() {
    setCurrentAction(RS2::ActionDrawEllipseAxis);
}

void QG_ActionHandler::slotDrawEllipseArcAxis() {
    setCurrentAction(RS2::ActionDrawEllipseArcAxis);
}

void QG_ActionHandler::slotDrawSpline() {
    setCurrentAction(RS2::ActionDrawSpline);
}

void QG_ActionHandler::slotDrawText() {
    setCurrentAction(RS2::ActionDrawText);
}

void QG_ActionHandler::slotDrawHatch() {
    setCurrentAction(RS2::ActionDrawHatch);
}

void QG_ActionHandler::slotDrawImage() {
    setCurrentAction(RS2::ActionDrawImage);
}

void QG_ActionHandler::slotDimAligned() {
    setCurrentAction(RS2::ActionDimAligned);
}

void QG_ActionHandler::slotDimLinear() {
    setCurrentAction(RS2::ActionDimLinear);
}

void QG_ActionHandler::slotDimLinearHor() {
    setCurrentAction(RS2::ActionDimLinearHor);
}

void QG_ActionHandler::slotDimLinearVer() {
    setCurrentAction(RS2::ActionDimLinearVer);
}

void QG_ActionHandler::slotDimRadial() {
    setCurrentAction(RS2::ActionDimRadial);
}

void QG_ActionHandler::slotDimDiametric() {
    setCurrentAction(RS2::ActionDimDiametric);
}

void QG_ActionHandler::slotDimAngular() {
    setCurrentAction(RS2::ActionDimAngular);
}

void QG_ActionHandler::slotDimLeader() {
    setCurrentAction(RS2::ActionDimLeader);
}


void QG_ActionHandler::slotModifyAttributes() {
    setCurrentAction(RS2::ActionModifyAttributes);
}

void QG_ActionHandler::slotModifyDelete() {
    setCurrentAction(RS2::ActionModifyDelete);
}

void QG_ActionHandler::slotModifyDeleteQuick() {
    //setCurrentAction(RS2::ActionModifyDeleteQuick);
    setCurrentAction(RS2::ActionModifyDeleteNoSelect);
}

void QG_ActionHandler::slotModifyDeleteFree() {
    setCurrentAction(RS2::ActionModifyDeleteFree);
}

void QG_ActionHandler::slotModifyMove() {
    setCurrentAction(RS2::ActionModifyMove);
}

void QG_ActionHandler::slotModifyRotate() {
    setCurrentAction(RS2::ActionModifyRotate);
}

void QG_ActionHandler::slotModifyScale() {
    setCurrentAction(RS2::ActionModifyScale);
}

void QG_ActionHandler::slotModifyStretch() {
    setCurrentAction(RS2::ActionModifyStretch);
}

void QG_ActionHandler::slotModifyBevel() {
    setCurrentAction(RS2::ActionModifyBevel);
}

void QG_ActionHandler::slotModifyRound() {
    setCurrentAction(RS2::ActionModifyRound);
}

void QG_ActionHandler::slotModifyMirror() {
    setCurrentAction(RS2::ActionModifyMirror);
}

void QG_ActionHandler::slotModifyMoveRotate() {
    setCurrentAction(RS2::ActionModifyMoveRotate);
}

void QG_ActionHandler::slotModifyRotate2() {
    setCurrentAction(RS2::ActionModifyRotate2);
}

void QG_ActionHandler::slotModifyEntity() {
    setCurrentAction(RS2::ActionModifyEntity);
}

void QG_ActionHandler::slotModifyTrim() {
    setCurrentAction(RS2::ActionModifyTrim);
}

void QG_ActionHandler::slotModifyTrim2() {
    setCurrentAction(RS2::ActionModifyTrim2);
}

void QG_ActionHandler::slotModifyTrimAmount() {
    setCurrentAction(RS2::ActionModifyTrimAmount);
}

void QG_ActionHandler::slotModifyCut() {
    setCurrentAction(RS2::ActionModifyCut);
}

void QG_ActionHandler::slotModifyExplodeText() {
    setCurrentAction(RS2::ActionModifyExplodeText);
}


void QG_ActionHandler::slotSnapFree() {
    disableSnaps();
    if (snapFree!=NULL) {
        snapFree->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapFree);
    }
    setCurrentAction(RS2::ActionSnapFree);
}

void QG_ActionHandler::slotSnapGrid() {
    disableSnaps();
    if (snapGrid!=NULL) {
        snapGrid->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapGrid);
    }
    setCurrentAction(RS2::ActionSnapGrid);
}

void QG_ActionHandler::slotSnapEndpoint() {
    disableSnaps();
    if (snapEndpoint!=NULL) {
        snapEndpoint->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapEndpoint);
    }
    setCurrentAction(RS2::ActionSnapEndpoint);
}

void QG_ActionHandler::slotSnapOnEntity() {
    disableSnaps();
    if (snapOnEntity!=NULL) {
        snapOnEntity->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapOnEntity);
    }
    setCurrentAction(RS2::ActionSnapOnEntity);
}

void QG_ActionHandler::slotSnapCenter() {
    disableSnaps();
    if (snapCenter!=NULL) {
        snapCenter->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapCenter);
    }
    setCurrentAction(RS2::ActionSnapCenter);
}

void QG_ActionHandler::slotSnapMiddle() {
    disableSnaps();
    if (snapMiddle!=NULL) {
        snapMiddle->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapMiddle);
    }
    setCurrentAction(RS2::ActionSnapMiddle);
}

void QG_ActionHandler::slotSnapDist() {
    disableSnaps();
    if (snapDist!=NULL) {
        snapDist->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapDist);
    }
    setCurrentAction(RS2::ActionSnapDist);
}

void QG_ActionHandler::slotSnapIntersection() {
    disableSnaps();
    if (snapIntersection!=NULL) {
        snapIntersection->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapIntersection);
    }
    setCurrentAction(RS2::ActionSnapIntersection);
}

void QG_ActionHandler::slotSnapIntersectionManual() {
    //disableSnaps();
    /*if (snapIntersectionManual!=NULL) {
        snapIntersectionManual->setChecked(true);
}*/
    /*if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapMode(RS2::SnapIntersectionManual);
}*/
    setCurrentAction(RS2::ActionSnapIntersectionManual);
}

void QG_ActionHandler::disableSnaps() {
    if (snapFree!=NULL) {
        snapFree->setChecked(false);
    }
    if (snapGrid!=NULL) {
        snapGrid->setChecked(false);
    }
    if (snapEndpoint!=NULL) {
        snapEndpoint->setChecked(false);
    }
    if (snapOnEntity!=NULL) {
        snapOnEntity->setChecked(false);
    }
    if (snapCenter!=NULL) {
        snapCenter->setChecked(false);
    }
    if (snapMiddle!=NULL) {
        snapMiddle->setChecked(false);
    }
    if (snapDist!=NULL) {
        snapDist->setChecked(false);
    }
    if (snapIntersection!=NULL) {
        snapIntersection->setChecked(false);
    }
    if (snapIntersectionManual!=NULL) {
        snapIntersectionManual->setChecked(false);
    }

    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->disableSnaps();
    }
}

void QG_ActionHandler::slotRestrictNothing() {
    disableRestrictions();
    if (restrictNothing!=NULL) {
        restrictNothing->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapRestriction(RS2::RestrictNothing);
    }
    setCurrentAction(RS2::ActionRestrictNothing);
}

void QG_ActionHandler::slotRestrictOrthogonal() {
    disableRestrictions();
    if (restrictOrthogonal!=NULL) {
        restrictOrthogonal->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapRestriction(RS2::RestrictOrthogonal);
    }
    setCurrentAction(RS2::ActionRestrictOrthogonal);
}

void QG_ActionHandler::slotRestrictHorizontal() {
    disableRestrictions();
    if (restrictHorizontal!=NULL) {
        restrictHorizontal->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapRestriction(RS2::RestrictHorizontal);
    }
    setCurrentAction(RS2::ActionRestrictHorizontal);
}

void QG_ActionHandler::slotRestrictVertical() {
    disableRestrictions();
    if (restrictVertical!=NULL) {
        restrictVertical->setChecked(true);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setSnapRestriction(RS2::RestrictVertical);
    }
    setCurrentAction(RS2::ActionRestrictVertical);
}

void QG_ActionHandler::disableRestrictions() {
    if (restrictNothing!=NULL) {
        restrictNothing->setChecked(false);
    }
    if (restrictOrthogonal!=NULL) {
        restrictOrthogonal->setChecked(false);
    }
    if (restrictHorizontal!=NULL) {
        restrictHorizontal->setChecked(false);
    }
    if (restrictVertical!=NULL) {
        restrictVertical->setChecked(false);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->disableRestrictions();
    }
}


/**
 * Updates the snap mode for the current document from the selected menu.
 * Used after the active window changed.
 */
void QG_ActionHandler::updateSnapMode() {
    if (snapFree!=NULL && snapFree->isChecked()) {
        slotSnapFree();
    } else if (snapGrid!=NULL && snapGrid->isChecked()) {
        slotSnapGrid();
    } else if (snapEndpoint!=NULL && snapEndpoint->isChecked()) {
        slotSnapEndpoint();
    } else if (snapOnEntity!=NULL && snapOnEntity->isChecked()) {
        slotSnapOnEntity();
    } else if (snapCenter!=NULL && snapCenter->isChecked()) {
        slotSnapCenter();
    } else if (snapMiddle!=NULL && snapMiddle->isChecked()) {
        slotSnapMiddle();
    } else if (snapDist!=NULL && snapDist->isChecked()) {
        slotSnapDist();
    } else if (snapIntersection!=NULL && snapIntersection->isChecked()) {
        slotSnapIntersection();
    }

    // snap restricitons:
    if (restrictNothing!=NULL && restrictNothing->isChecked()) {
        slotRestrictNothing();
    } else if (restrictOrthogonal!=NULL && restrictOrthogonal->isChecked()) {
        slotRestrictOrthogonal();
    } else if (restrictHorizontal!=NULL && restrictHorizontal->isChecked()) {
        slotRestrictHorizontal();
    } else if (restrictVertical!=NULL && restrictVertical->isChecked()) {
        slotRestrictVertical();
    }

    // lock of relative zero:
    if (lockRelativeZero!=NULL) {
        slotLockRelativeZero(lockRelativeZero->isChecked());
    }
}

void QG_ActionHandler::slotSetRelativeZero() {
    setCurrentAction(RS2::ActionSetRelativeZero);
}

void QG_ActionHandler::slotLockRelativeZero(bool on) {
    if (lockRelativeZero!=NULL) {
        lockRelativeZero->setChecked(on);
    }
    if (on) {
        setCurrentAction(RS2::ActionLockRelativeZero);
    } else {
        setCurrentAction(RS2::ActionUnlockRelativeZero);
    }
    if (cadToolBarSnap!=NULL) {
        cadToolBarSnap->setLockRelativeZero(on);
    }
}

void QG_ActionHandler::slotInfoInside() {
    setCurrentAction(RS2::ActionInfoInside);
}

void QG_ActionHandler::slotInfoDist() {
    setCurrentAction(RS2::ActionInfoDist);
}

void QG_ActionHandler::slotInfoDist2() {
    setCurrentAction(RS2::ActionInfoDist2);
}

void QG_ActionHandler::slotInfoAngle() {
    setCurrentAction(RS2::ActionInfoAngle);
}

void QG_ActionHandler::slotInfoTotalLength() {
    setCurrentAction(RS2::ActionInfoTotalLength);
}

void QG_ActionHandler::slotInfoArea() {
    setCurrentAction(RS2::ActionInfoArea);
}

void QG_ActionHandler::slotLayersDefreezeAll() {
    setCurrentAction(RS2::ActionLayersDefreezeAll);
}

void QG_ActionHandler::slotLayersFreezeAll() {
    setCurrentAction(RS2::ActionLayersFreezeAll);
}

void QG_ActionHandler::slotLayersAdd() {
    setCurrentAction(RS2::ActionLayersAdd);
}

void QG_ActionHandler::slotLayersRemove() {
    setCurrentAction(RS2::ActionLayersRemove);
}

void QG_ActionHandler::slotLayersEdit() {
    setCurrentAction(RS2::ActionLayersEdit);
}

void QG_ActionHandler::slotLayersToggleView() {
    setCurrentAction(RS2::ActionLayersToggleView);
}

void QG_ActionHandler::slotLayersToggleLock() {
    setCurrentAction(RS2::ActionLayersToggleLock);
}


void QG_ActionHandler::slotBlocksDefreezeAll() {
    setCurrentAction(RS2::ActionBlocksDefreezeAll);
}

void QG_ActionHandler::slotBlocksFreezeAll() {
    setCurrentAction(RS2::ActionBlocksFreezeAll);
}

void QG_ActionHandler::slotBlocksAdd() {
    setCurrentAction(RS2::ActionBlocksAdd);
}

void QG_ActionHandler::slotBlocksRemove() {
    setCurrentAction(RS2::ActionBlocksRemove);
}

void QG_ActionHandler::slotBlocksAttributes() {
    setCurrentAction(RS2::ActionBlocksAttributes);
}

void QG_ActionHandler::slotBlocksEdit() {
    setCurrentAction(RS2::ActionBlocksEdit);
}

void QG_ActionHandler::slotBlocksInsert() {
    setCurrentAction(RS2::ActionBlocksInsert);
}

void QG_ActionHandler::slotBlocksToggleView() {
    setCurrentAction(RS2::ActionBlocksToggleView);
}

void QG_ActionHandler::slotBlocksCreate() {
    setCurrentAction(RS2::ActionBlocksCreate);
}

void QG_ActionHandler::slotBlocksExplode() {
    setCurrentAction(RS2::ActionBlocksExplode);
}


void QG_ActionHandler::slotOptionsDrawing() {
    setCurrentAction(RS2::ActionOptionsDrawing);
}

void QG_ActionHandler::slotFocusNormal() {
    //QG_GraphicView* gv = mainWindow->getGraphicView();
    //if (gv!=NULL) {
        //gv->setFocus();
        mainWindow->setFocus2();
    //}
}

/**
    * Creates link to snap tool bar so we can update the button
    * state if the snapping action changes.
    */
void QG_ActionHandler::setCadToolBarSnap(QG_CadToolBarSnap* tb) {
    cadToolBarSnap = tb;
}

// EOF
