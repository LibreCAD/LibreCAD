/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_actionhandlerfactory.h"

#include "lc_actioncontext.h"
#include "lc_actiondimarc.h"
#include "lc_actiondimordinate.h"
#include "lc_actiondimordinaterebase.h"
#include "lc_actiondrawarc2pointsangle.h"
#include "lc_actiondrawarc2pointsheight.h"
#include "lc_actiondrawarc2pointslength.h"
#include "lc_actiondrawarc2pointsradius.h"
#include "lc_actiondrawboundingbox.h"
#include "lc_actiondrawcircle2pr.h"
#include "lc_actiondrawcirclebyarc.h"
#include "lc_actiondrawcross.h"
#include "lc_actiondrawdimbaseline.h"
#include "lc_actiondrawellipse1point.h"
#include "lc_actiondrawlineanglerel.h"
#include "lc_actiondrawlinefrompointtoline.h"
#include "lc_actiondrawlinepoints.h"
#include "lc_actiondrawlinepolygon3.h"
#include "lc_actiondrawlinepolygon4.h"
#include "lc_actiondrawlinesnake.h"
#include "lc_actiondrawmidline.h"
#include "lc_actiondrawparabola4points.h"
#include "lc_actiondrawparabolaFD.h"
#include "lc_actiondrawpointslattice.h"
#include "lc_actiondrawrectangle1point.h"
#include "lc_actiondrawrectangle2points.h"
#include "lc_actiondrawrectangle3points.h"
#include "lc_actiondrawslicedivide.h"
#include "lc_actiondrawsplinepoints.h"
#include "lc_actiondrawstar.h"
#include "lc_actioneditpastetransform.h"
#include "lc_actionentitylayertoggle.h"
#include "lc_actionfileexportmakercam.h"
#include "lc_actioninfo3pointsangle.h"
#include "lc_actioninfopickcoordinates.h"
#include "lc_actioninfoproperties.h"
#include "lc_actionlayerscmd.h"
#include "lc_actionlayersexport.h"
#include "lc_actionlayerstoggleconstruction.h"
#include "lc_actionmodifyalign.h"
#include "lc_actionmodifyalignref.h"
#include "lc_actionmodifyalignsingle.h"
#include "lc_actionmodifybreakdivide.h"
#include "lc_actionmodifyduplicate.h"
#include "lc_actionmodifylinegap.h"
#include "lc_actionmodifylinejoin.h"
#include "lc_actionpastetopoints.h"
#include "lc_actionpenapply.h"
#include "lc_actionpenpick.h"
#include "lc_actionpensyncactivebylayer.h"
#include "lc_actionpolylinearcstolines.h"
#include "lc_actionpolylinechangesegmenttype.h"
#include "lc_actionremovesplinepoints.h"
#include "lc_actionselectdimordinatesameorigin.h"
#include "lc_actionselectpoints.h"
#include "lc_actionsnapmiddlemanual.h"
#include "lc_actionsplineaddpoint.h"
#include "lc_actionsplineappendpoint.h"
#include "lc_actionsplineexplode.h"
#include "lc_actionsplinefrompolyline.h"
#include "lc_actionsplineremovebetween.h"
#include "lc_actionucsbydimordinate.h"
#include "lc_actionucscreate.h"
#include "rs_actionblocksadd.h"
#include "rs_actionblocksattributes.h"
#include "rs_actionblockscreate.h"
#include "rs_actionblocksedit.h"
#include "rs_actionblocksexplode.h"
#include "rs_actionblocksfreezeall.h"
#include "rs_actionblocksinsert.h"
#include "rs_actionblocksremove.h"
#include "rs_actionblockssave.h"
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
#include "rs_actiondrawcircleinscribe.h"
#include "rs_actiondrawcircletan1_2p.h"
#include "rs_actiondrawcircletan2.h"
#include "rs_actiondrawcircletan2_1p.h"
#include "rs_actiondrawcircletan3.h"
#include "rs_actiondrawellipse4points.h"
#include "rs_actiondrawellipseaxis.h"
#include "rs_actiondrawellipsecenter3points.h"
#include "rs_actiondrawellipsefocipoint.h"
#include "rs_actiondrawellipseinscribe.h"
#include "rs_actiondrawhatch.h"
#include "rs_actiondrawimage.h"
#include "rs_actiondrawline.h"
#include "rs_actiondrawlineangle.h"
#include "rs_actiondrawlinebisector.h"
#include "rs_actiondrawlinefree.h"
#include "rs_actiondrawlinehorvert.h"
#include "rs_actiondrawlineorthtan.h"
#include "rs_actiondrawlineparallel.h"
#include "rs_actiondrawlineparallelthrough.h"
#include "rs_actiondrawlinepolygon.h"
#include "rs_actiondrawlinepolygon2.h"
#include "rs_actiondrawlinerectangle.h"
#include "rs_actiondrawlinerelangle.h"
#include "rs_actiondrawlinetangent1.h"
#include "rs_actiondrawlinetangent2.h"
#include "rs_actiondrawmtext.h"
#include "rs_actiondrawpoint.h"
#include "rs_actiondrawpolyline.h"
#include "rs_actiondrawspline.h"
#include "rs_actiondrawtext.h"
#include "rs_actioneditcopy.h"
#include "rs_actioneditundo.h"
#include "rs_actioninfoangle.h"
#include "rs_actioninfoarea.h"
#include "rs_actioninfodist.h"
#include "rs_actioninfodist2.h"
#include "rs_actioninfoinside.h"
#include "rs_actioninfototallength.h"
#include "rs_actioninterface.h"
#include "rs_actionlayersadd.h"
#include "rs_actionlayersedit.h"
#include "rs_actionlayersfreezeall.h"
#include "rs_actionlayerslockall.h"
#include "rs_actionlayersremove.h"
#include "rs_actionlayerstogglelock.h"
#include "rs_actionlayerstoggleprint.h"
#include "rs_actionlayerstoggleview.h"
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
#include "rs_actionmodifyoffset.h"
#include "rs_actionmodifyrevertdirection.h"
#include "rs_actionmodifyrotate.h"
#include "rs_actionmodifyrotate2.h"
#include "rs_actionmodifyround.h"
#include "rs_actionmodifyscale.h"
#include "rs_actionmodifystretch.h"
#include "rs_actionmodifytrim.h"
#include "rs_actionmodifytrimamount.h"
#include "rs_actionoptionsdrawing.h"
#include "rs_actionorder.h"
#include "rs_actionpolylineadd.h"
#include "rs_actionpolylineappend.h"
#include "rs_actionpolylinedel.h"
#include "rs_actionpolylinedelbetween.h"
#include "rs_actionpolylineequidistant.h"
#include "rs_actionpolylinesegment.h"
#include "rs_actionpolylinetrim.h"
#include "rs_actionselectall.h"
#include "rs_actionselectcontour.h"
#include "rs_actionselectintersected.h"
#include "rs_actionselectinvert.h"
#include "rs_actionselectlayer.h"
#include "rs_actionselectsingle.h"
#include "rs_actionselectwindow.h"
#include "rs_actionsetrelativezero.h"
#include "rs_actiontoolregeneratedimensions.h"
#include "rs_actionzoomauto.h"
#include "rs_actionzoomin.h"
#include "rs_actionzoompan.h"
#include "rs_actionzoomprevious.h"
#include "rs_actionzoomredraw.h"
#include "rs_actionzoomwindow.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphicview.h"
#include "rs_layerlist.h"
#include "rs_selection.h"

