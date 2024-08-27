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

#include<cmath>

#include <QDockWidget>

#include "qg_actionhandler.h"

#include "qc_applicationwindow.h"

#include "lc_actiondrawparabola4points.h"
#include "lc_actiondrawparabolaFD.h"

#include "rs_dialogfactory.h"
#include "rs_commandevent.h"
#include "rs_commands.h"

#include "rs_actionblocksadd.h"
#include "rs_actionblocksattributes.h"
#include "rs_actionblockscreate.h"
#include "rs_actionblocksedit.h"
#include "rs_actionblockssave.h"
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
#include "lc_actiondimarc.h"
#include "rs_actiondrawarc.h"
#include "rs_actiondrawarc3p.h"
#include "rs_actiondrawarctangential.h"
#include "rs_actiondrawcircle.h"
#include "rs_actiondrawcircle2p.h"
#include "lc_actiondrawcircle2pr.h"
#include "rs_actiondrawcircle3p.h"
#include "rs_actiondrawcircletan1_2p.h"
#include "rs_actiondrawcircletan2_1p.h"
#include "rs_actiondrawcirclecr.h"
#include "rs_actiondrawcircleinscribe.h"
#include "rs_actiondrawcircletan2.h"
#include "rs_actiondrawcircletan3.h"
#include "rs_actiondrawellipseaxis.h"
#include "rs_actiondrawellipsefocipoint.h"
#include "rs_actiondrawellipse4points.h"
#include "rs_actiondrawellipsecenter3points.h"
#include "rs_actiondrawellipseinscribe.h"
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
#include "lc_actiondrawlinepolygon3.h"
#include "rs_actiondrawlinerectangle.h"
#include "rs_actiondrawlinerelangle.h"
#include "rs_actiondrawlineorthtan.h"
#include "rs_actiondrawlinetangent1.h"
#include "rs_actiondrawlinetangent2.h"
#include "rs_actiondrawmtext.h"
#include "rs_actiondrawpoint.h"
#include "rs_actiondrawspline.h"
#include "lc_actiondrawsplinepoints.h"
#include "rs_actiondrawtext.h"
#include "rs_actioneditcopy.h"
#include "rs_actioneditundo.h"
#include "lc_actionfileexportmakercam.h"
#include "rs_actionfilenewtemplate.h"
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
#include "rs_actionlayerslockall.h"
#include "rs_actionlayersremove.h"
#include "rs_actionlayerstogglelock.h"
#include "rs_actionlayerstoggleview.h"
#include "rs_actionlayerstoggleprint.h"
#include "lc_actionlayerstoggleconstruction.h"
#include "lc_actionlayersexport.h"
#include "rs_actionlibraryinsert.h"
#include "rs_actionlibraryinsert.h"
#include "rs_actionlockrelativezero.h"
#include "rs_actionmodifyattributes.h"
#include "rs_actionmodifybevel.h"
#include "rs_actionmodifycut.h"
#include "rs_actionmodifydelete.h"
#include "rs_actionmodifydeletefree.h"
#include "rs_actionmodifyentity.h"
#include "rs_actionmodifyexplodetext.h"
#include "rs_actionmodifymirror.h"
#include "rs_actionmodifymove.h"
#include "rs_actionmodifymoverotate.h"
#include "rs_actionmodifyrevertdirection.h"
#include "rs_actionmodifyrotate.h"
#include "rs_actionmodifyrotate2.h"
#include "rs_actionmodifyround.h"
#include "rs_actionmodifyoffset.h"
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
#include "lc_actionsnapmiddlemanual.h"
#include "rs_actiontoolregeneratedimensions.h"
#include "rs_actionzoomauto.h"
#include "rs_actionzoomin.h"
#include "rs_actionzoompan.h"
#include "rs_actionzoomprevious.h"
#include "rs_actionzoomredraw.h"
#include "rs_actionzoomwindow.h"
#include "lc_actiondrawrectangle3points.h"
#include "lc_actiondrawcross.h"
#include "lc_actiondrawlinesnake.h"
#include "rs_actiondrawpolyline.h"
#include "rs_actionpolylineadd.h"
#include "rs_actionpolylineappend.h"
#include "rs_actionpolylinedel.h"
#include "rs_actionpolylinedelbetween.h"
#include "rs_actionpolylinetrim.h"
#include "rs_actionpolylineequidistant.h"
#include "rs_actionpolylinesegment.h"
#include "rs_selection.h"
#include "rs_actionorder.h"
#include "rs_modification.h"

#include "qg_snaptoolbar.h"
#include "rs_debug.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_settings.h"
#include "lc_actionpenpick.h"
#include "lc_actionpenapply.h"
#include "lc_actionpensyncactivebylayer.h"
#include "lc_actiondrawlineanglerel.h"
#include "lc_actiondrawlinefrompointtoline.h"
#include "rs_math.h"
#include "lc_actiondrawslicedivide.h"
#include "lc_actiondrawrectangle1point.h"
#include "lc_actiondrawrectangle2points.h"
#include "lc_actiondrawcirclebyarc.h"
#include "lc_actionmodifylinejoin.h"
#include "lc_actiondrawlinepoints.h"
#include "lc_actionmodifyduplicate.h"
#include "lc_actiondrawstar.h"
#include "lc_actionmodifybreakdivide.h"
#include "lc_actionmodifylinegap.h"
#include "lc_actioninfoproperties.h"
#include "lc_actioninfopickcoordinates.h"
#include "lc_actioneditpastetransform.h"
#include "lc_actioninfo3pointsangle.h"
#include "lc_actiondrawellipse1point.h"
#include "lc_actiondrawdimbaseline.h"

/**
 * Constructor
 */
QG_ActionHandler::QG_ActionHandler(QObject *parent)
    :QObject(parent) {
    RS_DEBUG->print("QG_ActionHandler::QG_ActionHandler");
    RS_DEBUG->print("QG_ActionHandler::QG_ActionHandler: OK");
}

/**
 * Kills all running selection actions. Called when a selection action
  * is launched to reduce confusion.
   */
void QG_ActionHandler::killSelectActions() {
    if (view != nullptr) {
        view->killSelectActions();
    }
}

void QG_ActionHandler::killAllActions() {
    if (view != nullptr) {
        view->killAllActions();
    }
}

/**
 * @return Current action or nullptr.
 */
RS_ActionInterface *QG_ActionHandler::getCurrentAction() {
    if (view != nullptr) {
        return view->getCurrentAction();
    } else {
        return nullptr;
    }
}

/**
 * Sets current action.
 *
 * @return Pointer to the created action or nullptr.
 */