RS_Layer* obtainLayer(LC_ActionContext* m_actionContext, void* data) {
    RS_Layer* layer{nullptr};
    if (data != nullptr) {
        layer = static_cast<RS_Layer*>(data);
    }
    else {
        RS_Document* document = m_actionContext->getEntityContainer()->getDocument();
        layer = (document->getLayerList() != nullptr) ? document->getLayerList()->getActive() : nullptr;
    }
    return layer;
}

bool hasSelection(LC_ActionContext* m_actionContext) {
    return m_actionContext->getEntityContainer()->countSelected() > 0;
    // fixme - sand - think about moving to the action context
}

bool hasNoSelection(LC_ActionContext* m_actionContext) {
    return m_actionContext->getEntityContainer()->countSelected() == 0;
    // fixme - sand - think about moving to the action context
}

namespace InnerFactory{
    RS_ActionInterface* doCreateActionInstance(RS2::ActionType actionType, LC_ActionContext* ctx, void* data) {
        auto view = ctx->getGraphicView();
        switch (actionType) {
            case RS2::ActionEditKillAllActions: {
                if (view != nullptr) {
                    // DO we need to call some form of a 'clean' function?
                    view->killAllActions();
                    auto document = ctx->getEntityContainer();
                    RS_Selection s(static_cast<RS_EntityContainer&>(*document), view->getViewPort());
                    s.selectAll(false);
                    RS_DIALOGFACTORY->updateSelectionWidget(document->countSelected(), document->totalSelectedLength());
                }
                return nullptr;
            }
            case RS2::ActionEditUndo: {
                //to avoid operation on deleted entities, Undo action invalid all suspended
                //actions
                view->killAllActions();
                return new RS_ActionEditUndo(true, ctx);
            }
            case RS2::ActionEditRedo: {
                return new RS_ActionEditUndo(false, ctx);
            }
            case RS2::ActionEditCut: {
                return new RS_ActionEditCopyPaste(ctx, RS2::ActionEditCut);
            }
            case RS2::ActionEditCutQuick: {
                return new RS_ActionEditCopyPaste(ctx, RS2::ActionEditCutQuick);
            }
            case RS2::ActionEditCopy: {
                return new RS_ActionEditCopyPaste(ctx, RS2::ActionEditCopy);
            }
            case RS2::ActionEditCopyQuick: {
                return new RS_ActionEditCopyPaste(ctx, RS2::ActionEditCopyQuick);
            }
            case RS2::ActionEditPaste: {
                return new RS_ActionEditCopyPaste(ctx, RS2::ActionEditPaste);
            }
            case RS2::ActionEditPasteTransform: {
                return new LC_ActionEditPasteTransform(ctx);
            }
            case RS2::ActionPasteToPoints: {
                return new LC_ActionPasteToPoints(ctx);
            }
            case RS2::ActionOrderBottom: {
                return new RS_ActionOrder(ctx, RS2::ActionOrderBottom);
            }
            case RS2::ActionOrderLower: {
                return new RS_ActionOrder(ctx, RS2::ActionOrderLower);
            }
            case RS2::ActionOrderRaise: {
                return new RS_ActionOrder(ctx, RS2::ActionOrderRaise);
            }
            case RS2::ActionOrderTop: {
                return new RS_ActionOrder(ctx, RS2::ActionOrderTop);
            }
            case RS2::ActionSelectSingle: {
                // fixme - sand - files - that's suspicious implementation, check it again for plugins
                RS_ActionInterface* currentAction = ctx->getCurrentAction();
                if (currentAction->rtti() != RS2::ActionSelectSingle) {
                    return new RS_ActionSelectSingle(ctx, currentAction);
                }
                break;
            }
            case RS2::ActionSelectContour: {
                view->killSelectActions();
                return new RS_ActionSelectContour(ctx);
            }
            case RS2::ActionSelectAll: {
                return new RS_ActionSelectAll(ctx, true);
            }
            case RS2::ActionDeselectAll: {
                return new RS_ActionSelectAll(ctx, false);
            }
            case RS2::ActionSelectWindow: {
                view->killSelectActions();
                return new RS_ActionSelectWindow(view->getTypeToSelect(), ctx, true);
            }
            case RS2::ActionSelectPoints: {
                view->killSelectActions();
                return new LC_ActionSelectPoints(ctx);
            }
            case RS2::ActionDeselectWindow: {
                view->killSelectActions();
                return new RS_ActionSelectWindow(ctx, false);
            }
            case RS2::ActionSelectInvert: {
                return new RS_ActionSelectInvert(ctx);
            }
            case RS2::ActionSelectIntersected: {
                view->killSelectActions();
                return new RS_ActionSelectIntersected(ctx, true);
            }
            case RS2::ActionDeselectIntersected: {
                view->killSelectActions();
                return new RS_ActionSelectIntersected(ctx, false);
            }
            case RS2::ActionSelectLayer: {
                view->killSelectActions();
                return new RS_ActionSelectLayer(ctx);
            }
            case RS2::ActionToolRegenerateDimensions: {
                return new RS_ActionToolRegenerateDimensions(ctx);
            }
            case RS2::ActionZoomIn: {
                return new RS_ActionZoomIn(ctx, RS2::In, RS2::Both);
            }
            case RS2::ActionZoomOut: {
                return new RS_ActionZoomIn(ctx, RS2::Out, RS2::Both);
            }
            case RS2::ActionZoomAuto: {
                return new RS_ActionZoomAuto(ctx);
            }
            case RS2::ActionZoomWindow: {
                return new RS_ActionZoomWindow(ctx);
            }
            case RS2::ActionZoomPan: {
                return new RS_ActionZoomPan(ctx);
            }
            case RS2::ActionZoomPrevious: {
                return new RS_ActionZoomPrevious(ctx);
            }
            case RS2::ActionZoomRedraw: {
                return new RS_ActionZoomRedraw(ctx);
            }
            case RS2::ActionDrawPoint: {
                return new RS_ActionDrawPoint(ctx);
            }
            case RS2::ActionDrawLine: {
                return new RS_ActionDrawLine(ctx);
            }
            case RS2::ActionDrawLineAngle: {
                return new RS_ActionDrawLineAngle(ctx, false);
            }
            case RS2::ActionDrawLineHorizontal: {
                return new RS_ActionDrawLineAngle(ctx, true,RS2::ActionDrawLineHorizontal);
            }
            case RS2::ActionDrawLineHorVert: {
                return new RS_ActionDrawLineHorVert(ctx);
            }
            case RS2::ActionDrawLineVertical: {
                return new RS_ActionDrawLineAngle(ctx, true,RS2::ActionDrawLineVertical);
            }
            case RS2::ActionDrawLineFree: {
                return new RS_ActionDrawLineFree(ctx);
            }
            case RS2::ActionDrawLineParallel: {
                return new RS_ActionDrawLineParallel(ctx, RS2::ActionDrawLineParallel);
            }
            case RS2::ActionDrawCircleParallel: {
                return new RS_ActionDrawLineParallel(ctx, RS2::ActionDrawCircleParallel);
            }
            case RS2::ActionDrawArcParallel: {
                return new RS_ActionDrawLineParallel(ctx, RS2::ActionDrawArcParallel);
            }
            case RS2::ActionDrawLineParallelThrough: {
                return new RS_ActionDrawLineParallelThrough(ctx);
            }
            case RS2::ActionDrawLineRectangle: {
                return new RS_ActionDrawLineRectangle(ctx);
            }
            case RS2::ActionDrawRectangle3Points: {
                return new LC_ActionDrawRectangle3Points(ctx);
            }
            case RS2::ActionDrawRectangle2Points: {
                return new LC_ActionDrawRectangle2Points(ctx);
            }
            case RS2::ActionDrawRectangle1Point: {
                return new LC_ActionDrawRectangle1Point(ctx);
            }
            case RS2::ActionDrawCross: {
                return new LC_ActionDrawCross(ctx);
            }
            case RS2::ActionDrawBoundingBox: {
                return new LC_ActionDrawBoundingBox(ctx);
            }
            case RS2::ActionDrawSnakeLine: {
                return new LC_ActionDrawLineSnake(ctx, RS2::ActionDrawSnakeLine);
            }
            case RS2::ActionDrawSnakeLineX: {
                return new LC_ActionDrawLineSnake(ctx, RS2::ActionDrawSnakeLineX);
            }
            case RS2::ActionDrawSnakeLineY: {
                return new LC_ActionDrawLineSnake(ctx, RS2::ActionDrawSnakeLineY);
            }
            case RS2::ActionDrawSliceDivideLine: {
                return new LC_ActionDrawSliceDivide(ctx, false);
            }
            case RS2::ActionDrawSliceDivideCircle: {
                return new LC_ActionDrawSliceDivide(ctx, true);
            }
            case RS2::ActionDrawLinePoints: {
                return new LC_ActionDrawLinePoints(ctx, false);
            }
            case RS2::ActionDrawPointsMiddle: {
                return new LC_ActionDrawLinePoints(ctx, true);
            }
            case RS2::ActionDrawPointsLattice: {
                return new LC_ActionDrawPointsLattice(ctx);
            }
            case RS2::ActionDrawLineBisector: {
                return new RS_ActionDrawLineBisector(ctx);
            }
            case RS2::ActionDrawLineOrthTan: {
                return new RS_ActionDrawLineOrthTan(ctx);
            }
            case RS2::ActionDrawLineTangent1: {
                return new RS_ActionDrawLineTangent1(ctx);
            }
            case RS2::ActionDrawLineTangent2: {
                return new RS_ActionDrawLineTangent2(ctx);
            }
            case RS2::ActionDrawLineOrthogonal: {
                return new RS_ActionDrawLineRelAngle(ctx, M_PI_2, true);
            }
            case RS2::ActionDrawLineRelAngle: {
                return new RS_ActionDrawLineRelAngle(ctx, M_PI_2, false);
            }
            case RS2::ActionDrawPolyline: {
                return new RS_ActionDrawPolyline(ctx);
            }
            case RS2::ActionDrawLineOrthogonalRel: {
                return new LC_ActionDrawLineAngleRel(ctx, 90.0, true);
            }
            case RS2::ActionDrawLineAngleRel: {
                return new LC_ActionDrawLineAngleRel(ctx, 0.0, false);
            }
            case RS2::ActionDrawLineFromPointToLine: {
                return new LC_ActionDrawLineFromPointToLine(ctx);
            }
            case RS2::ActionDrawLineMiddle: {
                return new LC_ActionDrawMidLine(ctx);
            }
            case RS2::ActionDrawStar: {
                return new LC_ActionDrawStar(ctx);
            }
            case RS2::ActionPolylineAdd: {
                return new RS_ActionPolylineAdd(ctx);
            }
            case RS2::ActionPolylineAppend: {
                return new RS_ActionPolylineAppend(ctx);
            }
            case RS2::ActionPolylineDel: {
                return new RS_ActionPolylineDel(ctx);
            }
            case RS2::ActionPolylineDelBetween: {
                return new RS_ActionPolylineDelBetween(ctx);
            }
            case RS2::ActionPolylineTrim: {
                return new RS_ActionPolylineTrim(ctx);
            }
            case RS2::ActionPolylineEquidistant: {
                return new RS_ActionPolylineEquidistant(ctx);
            }
            case RS2::ActionPolylineSegment: {
                if (data == nullptr) {
                    return new RS_ActionPolylineSegment(ctx);
                }
                else {
                    return new RS_ActionPolylineSegment(ctx, static_cast<RS_Entity*>(data));
                }
            }
            case RS2::ActionPolylineArcsToLines: {
                return new LC_ActionPolylineArcsToLines(ctx);
            }
            case RS2::ActionPolylineChangeSegmentType: {
                return new LC_ActionPolylineChangeSegmentType(ctx);
            }
            case RS2::ActionDrawLinePolygonCenCor: {
                return new RS_ActionDrawLinePolygonCenCor(ctx);
            }
            case RS2::ActionDrawLinePolygonCenTan: {
                //20161223 added by txmy
                return new LC_ActionDrawLinePolygonCenTan(ctx);
            }
            case RS2::ActionDrawLinePolygonSideSide: {
                return new LC_ActionDrawLinePolygon4(ctx);
            }
            case RS2::ActionDrawLinePolygonCorCor: {
                return new RS_ActionDrawLinePolygonCorCor(ctx);
            }
            case RS2::ActionDrawCircle: {
                return new RS_ActionDrawCircle(ctx);
            }
            case RS2::ActionDrawCircleCR: {
                return new RS_ActionDrawCircleCR(ctx);
            }
            case RS2::ActionDrawCircleByArc: {
                return new LC_ActionDrawCircleByArc(ctx);
            }
            case RS2::ActionDrawCircle2P: {
                return new RS_ActionDrawCircle2P(ctx);
            }
            case RS2::ActionDrawCircle2PR: {
                return new LC_ActionDrawCircle2PR(ctx);
            }
            case RS2::ActionDrawCircle3P: {
                return new RS_ActionDrawCircle3P(ctx);
            }
            case RS2::ActionDrawCircleTan1_2P: {
                return new RS_ActionDrawCircleTan1_2P(ctx);
            }
            case RS2::ActionDrawCircleTan2_1P: {
                return new RS_ActionDrawCircleTan2_1P(ctx);
            }
            case RS2::ActionDrawCircleInscribe: {
                return new RS_ActionDrawCircleInscribe(ctx);
            }
            case RS2::ActionDrawCircleTan2: {
                return new RS_ActionDrawCircleTan2(ctx);
            }
            case RS2::ActionDrawCircleTan3: {
                return new RS_ActionDrawCircleTan3(ctx);
            }
            case RS2::ActionDrawArc: {
                return new RS_ActionDrawArc(ctx, RS2::ActionDrawArc);
            }
            case RS2::ActionDrawArcChord: {
                return new RS_ActionDrawArc(ctx, RS2::ActionDrawArcChord);
            }
            case RS2::ActionDrawArcAngleLen: {
                return new RS_ActionDrawArc(ctx, RS2::ActionDrawArcAngleLen);
            }
            case RS2::ActionDrawArc3P: {
                return new RS_ActionDrawArc3P(ctx);
            }
            case RS2::ActionDrawArcTangential: {
                return new RS_ActionDrawArcTangential(ctx);
            }
            case RS2::ActionDrawArc2PRadius: {
                return new LC_ActionDrawArc2PointsRadius(ctx);
            }
            case RS2::ActionDrawArc2PAngle: {
                return new LC_ActionDrawArc2PointsAngle(ctx);
            }
            case RS2::ActionDrawArc2PHeight: {
                return new LC_ActionDrawArc2PointsHeight(ctx);
            }
            case RS2::ActionDrawArc2PLength: {
                return new LC_ActionDrawArc2PointsLength(ctx);
            }
            case RS2::ActionDrawEllipseAxis: {
                return new RS_ActionDrawEllipseAxis(ctx, false);
            }
            case RS2::ActionDrawEllipseArcAxis: {
                return new RS_ActionDrawEllipseAxis(ctx, true);
            }
            case RS2::ActionDrawEllipse1Point: {
                return new LC_ActionDrawEllipse1Point(ctx, false);
            }
            case RS2::ActionDrawEllipseArc1Point: {
                return new LC_ActionDrawEllipse1Point(ctx, true);
            }
            case RS2::ActionDrawParabola4Points: {
                return new LC_ActionDrawParabola4Points(ctx);
            }
            case RS2::ActionDrawParabolaFD: {
                return new LC_ActionDrawParabolaFD(ctx);
            }
            case RS2::ActionDrawEllipseFociPoint: {
                return new RS_ActionDrawEllipseFociPoint(ctx);
            }
            case RS2::ActionDrawEllipse4Points: {
                return new RS_ActionDrawEllipse4Points(ctx);
            }
            case RS2::ActionDrawEllipseCenter3Points: {
                return new RS_ActionDrawEllipseCenter3Points(ctx);
            }
            case RS2::ActionDrawEllipseInscribe: {
                return new RS_ActionDrawEllipseInscribe(ctx);
            }
            case RS2::ActionDrawSpline: {
                return new RS_ActionDrawSpline(ctx);
            }
            case RS2::ActionDrawSplinePoints: {
                return new LC_ActionDrawSplinePoints(ctx);
            }
            case RS2::ActionDrawSplinePointRemove: {
                return new LC_ActionRemoveSplinePoints(ctx);
            }
            case RS2::ActionDrawSplinePointDelTwo: {
                return new LC_ActionSplineRemoveBetween(ctx);
            }
            case RS2::ActionDrawSplinePointAppend: {
                return new LC_ActionSplineAppendPoint(ctx);
            }
            case RS2::ActionDrawSplinePointAdd: {
                return new LC_ActionSplineAddPoint(ctx);
            }
            case RS2::ActionDrawSplineExplode: {
                return new LC_ActionSplineExplode(ctx);
            }
            case RS2::ActionDrawSplineFromPolyline: {
                return new LC_ActionSplineFromPolyline(ctx);
            }
            case RS2::ActionDrawMText: {
                return new RS_ActionDrawMText(ctx);
            }
            case RS2::ActionDrawText: {
                return new RS_ActionDrawText(ctx);
            }
            case RS2::ActionDrawHatch: {
                return new RS_ActionDrawHatch(ctx);
            }
            case RS2::ActionDrawImage: {
                return new RS_ActionDrawImage(ctx);
            }
            case RS2::ActionDimAligned: {
                return new RS_ActionDimAligned(ctx);
            }
            case RS2::ActionDimLinear: {
                return new RS_ActionDimLinear(ctx);
            }
            case RS2::ActionDimOrdinate: {
                return new LC_ActionDimOrdinate(ctx);
            }
            case RS2::ActionDimLinearHor: {
                return new RS_ActionDimLinear(ctx, 0.0, true, RS2::ActionDimLinearHor);
            }
            case RS2::ActionDimLinearVer: {
                return new RS_ActionDimLinear(ctx, M_PI_2, true, RS2::ActionDimLinearVer);
            }
            case RS2::ActionDimRadial: {
                return new RS_ActionDimRadial(ctx);
            }
            case RS2::ActionDimDiametric: {
                return new RS_ActionDimDiametric(ctx);
            }
            case RS2::ActionDimAngular: {
                return new RS_ActionDimAngular(ctx);
            }
            case RS2::ActionDimArc: {
                return new LC_ActionDimArc(ctx);
            }
            case RS2::ActionDimLeader: {
                return new RS_ActionDimLeader(ctx);
            }
            case RS2::ActionDimBaseline: {
                return new LC_ActionDrawDimBaseline(ctx, RS2::ActionDimBaseline);
            }
            case RS2::ActionDimContinue: {
                return new LC_ActionDrawDimBaseline(ctx, RS2::ActionDimContinue);
            }
            case RS2::ActionModifyLineJoin: {
                return new LC_ActionModifyLineJoin(ctx);
            }
            case RS2::ActionModifyDuplicate: {
                return new LC_ActionModifyDuplicate(ctx);
            }
            case RS2::ActionModifyBreakDivide: {
                return new LC_ActionModifyBreakDivide(ctx);
            }
            case RS2::ActionModifyLineGap: {
                return new LC_ActionModifyLineGap(ctx);
            }
            case RS2::ActionModifyAttributes: {
                return new RS_ActionModifyAttributes(ctx);
            }
            case RS2::ActionModifyDelete:
                [[fallthrough]];
            case RS2::ActionModifyDeleteQuick: {
                return new RS_ActionModifyDelete(ctx);
            }
            case RS2::ActionModifyDeleteFree: {
                return new RS_ActionModifyDeleteFree(ctx);
            }
            case RS2::ActionModifyMove: {
                return new RS_ActionModifyMove(ctx);
            }
            case RS2::ActionModifyRevertDirection: {
                return new RS_ActionModifyRevertDirection(ctx);
            }
            case RS2::ActionModifyRotate: {
                return new RS_ActionModifyRotate(ctx);
            }
            case RS2::ActionModifyScale: {
                return new RS_ActionModifyScale(ctx);
            }
            case RS2::ActionModifyMirror: {
                return new RS_ActionModifyMirror(ctx);
            }
            case RS2::ActionModifyMoveRotate: {
                return new RS_ActionModifyMoveRotate(ctx);
            }
            case RS2::ActionModifyRotate2: {
                return new RS_ActionModifyRotate2(ctx);
            }
            case RS2::ActionModifyEntity: {
                return new RS_ActionModifyEntity(ctx, static_cast<RS_Entity*>(data));
            }
            case RS2::ActionModifyTrim: {
                return new RS_ActionModifyTrim(ctx, false);
            }
            case RS2::ActionModifyTrim2: {
                return new RS_ActionModifyTrim(ctx, true);
            }
            case RS2::ActionModifyTrimAmount: {
                return new RS_ActionModifyTrimAmount(ctx);
            }
            case RS2::ActionModifyCut: {
                return new RS_ActionModifyCut(ctx);
            }
            case RS2::ActionModifyStretch: {
                return new RS_ActionModifyStretch(ctx);
            }
            case RS2::ActionModifyBevel: {
                return new RS_ActionModifyBevel(ctx);
            }
            case RS2::ActionModifyRound: {
                return new RS_ActionModifyRound(ctx);
            }
            case RS2::ActionModifyOffset: {
                return new RS_ActionModifyOffset(ctx);
            }
            case RS2::ActionModifyExplodeText: {
                return new RS_ActionModifyExplodeText(ctx);
            }
            case RS2::ActionModifyAlign: {
                return new LC_ActionModifyAlign(ctx);
            }
            case RS2::ActionModifyAlignOne: {
                return new LC_ActionModifyAlignSingle(ctx);
            }
            case RS2::ActionModifyAlignRef: {
                return new LC_ActionModifyAlignRef(ctx);
            }
            case RS2::ActionSetRelativeZero: {
                return new RS_ActionSetRelativeZero(ctx);
            }
            case RS2::ActionLockRelativeZero: {
                return new RS_ActionLockRelativeZero(ctx, true);
            }
            case RS2::ActionUnlockRelativeZero: {
                return new RS_ActionLockRelativeZero(ctx, false);
            }
            case RS2::ActionPenPick: {
                return new LC_ActionPenPick(ctx, false);
            }
            case RS2::ActionPenPickResolved: {
                return new LC_ActionPenPick(ctx, true);
            }
            case RS2::ActionPenApply: {
                return new LC_ActionPenApply(ctx, false);
            }
            case RS2::ActionPenCopy: {
                return new LC_ActionPenApply(ctx, true);
            }
            case RS2::ActionPenSyncFromLayer: {
                return new LC_ActionPenSyncActiveByLayer(ctx);
            }
            case RS2::ActionInfoInside: {
                return new RS_ActionInfoInside(ctx);
            }
            case RS2::ActionInfoDistPoint2Point: {
                return new RS_ActionInfoDist(ctx);
            }
            case RS2::ActionInfoDistEntity2Point: {
                return new RS_ActionInfoDist2(ctx);
            }
            case RS2::ActionInfoDistPoint2Entity: {
                return new RS_ActionInfoDist2(ctx, true);
            }
            case RS2::ActionInfoAngle: {
                return new RS_ActionInfoAngle(ctx);
            }
            case RS2::ActionInfoAngle3Points: {
                return new LC_ActionInfo3PointsAngle(ctx);
            }
            case RS2::ActionInfoTotalLength: {
                return new RS_ActionInfoTotalLength(ctx);
            }
            case RS2::ActionInfoArea: {
                return new RS_ActionInfoArea(ctx);
            }
            case RS2::ActionInfoProperties: {
                return new LC_ActionInfoProperties(ctx);
            }
            case RS2::ActionInfoPickCoordinates: {
                return new LC_ActionInfoPickCoordinates(ctx);
            }
            case RS2::ActionLayersDefreezeAll: {
                return new RS_ActionLayersFreezeAll(false, ctx);
            }
            case RS2::ActionLayersFreezeAll: {
                return new RS_ActionLayersFreezeAll(true, ctx);
            }
            case RS2::ActionLayersUnlockAll: {
                return new RS_ActionLayersLockAll(false, ctx);
            }
            case RS2::ActionLayersLockAll: {
                return new RS_ActionLayersLockAll(true, ctx);
            }
            case RS2::ActionLayersAdd: {
                return new RS_ActionLayersAdd(ctx);
            }
            case RS2::ActionLayersAddCmd: {
                return new LC_ActionLayersCmd(ctx, RS2::ActionLayersAddCmd);
            }
            case RS2::ActionLayersActivateCmd: {
                return new LC_ActionLayersCmd(ctx, RS2::ActionLayersActivateCmd);
            }
            case RS2::ActionLayersRemove: {
                return new RS_ActionLayersRemove(ctx);
            }
            case RS2::ActionLayersEdit: {
                return new RS_ActionLayersEdit(ctx);
            }
            case RS2::ActionLayersToggleView: {
                auto a_layer = obtainLayer(ctx, data);
                if (a_layer != nullptr) {
                    return new RS_ActionLayersToggleView(ctx, a_layer);
                }
                break;
            }
            case RS2::ActionLayersToggleLock: {
                auto a_layer = obtainLayer(ctx, data);
                if (a_layer != nullptr) {
                    return new RS_ActionLayersToggleLock(ctx, a_layer);
                }
                break;
            }
            case RS2::ActionLayersTogglePrint: {
                auto a_layer = obtainLayer(ctx, data);
                if (a_layer != nullptr) {
                    return new RS_ActionLayersTogglePrint(ctx, a_layer);
                }
                break;
            }
            case RS2::ActionLayersToggleConstruction: {
                auto a_layer = obtainLayer(ctx, data);
                if (a_layer != nullptr) {
                    return new LC_ActionLayersToggleConstruction(ctx, a_layer);
                }
                break;
            }
            case RS2::ActionLayersExportSelected: {
                return new LC_ActionLayersExport(ctx, LC_ActionLayersExport::SelectedMode);
            }
            case RS2::ActionLayersExportVisible: {
                return new LC_ActionLayersExport(ctx, LC_ActionLayersExport::VisibleMode);
            }
            case RS2::ActionBlocksDefreezeAll: {
                return new RS_ActionBlocksFreezeAll(false, ctx);
            }
            case RS2::ActionBlocksFreezeAll: {
                return new RS_ActionBlocksFreezeAll(true, ctx);
            }
            case RS2::ActionBlocksAdd: {
                return new RS_ActionBlocksAdd(ctx);
            }
            case RS2::ActionBlocksRemove: {
                return new RS_ActionBlocksRemove(ctx);
            }
            case RS2::ActionBlocksAttributes: {
                return new RS_ActionBlocksAttributes(ctx);
            }
            case RS2::ActionBlocksEdit: {
                return new RS_ActionBlocksEdit(ctx);
            }
            case RS2::ActionBlocksSave: {
                return new RS_ActionBlocksSave(ctx);
            }
            case RS2::ActionBlocksInsert: {
                return new RS_ActionBlocksInsert(ctx);
            }
            case RS2::ActionBlocksToggleView: {
                return new RS_ActionBlocksToggleView(ctx);
            }
            case RS2::ActionBlocksCreate: {
                return new RS_ActionBlocksCreate(ctx);
            }
            case RS2::ActionBlocksExplode: {
                return new RS_ActionBlocksExplode(ctx);
            }
            case RS2::ActionLibraryInsert: {
                return new RS_ActionLibraryInsert(ctx);
            }
            case RS2::ActionOptionsDrawing: {
                return new RS_ActionOptionsDrawing(ctx);
            }
            case RS2::ActionOptionsDrawingGrid: {
                return new RS_ActionOptionsDrawing(ctx, 2);
            }
            case RS2::ActionOptionsDrawingUnits: {
                return new RS_ActionOptionsDrawing(ctx, 1);
            }
            case RS2::ActionUCSCreate: {
                return new LC_ActionUCSCreate(ctx);
            }
            case RS2::ActionUCSSetByDimOrdinate: {
                return new LC_ActionUCSByDimOrdinate(ctx);
            }
            case RS2::ActionFileExportMakerCam: {
                return new LC_ActionFileExportMakerCam(ctx);
            }
            case RS2::ActionSnapMiddleManual: {
                auto currentAction = ctx->getCurrentAction();
                if (currentAction != nullptr) {
                    if (currentAction->rtti() == RS2::ActionSnapMiddleManual) {
                        currentAction->init(-1);
                        return nullptr;
                    }
                    return new LC_ActionSnapMiddleManual(ctx);
                }
                break;
            }
            case RS2::ActionLayerEntityActivate:
                [[fallthrough]];
            case RS2::ActionLayerEntityToggleView:
                [[fallthrough]];
            case RS2::ActionLayerEntityToggleConstruction:
                [[fallthrough]];
            case RS2::ActionLayerEntityTogglePrint:
                [[fallthrough]];
            case RS2::ActionLayerEntityToggleLock: {
                return new LC_ActionLayerToggle(ctx, actionType);
            }
            case RS2::ActionDimOrdinateSelectSameOrigin: {
                return new LC_ActionSelectDimOrdinateSameOrigin(ctx);
            }
            case RS2::ActionDimOrdinateRebase: {
                return new LC_ActionDimOrdinateRebase(ctx);
            }
            default:
                RS_DEBUG->print(RS_Debug::D_WARNING,
                                &"LC_ActionsHandlerFactory::createActionInstance: No such action found. Type "[
                                    actionType]);
                break;
        }
        return nullptr;
    }
}