RS_ActionInterface* QG_ActionHandler::setCurrentAction(RS2::ActionType id) {
    RS_DEBUG->print("QG_ActionHandler::setCurrentAction()");
    RS_ActionInterface* a = nullptr;
//    view->killAllActions();

    RS_DEBUG->print("QC_ActionHandler::setCurrentAction: "
            "view = %p, document = %p", view, document);

    // only global options are allowed without a document:
    if (view==nullptr || document==nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QG_ActionHandler::setCurrentAction: graphic view or "
                "document is nullptr");
        return nullptr;
    }

    auto a_layer = (document->getLayerList() != nullptr) ? document->getLayerList()->getActive() : nullptr;

    switch (id) {
        //case RS2::ActionFileNew:
        //    a = new RS_ActionFileNew(*document, *view);
        //	break;
        case RS2::ActionFileNewTemplate:
            a = new RS_ActionFileNewTemplate(*document, *view);
            break;
            //case RS2::ActionFileSave:
            //    a = new RS_ActionFileSave(*document, *view);
            //	break;
            //case RS2::ActionFileClose:
            //    //a = new RS_ActionFileClose(*document, *view);
            //	break;
            //case RS2::ActionFileQuit:
            //    //a = new RS_ActionFileQuit(*document, *view);
            //	break;
        case RS2::ActionFileOpen:
            a = new RS_ActionFileOpen(*document, *view);
            break;
        case RS2::ActionFileSaveAs:
            a = new RS_ActionFileSaveAs(*document, *view);
            break;
        case RS2::ActionFileExportMakerCam:
            a = new LC_ActionFileExportMakerCam(*document, *view);
            break;

            // Editing actions:
            //
        case RS2::ActionEditKillAllActions:
            if (view) {
                // DO we need to call some form of a 'clean' function?
                view->killAllActions();

                RS_Selection s((RS_EntityContainer&)*document, view);
                s.selectAll(false);
                RS_DIALOGFACTORY->updateSelectionWidget(document->countSelected(),document->totalSelectedLength());
            }
            break;
        case RS2::ActionEditUndo:
            //to avoid operation on deleted entities, Undo action invalid all suspended
            //actions
            killAllActions();
            a = new RS_ActionEditUndo(true, *document, *view);
            break;
        case RS2::ActionEditRedo:
            a = new RS_ActionEditUndo(false, *document, *view);
            break;
        case RS2::ActionEditCut:
            if(!document->countSelected()){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionEditCutNoSelect);
                break;
            }
            // fall-through
        case RS2::ActionEditCutNoSelect:
            a = new RS_ActionEditCopyPaste(RS_ActionEditCopyPaste::CUT, *document, *view);
            break;
        case RS2::ActionEditCutQuick:
            if(!document->countSelected()){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionEditCutQuickNoSelect);
                break;
            }
            // fall-through
        case RS2::ActionEditCutQuickNoSelect:
            a = new RS_ActionEditCopyPaste(RS_ActionEditCopyPaste::CUT_QUICK, *document, *view);
            break;
        case RS2::ActionEditCopy:
            if(!document->countSelected()){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionEditCopyNoSelect);
                break;
            }
            // fall-through
        case RS2::ActionEditCopyNoSelect:
            a = new RS_ActionEditCopyPaste(RS_ActionEditCopyPaste::COPY, *document, *view);
            break;
        case RS2::ActionEditCopyQuick:
            if(!document->countSelected()){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionEditCopyQuickNoSelect);
                break;
            }
            // fall-through
        case RS2::ActionEditCopyQuickNoSelect:
            a = new RS_ActionEditCopyPaste(RS_ActionEditCopyPaste::COPY_QUICK, *document, *view);
            break;
        case RS2::ActionEditPaste:
              a = new RS_ActionEditCopyPaste(RS_ActionEditCopyPaste::PASTE, *document, *view);
//            a = new RS_ActionEditPaste(*document, *view);
            break;
        case RS2::ActionEditPasteTransform:
            a = new LC_ActionEditPasteTransform(*document, *view);
            break;
        case RS2::ActionOrderBottom:
            orderType = RS2::ActionOrderBottom;
            if(!document->countSelected()){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionOrderNoSelect);
            }
            else {
                a = new RS_ActionOrder(*document, *view, orderType);
            }
            break;
        case RS2::ActionOrderLower:
            orderType = RS2::ActionOrderLower;
            a = new RS_ActionSelect(this, *document, *view, RS2::ActionOrderNoSelect);
            break;
        case RS2::ActionOrderRaise:
            orderType = RS2::ActionOrderRaise;
            a = new RS_ActionSelect(this, *document, *view, RS2::ActionOrderNoSelect);
            break;
        case RS2::ActionOrderTop:
            orderType = RS2::ActionOrderTop;
            if(!document->countSelected()){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionOrderNoSelect);
            }
            else {
                a = new RS_ActionOrder(*document, *view, orderType);
            }
            break;
        case RS2::ActionOrderNoSelect:
            a = new RS_ActionOrder(*document, *view, orderType);
            break;

            // Selecting actions:
            //
        case RS2::ActionSelectSingle:
//        view->killSelectActions();
            if(getCurrentAction()->rtti() != RS2::ActionSelectSingle) {
                a = new RS_ActionSelectSingle(*document, *view,getCurrentAction());
            }else{
                a=nullptr;
            }
            break;
        case RS2::ActionSelectContour:
            view->killSelectActions();
            a = new RS_ActionSelectContour(*document, *view);
            break;
        case RS2::ActionSelectAll:
            a = new RS_ActionSelectAll(*document, *view, true);
            break;
        case RS2::ActionDeselectAll:
            a = new RS_ActionSelectAll(*document, *view, false);
            break;
        case RS2::ActionSelectWindow:
            view->killSelectActions();
            a = new RS_ActionSelectWindow(view->getTypeToSelect(),*document, *view, true);
            break;
        case RS2::ActionDeselectWindow:
            view->killSelectActions();
            a = new RS_ActionSelectWindow(*document, *view, false);
            break;
        case RS2::ActionSelectInvert:
            a = new RS_ActionSelectInvert(*document, *view);
            break;
        case RS2::ActionSelectIntersected:
            view->killSelectActions();
            a = new RS_ActionSelectIntersected(*document, *view, true);
            break;
        case RS2::ActionDeselectIntersected:
            view->killSelectActions();
            a = new RS_ActionSelectIntersected(*document, *view, false);
            break;
        case RS2::ActionSelectLayer:
            view->killSelectActions();
            a = new RS_ActionSelectLayer(*document, *view);
            break;

            // Tool actions:
            //
        case RS2::ActionToolRegenerateDimensions:
            a = new RS_ActionToolRegenerateDimensions(*document, *view);
            break;

            // Zooming actions:
            //
        case RS2::ActionZoomIn:
            a = new RS_ActionZoomIn(*document, *view, RS2::In, RS2::Both);
            break;
        case RS2::ActionZoomOut:
            a = new RS_ActionZoomIn(*document, *view, RS2::Out, RS2::Both);
            break;
        case RS2::ActionZoomAuto:
            a = new RS_ActionZoomAuto(*document, *view);
            break;
        case RS2::ActionZoomWindow:
            a = new RS_ActionZoomWindow(*document, *view);
            break;
        case RS2::ActionZoomPan:
            a = new RS_ActionZoomPan(*document, *view);
            break;
        case RS2::ActionZoomPrevious:
            a = new RS_ActionZoomPrevious(*document, *view);
            break;
        case RS2::ActionZoomRedraw:
            a = new RS_ActionZoomRedraw(*document, *view);
            break;

            // Drawing actions:
            //
        case RS2::ActionDrawPoint:
            a = new RS_ActionDrawPoint(*document, *view);
            break;
        case RS2::ActionDrawLine:
            a = new RS_ActionDrawLine(*document, *view);
            break;
        case RS2::ActionDrawLineAngle:
            a = new RS_ActionDrawLineAngle(*document, *view, 0.0, false);
            break;
        case RS2::ActionDrawLineHorizontal:
            a = new RS_ActionDrawLineAngle(*document, *view, 0.0, true,
                                           RS2::ActionDrawLineHorizontal);
            break;
        case RS2::ActionDrawLineHorVert:
            a = new RS_ActionDrawLineHorVert(*document, *view);
            break;
        case RS2::ActionDrawLineVertical:
            a = new RS_ActionDrawLineAngle(*document, *view, 90, true,
                                           RS2::ActionDrawLineVertical);
            break;
        case RS2::ActionDrawLineFree:
            a = new RS_ActionDrawLineFree(*document, *view);
            break;
        case RS2::ActionDrawLineParallel:
        case RS2::ActionDrawCircleParallel:
        case RS2::ActionDrawArcParallel:
            a= new RS_ActionDrawLineParallel(*document, *view, id);
            break;
        case RS2::ActionDrawLineParallelThrough:
            a = new RS_ActionDrawLineParallelThrough(*document, *view);
            break;
        case RS2::ActionDrawLineRectangle:
            a = new RS_ActionDrawLineRectangle(*document, *view);
            break;
        case RS2::ActionDrawRectangle3Points:
            a = new LC_ActionDrawRectangle3Points(*document, *view);
            break;
        case RS2::ActionDrawRectangle2Points:
            a = new LC_ActionDrawRectangle2Points(*document, *view);
            break;

        case RS2::ActionDrawRectangle1Point:
            a = new LC_ActionDrawRectangle1Point(*document, *view);
            break;
        case RS2::ActionDrawCross:
            a = new LC_ActionDrawCross(*document, *view);
            break;
        case RS2::ActionDrawSnakeLine:
            a = new LC_ActionDrawLineSnake(*document, *view, LC_ActionDrawLineSnake::DIRECTION_POINT);
            break;
        case RS2::ActionDrawSnakeLineX:
            a = new LC_ActionDrawLineSnake(*document, *view, LC_ActionDrawLineSnake::DIRECTION_X);
            break;
        case RS2::ActionDrawSnakeLineY:
            a = new LC_ActionDrawLineSnake(*document, *view, LC_ActionDrawLineSnake::DIRECTION_Y);
            break;
        case RS2::ActionDrawSliceDivideLine:
            a = new LC_ActionDrawSliceDivide(*document, *view, false);
            break;
        case RS2::ActionDrawSliceDivideCircle:
            a = new LC_ActionDrawSliceDivide(*document, *view, true);
            break;
        case RS2::ActionDrawLinePoints:
            a = new LC_ActionDrawLinePoints(*document, *view);
            break;
        case RS2::ActionDrawLineBisector:
            a = new RS_ActionDrawLineBisector(*document, *view);
            break;
        case RS2::ActionDrawLineOrthTan:
            a = new RS_ActionDrawLineOrthTan(*document, *view);
            break;
        case RS2::ActionDrawLineTangent1:
            a = new RS_ActionDrawLineTangent1(*document, *view);
            break;
        case RS2::ActionDrawLineTangent2:
            a = new RS_ActionDrawLineTangent2(*document, *view);
            break;
        case RS2::ActionDrawLineOrthogonal:
            a = new RS_ActionDrawLineRelAngle(*document, *view, M_PI_2, true);
            break;
        case RS2::ActionDrawLineRelAngle:
            a = new RS_ActionDrawLineRelAngle(*document, *view, M_PI_2, false);
            break;
        case RS2::ActionDrawPolyline:
            a = new RS_ActionDrawPolyline(*document, *view);
            break;
        case RS2::ActionDrawLineOrthogonalRel:
            a = new LC_ActionDrawLineAngleRel(*document, *view, 90.0, true);
            break;
        case RS2::ActionDrawLineAngleRel:
            a = new LC_ActionDrawLineAngleRel(*document, *view, 0.0, false);
            break;
        case RS2::ActionDrawLineFromPointToLine:{
            a = new LC_ActionDrawLineFromPointToLine(this, *document, *view);
            break;
        }
        case RS2::ActionDrawStar:{
            a = new LC_ActionDrawStar(*document, *view);
            break;
        }
        case RS2::ActionPolylineAdd:
            a = new RS_ActionPolylineAdd(*document, *view);
            break;
        case RS2::ActionPolylineAppend:
            a = new RS_ActionPolylineAppend(*document, *view);
            break;
        case RS2::ActionPolylineDel:
            a = new RS_ActionPolylineDel(*document, *view);
            break;
        case RS2::ActionPolylineDelBetween:
            a = new RS_ActionPolylineDelBetween(*document, *view);
            break;
        case RS2::ActionPolylineTrim:
            a = new RS_ActionPolylineTrim(*document, *view);
            break;
        case RS2::ActionPolylineEquidistant:
            a = new RS_ActionPolylineEquidistant(*document, *view);
            break;
        case RS2::ActionPolylineSegment:
            a = new RS_ActionPolylineSegment(*document, *view);
            break;
        case RS2::ActionDrawLinePolygonCenCor:
            a = new RS_ActionDrawLinePolygonCenCor(*document, *view);
            break;
        case RS2::ActionDrawLinePolygonCenTan:                      //20161223 added by txmy
            a = new LC_ActionDrawLinePolygonCenTan(*document, *view);
            break;
        case RS2::ActionDrawLinePolygonCorCor:
            a = new RS_ActionDrawLinePolygonCorCor(*document, *view);
            break;
        case RS2::ActionDrawCircle:
            a = new RS_ActionDrawCircle(*document, *view);
            break;
        case RS2::ActionDrawCircleCR:
            a = new RS_ActionDrawCircleCR(*document, *view);
            break;
        case RS2::ActionDrawCircleByArc:
            a = new LC_ActionDrawCircleByArc(*document, *view);
            break;
        case RS2::ActionDrawCircle2P:
            a = new RS_ActionDrawCircle2P(*document, *view);
            break;
        case RS2::ActionDrawCircle2PR:
            a = new LC_ActionDrawCircle2PR(*document, *view);
            break;
        case RS2::ActionDrawCircle3P:
            a = new RS_ActionDrawCircle3P(*document, *view);
            break;
        case RS2::ActionDrawCircleTan1_2P:
            a = new RS_ActionDrawCircleTan1_2P(*document, *view);
            break;
        case RS2::ActionDrawCircleTan2_1P:
            a = new RS_ActionDrawCircleTan2_1P(*document, *view);
            break;
        case RS2::ActionDrawCircleInscribe:
            a = new RS_ActionDrawCircleInscribe(*document, *view);
            break;
        case RS2::ActionDrawCircleTan2:
            a = new RS_ActionDrawCircleTan2(*document, *view);
            break;
        case RS2::ActionDrawCircleTan3:
            a = new RS_ActionDrawCircleTan3(*document, *view);
            break;
        case RS2::ActionDrawArc:
            a = new RS_ActionDrawArc(*document, *view);
            break;
        case RS2::ActionDrawArc3P:
            a = new RS_ActionDrawArc3P(*document, *view);
            break;    case RS2::ActionDrawArcTangential:
            a = new RS_ActionDrawArcTangential(*document, *view);
            break;
        case RS2::ActionDrawEllipseAxis:
            a = new RS_ActionDrawEllipseAxis(*document, *view, false);
            break;
        case RS2::ActionDrawEllipseArcAxis:
            a = new RS_ActionDrawEllipseAxis(*document, *view, true);
            break;
        case RS2::ActionDrawEllipse1Point:
            a = new LC_ActionDrawEllipse1Point(*document, *view, false);
            break;
        case RS2::ActionDrawEllipseArc1Point:
            a = new LC_ActionDrawEllipse1Point(*document, *view, true);
            break;
        case RS2::ActionDrawParabola4Points:
            a = new LC_ActionDrawParabola4Points(*document, *view);
            break;
        case RS2::ActionDrawParabolaFD:
            a = new LC_ActionDrawParabolaFD(*document, *view);
            break;
        case RS2::ActionDrawEllipseFociPoint:
            a = new RS_ActionDrawEllipseFociPoint(*document, *view);
            break;
        case RS2::ActionDrawEllipse4Points:
            a = new RS_ActionDrawEllipse4Points(*document, *view);
            break;
        case RS2::ActionDrawEllipseCenter3Points:
            a = new RS_ActionDrawEllipseCenter3Points(*document, *view);
            break;
        case RS2::ActionDrawEllipseInscribe:
            a = new RS_ActionDrawEllipseInscribe(*document, *view);
            break;
        case RS2::ActionDrawSpline:
            a = new RS_ActionDrawSpline(*document, *view);
            break;
        case RS2::ActionDrawSplinePoints:
            a = new LC_ActionDrawSplinePoints(*document, *view);
            break;
        case RS2::ActionDrawMText:
            a = new RS_ActionDrawMText(*document, *view);
            break;
        case RS2::ActionDrawText:
            a = new RS_ActionDrawText(*document, *view);
            break;
        case RS2::ActionDrawHatch:
            if(!document->countSelected())
            {
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionDrawHatchNoSelect);
                break;
            }
            // fall-through
        case RS2::ActionDrawHatchNoSelect:
            a = new RS_ActionDrawHatch(*document, *view);
            break;
        case RS2::ActionDrawImage:
            a = new RS_ActionDrawImage(*document, *view);
            break;

            // Dimensioning actions:
            //
        case RS2::ActionDimAligned:
            a = new RS_ActionDimAligned(*document, *view);
            break;
        case RS2::ActionDimLinear:
            a = new RS_ActionDimLinear(*document, *view);
            break;
        case RS2::ActionDimLinearHor:
            a = new RS_ActionDimLinear(*document, *view, 0.0, true, RS2::ActionDimLinearHor);
            break;
        case RS2::ActionDimLinearVer:
            a = new RS_ActionDimLinear(*document, *view, M_PI_2, true, RS2::ActionDimLinearVer);
            break;
        case RS2::ActionDimRadial:
            a = new RS_ActionDimRadial(*document, *view);
            break;
        case RS2::ActionDimDiametric:
            a = new RS_ActionDimDiametric(*document, *view);
            break;
        case RS2::ActionDimAngular:
            a = new RS_ActionDimAngular(*document, *view);
            break;
        case RS2::ActionDimArc:
            a = new LC_ActionDimArc(*document, *view);
            break;
        case RS2::ActionDimLeader:
            a = new RS_ActionDimLeader(*document, *view);
            break;
        case RS2::ActionDimBaseline:
            a = new LC_ActionDrawDimBaseline(*document, *view, RS2::ActionDimBaseline);
            break;
        case RS2::ActionDimContinue:
            a = new LC_ActionDrawDimBaseline(*document, *view, RS2::ActionDimContinue);
            break;

            // Modifying actions:
            //
        case RS2::ActionModifyLineJoin: {
            a = new LC_ActionModifyLineJoin(*document, *view);
            break;
        }
        case RS2::ActionModifyDuplicate: {
            a = new LC_ActionModifyDuplicate(*document, *view);
            break;
        }
        case RS2::ActionModifyBreakDivide: {
            a = new LC_ActionModifyBreakDivide(*document, *view);
            break;
        }
        case RS2::ActionModifyLineGap: {
            a = new LC_ActionModifyLineGap(*document, *view);
            break;
        }
        case RS2::ActionModifyAttributes:
            a = new RS_ActionModifyAttributes(*document, *view);
            break;
        case RS2::ActionModifyDelete:
            a = new RS_ActionModifyDelete(*document, *view);
            break;
        case RS2::ActionModifyDeleteQuick:
            a = new RS_ActionSelect(this, *document, *view, RS2::ActionModifyDeleteQuick);
            break;
        case RS2::ActionModifyDeleteFree:
            a = new RS_ActionModifyDeleteFree(*document, *view);
            break;
        case RS2::ActionModifyMove:
            a = new RS_ActionModifyMove(*document, *view);
            break;
        case RS2::ActionModifyRevertDirection:
            a = new RS_ActionModifyRevertDirection(*document, *view);
            break;
        case RS2::ActionModifyRotate:
            a = new RS_ActionModifyRotate(*document, *view);
            break;
        case RS2::ActionModifyScale:
            a = new RS_ActionModifyScale(*document, *view);
            break;
        case RS2::ActionModifyMirror:
            a = new RS_ActionModifyMirror(*document, *view);
            break;
        case RS2::ActionModifyMoveRotate:
            a = new RS_ActionModifyMoveRotate(*document, *view);
            break;
        case RS2::ActionModifyRotate2:
            a = new RS_ActionModifyRotate2(*document, *view);
            break;
        case RS2::ActionModifyEntity:
            a = new RS_ActionModifyEntity(*document, *view);
            break;
        case RS2::ActionModifyTrim:
            a = new RS_ActionModifyTrim(*document, *view, false);
            break;
        case RS2::ActionModifyTrim2:
            a = new RS_ActionModifyTrim(*document, *view, true);
            break;
        case RS2::ActionModifyTrimAmount:
            a = new RS_ActionModifyTrimAmount(*document, *view);
            break;
        case RS2::ActionModifyCut:
            a = new RS_ActionModifyCut(*document, *view);
            break;
        case RS2::ActionModifyStretch:
            a = new RS_ActionModifyStretch(*document, *view);
            break;
        case RS2::ActionModifyBevel:
            a = new RS_ActionModifyBevel(*document, *view);
            break;
        case RS2::ActionModifyRound:
            a = new RS_ActionModifyRound(*document, *view);
            break;
        case RS2::ActionModifyOffset:
            a = new RS_ActionModifyOffset(*document, *view);
            break;
        case RS2::ActionModifyExplodeText:
          /*  if(!document->countSelected(false, {RS2::EntityText, RS2::EntityMText})){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionModifyExplodeTextNoSelect);
                break;
            }
            // fall-through
*/
            a = new RS_ActionModifyExplodeText(*document, *view);
            break;

            // Snapping actions:
            //
        case RS2::ActionSnapFree:
//        a = new RS_ActionSetSnapMode(*document, *view, RS2::SnapFree);
            slotSnapFree();
            break;
        case RS2::ActionSnapCenter:
//        a = new RS_ActionSetSnapMode(*document, *view, RS2::SnapCenter);
            slotSnapCenter();
            break;
        case RS2::ActionSnapDist:
            slotSnapDist();
//        a = new RS_ActionSetSnapMode(*document, *view, RS2::SnapDist);
            break;
        case RS2::ActionSnapEndpoint:
            slotSnapEndpoint();
//        a = new RS_ActionSetSnapMode(*document, *view, RS2::SnapEndpoint);
            break;
        case RS2::ActionSnapGrid:
            slotSnapGrid();
//        a = new RS_ActionSetSnapMode(*document, *view, RS2::SnapGrid);
            break;
        case RS2::ActionSnapIntersection:
            slotSnapIntersection();
//        a = new RS_ActionSetSnapMode(*document, *view, RS2::SnapIntersection);
            break;
        case RS2::ActionSnapMiddle:
            slotSnapMiddle();
//        a = new RS_ActionSetSnapMode(*document, *view, RS2::SnapMiddle);
            break;
        case RS2::ActionSnapOnEntity:
            slotSnapOnEntity();
//        a = new RS_ActionSetSnapMode(*document, *view, RS2::SnapOnEntity);
            break;
//    case RS2::ActionSnapIntersectionManual:
//        a = new RS_ActionSnapIntersectionManual(*document, *view);
//        break;

            // Snap restriction actions:
            //
        case RS2::ActionRestrictNothing:
            slotRestrictNothing();
//        a = new RS_ActionSetSnapRestriction(*document, *view, RS2::RestrictNothing);
            break;
        case RS2::ActionRestrictOrthogonal:
            slotRestrictOrthogonal();
//        a = new RS_ActionSetSnapRestriction(*document, *view, RS2::RestrictOrthogonal);
            break;
        case RS2::ActionRestrictHorizontal:
            slotRestrictHorizontal();
//        a = new RS_ActionSetSnapRestriction(*document, *view, RS2::RestrictHorizontal);
            break;
        case RS2::ActionRestrictVertical:
            slotRestrictVertical();
//        a = new RS_ActionSetSnapRestriction(*document, *view, RS2::RestrictVertical);
            break;

            // Relative zero:
            //
        case RS2::ActionSetRelativeZero:
            a = new RS_ActionSetRelativeZero(*document, *view);
            break;
        case RS2::ActionLockRelativeZero:
            a = new RS_ActionLockRelativeZero(*document, *view, true);
            break;
        case RS2::ActionUnlockRelativeZero:
            a = new RS_ActionLockRelativeZero(*document, *view, false);
            break;

            // pen actions

        case RS2::ActionPenPick:
            a = new LC_ActionPenPick(*document, *view,  false);
            break;
        case RS2::ActionPenPickResolved:
            a = new LC_ActionPenPick(*document, *view, true);
            break;
        case RS2::ActionPenApply:
            a = new LC_ActionPenApply(*document, *view, false);
            break;
        case RS2::ActionPenCopy:
            a = new LC_ActionPenApply(*document, *view, true);
            break;

        case RS2::ActionPenSyncFromLayer:
            a = new LC_ActionPenSyncActiveByLayer(*document, *view);
            break;
            // Info actions:
            //
        case RS2::ActionInfoInside:
            a = new RS_ActionInfoInside(*document, *view);
            break;
        case RS2::ActionInfoDistPoint2Point:
            a = new RS_ActionInfoDist(*document, *view);
            break;
        case RS2::ActionInfoDistEntity2Point:
            a = new RS_ActionInfoDist2(*document, *view);
            break;
        case RS2::ActionInfoDistPoint2Entity:
            a = new RS_ActionInfoDist2(*document, *view, true);
            break;
        case RS2::ActionInfoAngle:
            a = new RS_ActionInfoAngle(*document, *view);
            break;
         case RS2::ActionInfoAngle3Points:
            a = new LC_ActionInfo3PointsAngle(*document, *view);
            break;
        case RS2::ActionInfoTotalLength:
            if(!document->countSelected()){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionInfoTotalLengthNoSelect);
                break;
            }
            // fall-through
        case RS2::ActionInfoTotalLengthNoSelect:
            a = new RS_ActionInfoTotalLength(*document, *view);
            break;
        case RS2::ActionInfoArea:
            a = new RS_ActionInfoArea(*document, *view);
            break;
        case RS2::ActionInfoProperties:
            a = new LC_ActionInfoProperties(*document, *view);
            break;
        case RS2::ActionInfoPickCoordinates:
            a = new LC_ActionInfoPickCoordinates(*document, *view);
            break;


            // Layer actions:
            //
        case RS2::ActionLayersDefreezeAll:
            a = new RS_ActionLayersFreezeAll(false, *document, *view);
            break;
        case RS2::ActionLayersFreezeAll:
            a = new RS_ActionLayersFreezeAll(true, *document, *view);
            break;
        case RS2::ActionLayersUnlockAll:
            a = new RS_ActionLayersLockAll(false, *document, *view);
            break;
        case RS2::ActionLayersLockAll:
            a = new RS_ActionLayersLockAll(true, *document, *view);
            break;
        case RS2::ActionLayersAdd:
            a = new RS_ActionLayersAdd(*document, *view);
            break;
        case RS2::ActionLayersRemove:
            a = new RS_ActionLayersRemove(*document, *view);
            break;
        case RS2::ActionLayersEdit:
            a = new RS_ActionLayersEdit(*document, *view);
            break;
        case RS2::ActionLayersToggleView:
            if (a_layer != nullptr)
                a = new RS_ActionLayersToggleView(*document, *view, a_layer);
            break;
        case RS2::ActionLayersToggleLock:
            if (a_layer != nullptr)
                a = new RS_ActionLayersToggleLock(*document, *view, a_layer);
            break;
        case RS2::ActionLayersTogglePrint:
            if (a_layer != nullptr)
                a = new RS_ActionLayersTogglePrint(*document, *view, a_layer);
            break;
        case RS2::ActionLayersToggleConstruction:
            if (a_layer != nullptr)
                a = new LC_ActionLayersToggleConstruction(*document, *view, a_layer);
            break;
        case RS2::ActionLayersExportSelected:
            a = new LC_ActionLayersExport(*document, *view, document->getLayerList(), LC_ActionLayersExport::SelectedMode);
            break;
        case RS2::ActionLayersExportVisible:
            a = new LC_ActionLayersExport(*document, *view, document->getLayerList(), LC_ActionLayersExport::VisibleMode);
            break;

            // Block actions:
            //
        case RS2::ActionBlocksDefreezeAll:
            a = new RS_ActionBlocksFreezeAll(false, *document, *view);
            break;
        case RS2::ActionBlocksFreezeAll:
            a = new RS_ActionBlocksFreezeAll(true, *document, *view);
            break;
        case RS2::ActionBlocksAdd:
            a = new RS_ActionBlocksAdd(*document, *view);
            break;
        case RS2::ActionBlocksRemove:
            a = new RS_ActionBlocksRemove(*document, *view);
            break;
        case RS2::ActionBlocksAttributes:
            a = new RS_ActionBlocksAttributes(*document, *view);
            break;
        case RS2::ActionBlocksEdit:
            a = new RS_ActionBlocksEdit(*document, *view);
            break;
        case RS2::ActionBlocksSave:
            a = new RS_ActionBlocksSave(*document, *view);
            break;
        case RS2::ActionBlocksInsert:
            a = new RS_ActionBlocksInsert(*document, *view);
            break;
        case RS2::ActionBlocksToggleView:
            a = new RS_ActionBlocksToggleView(*document, *view);
            break;
        case RS2::ActionBlocksCreate:
            if(!document->countSelected()){
                a = new RS_ActionSelect(this, *document, *view, RS2::ActionBlocksCreateNoSelect);
                break;
            }
            // fall-through
        case RS2::ActionBlocksCreateNoSelect:
            a = new RS_ActionBlocksCreate(*document, *view);
            break;
        case RS2::ActionBlocksExplode:
            a = new RS_ActionBlocksExplode(*document, *view);
            break;
            // library browser:
            //
        case RS2::ActionLibraryInsert:
            a = new RS_ActionLibraryInsert(*document, *view);
            break;

            // options:
            //
            //case RS2::ActionOptionsGeneral:
            //    a = new RS_ActionOptionsGeneral(*document, *view);
            //	break;

        case RS2::ActionOptionsDrawing:
            a = new RS_ActionOptionsDrawing(*document, *view);
            break;
        default:
            RS_DEBUG->print(RS_Debug::D_WARNING,
                            "QG_ActionHandler::setCurrentAction():"
                            "No such action found.");
            break;
    }

	if (a) {
        view->setCurrentAction(a);
    }

    RS_DEBUG->print("QG_ActionHandler::setCurrentAction(): OK");
    return a;
}