/*
 * Usage of raw pointers in factory is intentional. The reason for this is as follows:  
 * 1) shared_ptr is actually just a template.
 * 2) the preprocessor expands that template, and since the amount of different classes wrapped by shared_ptr within the same
 * comiplation unit is big, the resulting file is also big - something like 20MB (maybe even bigger).
 * 3) While MSVC compiler handles this generated file, the GCC compiler fails after achieving some limiting amount of shared_ptr templates
 * (at least with MinGW), and returns the following compilation error
 *
 *
\generated\librecad\obj\lc_actionhandlerfactory.o ..\..\..\..\librecad\src\ui\lc_actionhandlerfactory.cpp
D:/Qt/QT6/Tools/mingw1120_64/bin/../lib/gcc/x86_64-w64-mingw32/11.2.0/../../../../x86_64-w64-mingw32/bin/as.exe: ..\..\generated\librecad\obj\lc_actionhandlerfactory.o: too many sections (38292)
C:\Users\sand1\AppData\Local\Temp\ccayHcyJ.s: Assembler messages:
C:\Users\sand1\AppData\Local\Temp\ccayHcyJ.s: Fatal error: can't write 42 bytes to section .text of ..\..\generated\librecad\obj\lc_actionhandlerfactory.o: 'file too big'
D:/Qt/QT6/Tools/mingw1120_64/bin/../lib/gcc/x86_64-w64-mingw32/11.2.0/../../../../x86_64-w64-mingw32/bin/as.exe: ..\..\generated\librecad\obj\lc_actionhandlerfactory.o: too many sections (38292)
C:\Users\sand1\AppData\Local\Temp\ccayHcyJ.s: Fatal error: ..\..\generated\librecad\obj\lc_actionhandlerfactory.o: file too big
mingw32-make[3]: *** [Makefile.Debug:88797: ../../generated/librecad/obj/lc_actionhandlerfactory.o] Error 1
*
*  So either this factory should be split on several ones, or wrapping of raw pointer to shared one is performed only once.
*  The later case is also fine, and shared_ptr to base type is returned and all actions has virtual destructors
*
*/
std::shared_ptr<RS_ActionInterface> LC_ActionsHandlerFactory::createActionInstance(
    RS2::ActionType actionType, LC_ActionContext* ctx, void* data) {
    RS_ActionInterface* actionInstance = InnerFactory::doCreateActionInstance(actionType, ctx, data);
    std::shared_ptr<RS_ActionInterface> result{actionInstance};
    return result;
}