/**
 * @return Available commands of the application or the current action.
 */
QStringList QG_ActionHandler::getAvailableCommands() {
    RS_ActionInterface* currentAction = getCurrentAction();

	if (currentAction) {
        return currentAction->getAvailableCommands();
    } else {
        QStringList cmd;
        cmd += "line";
        cmd += "rectangle";
        return cmd;
    }
}

//get snap mode from snap toolbar
RS_SnapMode QG_ActionHandler::getSnaps()
{

    if (snap_toolbar) {
        return snap_toolbar->getSnaps();
    }
    //return a free snap mode
    return RS_SnapMode();
}

/**
 * Launches the command represented by the given keycode if possible.
 *
 * @return true: the command was recognized.
 *         false: the command is not known and was probably intended for a
 *            running action.
 */
bool QG_ActionHandler::keycode(const QString& code) {
    RS_DEBUG->print("QG_ActionHandler::keycode()");

    // pass keycode on to running action:
    //RS_keycodeEvent e(cmd);

	//if (view) {
    //    view->keycodeEvent(&e);
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
        case RS2::ActionSnapCenter:
            slotSnapCenter();
            break;
        case RS2::ActionSnapDist:
            slotSnapDist();
            break;
        case RS2::ActionSnapEndpoint:
            slotSnapEndpoint();
            break;
        case RS2::ActionSnapGrid:
            slotSnapGrid();
            break;
        case RS2::ActionSnapIntersection:
            slotSnapIntersection();
            break;
        case RS2::ActionSnapMiddle:
            slotSnapMiddle();
            break;
        case RS2::ActionSnapOnEntity:
            slotSnapOnEntity();
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
  * toggle snap modes when calling from command line
  **/
bool QG_ActionHandler::commandLineActions(RS2::ActionType type){
    RS_DEBUG->print("QG_ActionHandler::commandLineSnap()");

        // snap actions require special handling (GUI update)
    //more special handling of actions can be added here
        switch (type) {
        case RS2::ActionSnapCenter:
            slotSnapCenter();
            return true;
        case RS2::ActionSnapDist:
            slotSnapDist();
            return true;
        case RS2::ActionSnapEndpoint:
            slotSnapEndpoint();
            return true;
        case RS2::ActionSnapGrid:
            slotSnapGrid();
            return true;
        case RS2::ActionSnapIntersection:
            slotSnapIntersection();
            return true;
        case RS2::ActionSnapMiddle:
            slotSnapMiddle();
            return true;
        case RS2::ActionSnapOnEntity:
            slotSnapOnEntity();
            return true;
        case RS2::ActionRestrictNothing:
            slotRestrictNothing();
            return true;
        case RS2::ActionRestrictOrthogonal:
            slotRestrictOrthogonal();
            return true;
        case RS2::ActionRestrictHorizontal:
            slotRestrictHorizontal();
            return true;
        case RS2::ActionRestrictVertical:
            slotRestrictVertical();
            return true;

        default:
            return false;
        }

}

/**
 * Launches the given command if possible.
 *
 * @return true: the command was recognized.
 *         false: the command is not known and was probably intended for a
 *            running action.
 */
bool QG_ActionHandler::command(const QString& cmd)
{
    if (!view) return false;

    if (cmd.isEmpty())
    {
        if (snap_toolbar != nullptr && LC_GET_BOOL("Keyboard/ToggleFreeSnapOnSpace")) {
            RS_DEBUG->print("QG_ActionHandler::command: toggle Snap Free: begin");
            RS_SnapMode smFree = {};
            RS_SnapMode smGV = snap_toolbar->getSnaps();
            if (smFree != smGV) {
                const bool isSnappingFree = view->getDefaultSnapMode() == smFree;
                view->setDefaultSnapMode(isSnappingFree ? smGV: smFree);
                RS_DIALOGFACTORY->commandMessage(isSnappingFree?
                                                     tr("Spacebar: restored snapping mode to normal")
                                                   : tr("Spacebar: temporarily set snapping mode to free snapping"));
            }
            RS_DEBUG->print("QG_ActionHandler::command: toggle Snap Free: OK");
        }
        return true;
    }

    RS_DEBUG->print("QG_ActionHandler::command: %s", cmd.toLatin1().data());
    QString c = cmd.toLower();

    if (c==tr("escape", "escape, go back from action steps"))
    {
        view->back();
        RS_DEBUG->print("QG_ActionHandler::command: back");
        return true;
    }

    // pass command on to running action:
    RS_CommandEvent e(cmd);

    RS_DEBUG->print("QG_ActionHandler::command: trigger command event in "
                    " graphic view");
    view->commandEvent(&e);

    // if the current action can't deal with the command,
    //   it might be intended to launch a new command
    //    std::cout<<"QG_ActionHandler::command(): e.isAccepted()="<<e.isAccepted()<<std::endl;
    if (!e.isAccepted()) {

        RS_DEBUG->print("QG_ActionHandler::command: convert cmd to action type");
        // command for new action:
        RS2::ActionType type = RS_COMMANDS->cmdToAction(cmd);
        if (type!=RS2::ActionNone) {
            RS_DEBUG->print("QG_ActionHandler::command: setting current action");
             //special handling, currently needed for snap actions
			if (!commandLineActions(type)){
                //not handled yet
                setCurrentAction(type);
            }
            RS_DEBUG->print("QG_ActionHandler::command: current action set");
            return true;
        }
    }else{
        return true;
    }

    RS_DEBUG->print("QG_ActionHandler::command: current action not set");
    return false;
}


//void QG_ActionHandler::slotFileNew() {
//	setCurrentAction(RS2::ActionFileNew);
//}

void QG_ActionHandler::slotFileNewTemplate() {
    setCurrentAction(RS2::ActionFileNewTemplate);
}

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

void QG_ActionHandler::slotFileExportMakerCam() {
    setCurrentAction(RS2::ActionFileExportMakerCam);
}

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

void QG_ActionHandler::slotOrderBottom() {
    setCurrentAction(RS2::ActionOrderBottom);
}

void QG_ActionHandler::slotOrderLower() {
    setCurrentAction(RS2::ActionOrderLower);
}

void QG_ActionHandler::slotOrderRaise() {
    setCurrentAction(RS2::ActionOrderRaise);
}

void QG_ActionHandler::slotOrderTop() {
    setCurrentAction(RS2::ActionOrderTop);
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

/*void QG_ActionHandler::slotDrawPoint() {
        setCurrentAction(RS2::ActionDrawPoint);
}*/

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

void QG_ActionHandler::slotDrawLineRectangle3Points() {
    setCurrentAction(RS2::ActionDrawRectangle3Points);
}

void QG_ActionHandler::slotDrawLineRectangle1Point() {
    setCurrentAction(RS2::ActionDrawRectangle1Point);
}

void QG_ActionHandler::slotDrawLineRectangle2Points(){
    setCurrentAction(RS2::ActionDrawRectangle2Points);
}

void QG_ActionHandler::slotDrawLineSnake() {
    setCurrentAction(RS2::ActionDrawSnakeLine);
}

void QG_ActionHandler::slotDrawLineSnakeX() {
    setCurrentAction(RS2::ActionDrawSnakeLineX);
}

void QG_ActionHandler::slotDrawLineSnakeY() {
    setCurrentAction(RS2::ActionDrawSnakeLineY);
}

void QG_ActionHandler::slotDrawLineAngleRel(){
    setCurrentAction(RS2::ActionDrawLineAngleRel);
}

void QG_ActionHandler::slotDrawLineOrthogonalRel(){
    setCurrentAction(RS2::ActionDrawLineOrthogonalRel);
}

void QG_ActionHandler::slotDrawLineOrthogonalTo(){
    setCurrentAction(RS2::ActionDrawLineFromPointToLine);
}


void QG_ActionHandler::slotDrawSliceDivideLine(){
    setCurrentAction(RS2::ActionDrawSliceDivideLine);
}

void QG_ActionHandler::slotDrawSliceDivideCircle(){
    setCurrentAction(RS2::ActionDrawSliceDivideCircle);
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

void QG_ActionHandler::slotDrawLineOrthTan() {
    setCurrentAction(RS2::ActionDrawLineOrthTan);
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

void QG_ActionHandler::slotPolylineEquidistant() {
    setCurrentAction(RS2::ActionPolylineEquidistant);
}
void QG_ActionHandler::slotPolylineSegment() {
    setCurrentAction(RS2::ActionPolylineSegment);
}

void QG_ActionHandler::slotDrawLinePolygon() {
    setCurrentAction(RS2::ActionDrawLinePolygonCenCor);
}

void QG_ActionHandler::slotDrawLinePolygon3() {           //20161223 added by txmy
    setCurrentAction(RS2::ActionDrawLinePolygonCenTan);
}

void QG_ActionHandler::slotDrawLinePolygon2() {
    setCurrentAction(RS2::ActionDrawLinePolygonCorCor);
}

void QG_ActionHandler::slotDrawCircle() {
    setCurrentAction(RS2::ActionDrawCircle);
}

void QG_ActionHandler::slotDrawCircleCross(){
    setCurrentAction(RS2::ActionDrawCross);
}

void QG_ActionHandler::slotDrawCircleCR() {
    setCurrentAction(RS2::ActionDrawCircleCR);
}

void QG_ActionHandler::slotDrawCircleByArc() {
    setCurrentAction(RS2::ActionDrawCircleByArc);
}

void QG_ActionHandler::slotDrawCircle2P() {
    setCurrentAction(RS2::ActionDrawCircle2P);
}

void QG_ActionHandler::slotDrawCircle2PR() {
    setCurrentAction(RS2::ActionDrawCircle2PR);
}

void QG_ActionHandler::slotDrawCircle3P() {
    setCurrentAction(RS2::ActionDrawCircle3P);
}

void QG_ActionHandler::slotDrawCircleTan1_2P() {
    setCurrentAction(RS2::ActionDrawCircleTan1_2P);
}

void QG_ActionHandler::slotDrawCircleTan2_1P() {
    setCurrentAction(RS2::ActionDrawCircleTan2_1P);
}

void QG_ActionHandler::slotDrawCircleParallel() {
    setCurrentAction(RS2::ActionDrawCircleParallel);
}

void QG_ActionHandler::slotDrawCircleInscribe() {
    setCurrentAction(RS2::ActionDrawCircleInscribe);
}

void QG_ActionHandler::slotDrawCircleTan2() {
    setCurrentAction(RS2::ActionDrawCircleTan2);
}
void QG_ActionHandler::slotDrawCircleTan3() {
    setCurrentAction(RS2::ActionDrawCircleTan3);
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

void QG_ActionHandler::slotDrawEllipseFociPoint() {
    setCurrentAction(RS2::ActionDrawEllipseFociPoint);
}
void QG_ActionHandler::slotDrawEllipse4Points() {
    setCurrentAction(RS2::ActionDrawEllipse4Points);
}
void QG_ActionHandler::slotDrawEllipseCenter3Points() {
    setCurrentAction(RS2::ActionDrawEllipseCenter3Points);
}
void QG_ActionHandler::slotDrawEllipseInscribe() {
    setCurrentAction(RS2::ActionDrawEllipseInscribe);
}
void QG_ActionHandler::slotDrawParabola4Points() {
    setCurrentAction(RS2::ActionDrawParabola4Points);
}
void QG_ActionHandler::slotDrawParabolaFD() {
    setCurrentAction(RS2::ActionDrawParabolaFD);
}
void QG_ActionHandler::slotDrawSpline() {
    setCurrentAction(RS2::ActionDrawSpline);
}

void QG_ActionHandler::slotDrawSplinePoints() {
    setCurrentAction(RS2::ActionDrawSplinePoints);
}

void QG_ActionHandler::slotDrawMText() {
    setCurrentAction(RS2::ActionDrawMText);
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

void QG_ActionHandler::slotDimArc() {
    setCurrentAction(RS2::ActionDimArc);
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
    setCurrentAction(RS2::ActionModifyDelete);
}

void QG_ActionHandler::slotModifyDeleteFree() {
    setCurrentAction(RS2::ActionModifyDeleteFree);
}

void QG_ActionHandler::slotModifyMove() {
    setCurrentAction(RS2::ActionModifyMove);
}

void QG_ActionHandler::slotModifyRevertDirection() {
	setCurrentAction(RS2::ActionModifyRevertDirection);
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
void QG_ActionHandler::slotModifyOffset() {
    setCurrentAction(RS2::ActionModifyOffset);
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

void QG_ActionHandler::slotSetSnaps(RS_SnapMode const& s) {
    RS_DEBUG->print("QG_ActionHandler::slotSetSnaps()");

    if(snap_toolbar) {
        RS_DEBUG->print("QG_ActionHandler::slotSetSnaps(): set snapToolBar");
        snap_toolbar->setSnaps(s);
    }else{
        RS_DEBUG->print("QG_ActionHandler::slotSetSnaps(): snapToolBar is nullptr");
    }
    if(view) {
        view->setDefaultSnapMode(s);
    }
    RS_DEBUG->print("QG_ActionHandler::slotSetSnaps(): ok");
}

void QG_ActionHandler::slotSnapFree() {
//    if ( snapFree == nullptr) return;
//    disableSnaps();
    RS_SnapMode s=getSnaps();
    s.snapFree = !s.snapFree;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSnapGrid() {
//    if(snapGrid==nullptr) return;
    RS_SnapMode s=getSnaps();
    s.snapGrid = !s.snapGrid;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSnapEndpoint() {
//    if(snapEndpoint==nullptr) return;
    RS_SnapMode s=getSnaps();
    s.snapEndpoint = !s.snapEndpoint;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSnapOnEntity() {
//    if(snapOnEntity==nullptr) return;
    RS_SnapMode s=getSnaps();
    s.snapOnEntity = !s.snapOnEntity;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSnapCenter() {
//    std::cout<<" QG_ActionHandler::slotSnapCenter(): start"<<std::endl;
//    if(snapCenter==nullptr) return;
    RS_SnapMode s=getSnaps();
    s.snapCenter = !s.snapCenter;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSnapMiddle() {
    RS_SnapMode s=getSnaps();
    s.snapMiddle = !s.snapMiddle;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSnapDist() {
    RS_SnapMode s=getSnaps();
    s.snapDistance = !s.snapDistance;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSnapIntersection() {
    RS_SnapMode s=getSnaps();
    s.snapIntersection = !s.snapIntersection;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSnapIntersectionManual() {
    //disableSnaps();
	/*if (snapIntersectionManual) {
        snapIntersectionManual->setChecked(true);
}*/
	/*if (snapToolBar) {
        snapToolBar->setSnapMode(RS2::SnapIntersectionManual);
}*/
    //setCurrentAction(RS2::ActionSnapIntersectionManual);
}

void QG_ActionHandler::slotSnapMiddleManual(){
    if (getCurrentAction()->rtti() == RS2::ActionSnapMiddleManual)    {
        getCurrentAction()->init(-1);
        return;
    }

    const RS_Pen currentAppPen { document->getActivePen() };
    const RS_Pen snapMiddleManual_pen { RS_Pen(RS_Color(255,0,0), RS2::Width01, RS2::DashDotLineTiny) };
    document->setActivePen(snapMiddleManual_pen);
    auto a = new LC_ActionSnapMiddleManual(*document, *view, currentAppPen);
    connect(a, &LC_ActionSnapMiddleManual::signalUnsetSnapMiddleManual, snap_toolbar, &QG_SnapToolBar::slotUnsetSnapMiddleManual);
    view->setCurrentAction(a);
}

void QG_ActionHandler::disableSnaps() {
    slotSetSnaps(RS_SnapMode());
}

void QG_ActionHandler::slotRestrictNothing() {
    RS_SnapMode s = getSnaps();
    s.restriction = RS2::RestrictNothing;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotRestrictOrthogonal() {
    RS_SnapMode s = getSnaps();
    s.restriction = RS2::RestrictOrthogonal;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotRestrictHorizontal() {
    RS_SnapMode s = getSnaps();
    s.restriction = RS2::RestrictHorizontal;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotRestrictVertical() {
    RS_SnapMode s = getSnaps();
    s.restriction = RS2::RestrictVertical;
    slotSetSnaps(s);
}

// find snap restriction from menu
RS2::SnapRestriction QG_ActionHandler::getSnapRestriction(){
    return getSnaps().restriction;
}

void QG_ActionHandler::disableRestrictions() {
    RS_SnapMode s=getSnaps();
    s.restriction= RS2::RestrictNothing;
    slotSetSnaps(s);
}

void QG_ActionHandler::slotSetRelativeZero() {
    setCurrentAction(RS2::ActionSetRelativeZero);
}

void QG_ActionHandler::slotLockRelativeZero(bool on)
{
	if (snap_toolbar) {
        snap_toolbar->setLockedRelativeZero(on);
    }
    if (on) {
        setCurrentAction(RS2::ActionLockRelativeZero);
    } else {
        setCurrentAction(RS2::ActionUnlockRelativeZero);
    }
}

void QG_ActionHandler::slotInfoInside() {
    setCurrentAction(RS2::ActionInfoInside);
}

void QG_ActionHandler::slotInfoDist() {
    setCurrentAction(RS2::ActionInfoDistPoint2Point);
}

void QG_ActionHandler::slotInfoDist2() {
    setCurrentAction(RS2::ActionInfoDistEntity2Point);
}

void QG_ActionHandler::slotInfoDist3() {
    setCurrentAction(RS2::ActionInfoDistPoint2Entity);
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

void QG_ActionHandler::slotEntityInfo() {
    setCurrentAction(RS2::ActionInfoProperties);
}

void QG_ActionHandler::slotPickCoordinates() {
    setCurrentAction(RS2::ActionInfoPickCoordinates);
}

void QG_ActionHandler::slotLayersDefreezeAll() {
    setCurrentAction(RS2::ActionLayersDefreezeAll);
}

void QG_ActionHandler::slotLayersFreezeAll() {
    setCurrentAction(RS2::ActionLayersFreezeAll);
}

void QG_ActionHandler::slotLayersUnlockAll() {
    setCurrentAction(RS2::ActionLayersUnlockAll);
}

void QG_ActionHandler::slotLayersLockAll() {
    setCurrentAction(RS2::ActionLayersLockAll);
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

void QG_ActionHandler::slotLayersTogglePrint() {
    setCurrentAction(RS2::ActionLayersTogglePrint);
}

void QG_ActionHandler::slotLayersToggleConstruction() {
    setCurrentAction(RS2::ActionLayersToggleConstruction);
}

void QG_ActionHandler::slotLayersExportSelected() {
    setCurrentAction(RS2::ActionLayersExportSelected);
}

void QG_ActionHandler::slotLayersExportVisible() {
    setCurrentAction(RS2::ActionLayersExportVisible);
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

void QG_ActionHandler::slotBlocksSave() {
    setCurrentAction(RS2::ActionBlocksSave);
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

void QG_ActionHandler::slotModifyLineJoin() {
    setCurrentAction(RS2::ActionModifyLineJoin);
}

void QG_ActionHandler::slotModifyDuplicate() {
    setCurrentAction(RS2::ActionModifyDuplicate);
}

void QG_ActionHandler::slotDrawStar() {
    setCurrentAction(RS2::ActionDrawStar);
}

void QG_ActionHandler::slotModifyBreakDivide() {
    setCurrentAction(RS2::ActionModifyBreakDivide);
}

void QG_ActionHandler::slotModifyLineGap() {
    setCurrentAction(RS2::ActionModifyLineGap);
}

void QG_ActionHandler::slotOptionsDrawing() {
    setCurrentAction(RS2::ActionOptionsDrawing);
}

void QG_ActionHandler::slotPenPick(){
    setCurrentAction(RS2::ActionPenPick);
}

void QG_ActionHandler::slotPenPickResolved(){
    setCurrentAction(RS2::ActionPenPickResolved);
}

void QG_ActionHandler::slotPenApply(){
    setCurrentAction(RS2::ActionPenApply);
}

void QG_ActionHandler::slotPenCopy(){
    setCurrentAction(RS2::ActionPenCopy);
}

void QG_ActionHandler::slotPenSyncFromLayer(){
    setCurrentAction(RS2::ActionPenSyncFromLayer);
}

void QG_ActionHandler::set_view(RS_GraphicView* gview){
    view = gview;
}

void QG_ActionHandler::set_document(RS_Document* doc){
    document = doc;
}

void QG_ActionHandler::set_snap_toolbar(QG_SnapToolBar* snap_tb){
    snap_toolbar = snap_tb;
}

void QG_ActionHandler::toggleVisibility(RS_Layer* layer){
    auto a = new RS_ActionLayersToggleView(*document, *view, layer);
    view->setCurrentAction(a);
}

void QG_ActionHandler::toggleLock(RS_Layer* layer){
    auto a = new RS_ActionLayersToggleLock(*document, *view, layer);
    view->setCurrentAction(a);
}

void QG_ActionHandler::togglePrint(RS_Layer* layer){
    auto a = new RS_ActionLayersTogglePrint(*document, *view, layer);
    view->setCurrentAction(a);
}
void QG_ActionHandler::toggleConstruction(RS_Layer* layer){
    auto a = new LC_ActionLayersToggleConstruction(*document, *view, layer);
    view->setCurrentAction(a);
}



void QG_ActionHandler::slotDrawLinePoints() {
    setCurrentAction(RS2::ActionDrawLinePoints);
}
