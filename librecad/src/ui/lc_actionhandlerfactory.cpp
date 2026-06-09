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

#include "lc_action_block_explode.h"
#include "lc_action_block_insert.h"
#include "lc_action_block_library_insert.h"
#include "lc_action_draw_arc_2points_angle.h"
#include "lc_action_draw_arc_2points_height.h"
#include "lc_action_draw_arc_2points_length.h"
#include "lc_action_draw_arc_2points_radius.h"
#include "lc_action_draw_arc_3points.h"
#include "lc_action_draw_arc_center_point_param.h"
#include "lc_action_draw_arc_tangential.h"
#include "lc_action_draw_bounding_box.h"
#include "lc_action_draw_center_line.h"
#include "lc_action_draw_center_mark.h"
#include "lc_action_draw_circle_2points.h"
#include "lc_action_draw_circle_2points_radius.h"
#include "lc_action_draw_circle_3points.h"
#include "lc_action_draw_circle_by_arc.h"
#include "lc_action_draw_circle_center_point.h"
#include "lc_action_draw_circle_center_radius.h"
#include "lc_action_draw_circle_inscribe.h"
#include "lc_action_draw_circle_tangental_1entity_2points.h"
#include "lc_action_draw_circle_tangental_2entities_1point.h"
#include "lc_action_draw_circle_tangental_2entities_radius.h"
#include "lc_action_draw_circle_tangental_3entities.h"
#include "lc_action_draw_ellipse_1point.h"
#include "lc_action_draw_ellipse_axis.h"
#include "lc_action_draw_image.h"
#include "lc_action_draw_line.h"
#include "lc_action_draw_line_angle.h"
#include "lc_action_draw_line_angle_rel.h"
#include "lc_action_draw_line_bisector.h"
#include "lc_action_draw_line_freehand.h"
#include "lc_action_draw_line_from_point_to_line.h"
#include "lc_action_draw_line_parallel.h"
#include "lc_action_draw_line_parallel_through.h"
#include "lc_action_draw_line_radiant.h"
#include "lc_action_draw_line_rel_angle.h"
#include "lc_action_draw_line_snake.h"
#include "lc_action_draw_mtext.h"
#include "lc_action_draw_point.h"
#include "lc_action_draw_points_lattice.h"
#include "lc_action_draw_points_line.h"
#include "lc_action_draw_polygon_center_corner.h"
#include "lc_action_draw_polygon_center_tan.h"
#include "lc_action_draw_polygon_corner_corner.h"
#include "lc_action_draw_polygon_side_side.h"
#include "lc_action_draw_polyline.h"
#include "lc_action_draw_rectangle_1point.h"
#include "lc_action_draw_rectangle_2points.h"
#include "lc_action_draw_rectangle_3points.h"
#include "lc_action_draw_rectangle_simple.h"
#include "lc_action_draw_slice_divide.h"
#include "lc_action_draw_spline.h"
#include "lc_action_draw_spline_points.h"
#include "lc_action_draw_star.h"
#include "lc_action_draw_text.h"
#include "lc_action_edit_copy_cut.h"
#include "lc_action_edit_paste_to_points.h"
#include "lc_action_edit_paste_transform.h"
#include "lc_action_edit_undo_redo.h"
#include "lc_action_info_dist_point_to_entity.h"
#include "lc_action_modify_align.h"
#include "lc_action_modify_align_ref.h"
#include "lc_action_modify_align_single.h"
#include "lc_action_modify_attributes.h"
#include "lc_action_modify_bevel.h"
#include "lc_action_modify_break_divide.h"
#include "lc_action_modify_cut.h"
#include "lc_action_modify_delete.h"
#include "lc_action_modify_delete_free.h"
#include "lc_action_modify_duplicate.h"
#include "lc_action_modify_entity.h"
#include "lc_action_modify_explode_text.h"
#include "lc_action_modify_line_gap.h"
#include "lc_action_modify_line_join.h"
#include "lc_action_modify_mirror.h"
#include "lc_action_modify_move.h"
#include "lc_action_modify_move_adjust.h"
#include "lc_action_modify_move_rotate.h"
#include "lc_action_modify_offset.h"
#include "lc_action_modify_order.h"
#include "lc_action_modify_revert_direction.h"
#include "lc_action_modify_rotate.h"
#include "lc_action_modify_rotate_twice.h"
#include "lc_action_modify_round.h"
#include "lc_action_modify_scale.h"
#include "lc_action_modify_stretch.h"
#include "lc_action_modify_trim.h"
#include "lc_action_modify_trim_amount.h"
#include "lc_action_polyline_add.h"
#include "lc_action_polyline_append.h"
#include "lc_action_polyline_arcs_to_lines.h"
#include "lc_action_polyline_change_segment_type.h"
#include "lc_action_polyline_delete_node.h"
#include "lc_action_polyline_delete_node_between.h"
#include "lc_action_polyline_equidistant.h"
#include "lc_action_polyline_from_segment.h"
#include "lc_action_polyline_trim.h"
#include "lc_action_select_all.h"
#include "lc_action_select_contour.h"
#include "lc_action_select_generic.h"
#include "lc_action_select_intersected.h"
#include "lc_action_select_invert.h"
#include "lc_action_select_layer.h"
#include "lc_action_select_mode_toggle.h"
#include "lc_action_select_points.h"
#include "lc_action_select_quick.h"
#include "lc_action_select_single.h"
#include "lc_action_spline_add_point.h"
#include "lc_action_spline_append_point.h"
#include "lc_action_spline_from_polyline.h"
#include "lc_action_spline_modify_explode.h"
#include "lc_action_spline_remove_between.h"
#include "lc_action_spline_remove_points.h"
#include "lc_actioncontext.h"
#include "lc_actiondimarc.h"
#include "lc_actiondimordinate.h"
#include "lc_actiondimordinaterebase.h"
#include "lc_actiondimstyleapply.h"
#include "lc_actiondrawdimbaseline.h"
#include "lc_actiondrawdual.h"
#include "lc_actiondrawgdtfeaturecontrolframe.h"
#include "lc_actiondrawhyperbolafp.h"
#include "lc_actiondrawparabola4points.h"
#include "lc_actiondrawparabolaFD.h"
#include "lc_actionentitylayertoggle.h"
#include "lc_actionfileexportmakercam.h"
#include "lc_actioninfo3pointsangle.h"
#include "lc_actioninfopickcoordinates.h"
#include "lc_actioninfopoint.h"
#include "lc_actioninfoproperties.h"
#include "lc_actioninteractivepickangle.h"
#include "lc_actioninteractivepickdistance.h"
#include "lc_actioninteractivepickposition.h"
#include "lc_actionlayerscmd.h"
#include "lc_actionlayersexport.h"
#include "lc_actionlayerstoggleconstruction.h"
#include "lc_actionpenapply.h"
#include "lc_actionpenpick.h"
#include "lc_actionpensyncactivebylayer.h"
#include "lc_actionselectdimordinatesameorigin.h"
#include "lc_actionsnapmiddlemanual.h"
#include "lc_actionucsbydimordinate.h"
#include "lc_actionucscreate.h"
#include "rs_actionblocksadd.h"
#include "rs_actionblocksattributes.h"
#include "rs_actionblockscreate.h"
#include "rs_actionblocksedit.h"
#include "rs_actionblocksfreezeall.h"
#include "rs_actionblocksremove.h"
#include "rs_actionblockssave.h"
#include "rs_actionblockstoggleview.h"
#include "rs_actiondimaligned.h"
#include "rs_actiondimangular.h"
#include "rs_actiondimdiametric.h"
#include "rs_actiondimleader.h"
#include "rs_actiondimlinear.h"
#include "rs_actiondimradial.h"
#include "rs_actiondrawellipse4points.h"
#include "rs_actiondrawellipsecenter3points.h"
#include "rs_actiondrawellipsefocipoint.h"
#include "rs_actiondrawellipseinscribe.h"
#include "rs_actiondrawhatch.h"
#include "rs_actiondrawlinehorvert.h"
#include "rs_actiondrawlineorthtan.h"
#include "rs_actiondrawlinetangent1.h"
#include "rs_actiondrawlinetangent2.h"
#include "rs_actioninfoangle.h"
#include "rs_actioninfoarea.h"
#include "rs_actioninfodist.h"
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
#include "rs_actionlockrelativezero.h"
#include "rs_actionoptionsdrawing.h"
#include "rs_actionsetrelativezero.h"
#include "rs_actiontoolregeneratedimensions.h"
#include "rs_actionzoomauto.h"
#include "rs_actionzoomin.h"
#include "rs_actionzoompan.h"
#include "rs_actionzoomprevious.h"
#include "rs_actionzoomredraw.h"
#include "rs_actionzoomwindow.h"
#include "rs_debug.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_graphicview.h"
#include "rs_layerlist.h"
#include "rs_selection.h"

RS_Layer* obtainLayer(LC_ActionContext* actionContext, void* data) {
    RS_Layer* layer{nullptr};
    if (data != nullptr) {
        layer = static_cast<RS_Layer*>(data);
    }
    else {
        RS_Document* document = actionContext->getDocument()->getDocument();
        layer = (document->getLayerList() != nullptr) ? document->getLayerList()->getActive() : nullptr;
    }
    return layer;
}

namespace InnerFactory{
    RS_ActionInterface* doCreateActionInstance(const RS2::ActionType actionType, LC_ActionContext* ctx, void* data) {
        const auto view = ctx->getGraphicView();
        switch (actionType) {
            case RS2::ActionEditKillAllActions: {
                if (view != nullptr) {
                    // DO we need to call some form of a 'clean' function?
                    if (view->killAllActionsWithResult()) {
                        const auto document = ctx->getDocument();
                        RS_Selection::unselectAllInDocument(document, view->getViewPort());
                    }
                }
                return nullptr;
            }
            case RS2::ActionEditUndo: {
                //to avoid operation on deleted entities, Undo action invalid all suspended
                //actions
                view->killAllActions();
                return new LC_ActionEditUndoRedo(true, ctx);
            }
            case RS2::ActionEditRedo: {
                return new LC_ActionEditUndoRedo(false, ctx);
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
                return new LC_ActionOrder(ctx, RS2::ActionOrderBottom);
            }
            case RS2::ActionOrderLower: {
                return new LC_ActionOrder(ctx, RS2::ActionOrderLower);
            }
            case RS2::ActionOrderRaise: {
                return new LC_ActionOrder(ctx, RS2::ActionOrderRaise);
            }
            case RS2::ActionOrderTop: {
                return new LC_ActionOrder(ctx, RS2::ActionOrderTop);
            }
            case RS2::ActionSelectSingle: {
                // fixme - sand - files - that's suspicious implementation, check it again for plugins
                RS_ActionInterface* currentAction = ctx->getCurrentAction();
                if (currentAction->rtti() != RS2::ActionSelectSingle) {
                    return new LC_ActionSelectSingle(ctx, currentAction);
                }
                break;
            }
            case RS2::ActionSelectContour: {
                return new LC_ActionSelectContour(ctx);
            }
            case RS2::ActionSelectAll: {
                return new LC_ActionSelectAll(ctx, true);
            }
            case RS2::ActionDeselectAll: {
                return new LC_ActionSelectAll(ctx, false);
            }
            case RS2::ActionSelectWindow: {
                return new LC_ActionSelectWindow(view->getTypeToSelect(), ctx);
            }
            case RS2::ActionSelectPoints: {
                return new LC_ActionSelectPoints(ctx);
            }
            case RS2::ActionDeselectWindow: {
                return new LC_ActionSelectWindow(ctx, false);
            }
            case RS2::ActionSelectInvert: {
                return new LC_ActionSelectInvert(ctx);
            }
            case RS2::ActionSelectIntersected: {
                return new LC_ActionSelectIntersected(ctx, true);
            }
            case RS2::ActionDeselectIntersected: {
                return new LC_ActionSelectIntersected(ctx, false);
            }
            case RS2::ActionSelectLayer: {
                return new LC_ActionSelectLayer(ctx);
            }
            case RS2::ActionSelectQuick: {
                return new LC_ActionSelectQuick(ctx);
            }
            case RS2::ActionSelectGeneric: {
                return new LC_ActionSelectGeneric(ctx);
            }
            case RS2::ActionSelectModeToggle: {
                return new LC_ActionSelectModeToggle(ctx);
            }
            case RS2::ActionDimRegenerate: {
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
            case RS2::ActionZoomPan:{
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
                return new LC_ActionDrawLine(ctx);
            }
            case RS2::ActionDrawLineAngle: {
                return new LC_ActionDrawLineAngle(ctx, false);
            }
            case RS2::ActionDrawLineHorizontal: {
                return new LC_ActionDrawLineAngle(ctx, true,RS2::ActionDrawLineHorizontal);
            }
            case RS2::ActionDrawLineHorVert: {
                return new RS_ActionDrawLineHorVert(ctx);
            }
            case RS2::ActionDrawLineVertical: {
                return new LC_ActionDrawLineAngle(ctx, true,RS2::ActionDrawLineVertical);
            }
            case RS2::ActionDrawLineFreehand: {
                return new LC_ActionDrawLineFreehand(ctx);
            }
            case RS2::ActionDrawLineParallel: {
                return new LC_ActionDrawLineParallel(ctx, RS2::ActionDrawLineParallel);
            }
            case RS2::ActionDrawCircleParallel: {
                return new LC_ActionDrawLineParallel(ctx, RS2::ActionDrawCircleParallel);
            }
            case RS2::ActionDrawArcParallel: {
                return new LC_ActionDrawLineParallel(ctx, RS2::ActionDrawArcParallel);
            }
            case RS2::ActionDrawLineParallelThrough: {
                return new LC_ActionDrawLineParallelThrough(ctx);
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
            case RS2::ActionDrawCenterMark: {
                return new LC_ActionDrawCenterMark(ctx);
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
            case RS2::ActionDrawPointsLine: {
                return new LC_ActionDrawPointsLine(ctx, false);
            }
            case RS2::ActionDrawPointsMiddle: {
                return new LC_ActionDrawPointsLine(ctx, true);
            }
            case RS2::ActionDrawPointsLattice: {
                return new LC_ActionDrawPointsLattice(ctx);
            }
            case RS2::ActionDrawLineBisector: {
                return new LC_ActionDrawLineBisector(ctx);
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
                return new LC_ActionDrawLineRelAngle(ctx, M_PI_2, true);
            }
            case RS2::ActionDrawLineRelAngle: {
                return new LC_ActionDrawLineRelAngle(ctx, M_PI_2, false);
            }
            case RS2::ActionDrawPolyline: {
                return new LC_ActionDrawPolyline(ctx);
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
            case RS2::ActionDrawCenterLine: {
                return new LC_ActionDrawCenterLine(ctx);
            }
            case RS2::ActionDrawLineRadiant: {
                return new LC_ActionDrawLineRadiant(ctx);
            }
            case RS2::ActionDrawStar: {
                return new LC_ActionDrawStar(ctx);
            }
            case RS2::ActionPolylineAdd: {
                return new LC_ActionPolylineAdd(ctx);
            }
            case RS2::ActionPolylineAppend: {
                return new LC_ActionPolylineAppend(ctx);
            }
            case RS2::ActionPolylineDel: {
                return new LC_ActionPolylineDeleteNode(ctx);
            }
            case RS2::ActionPolylineDelBetween: {
                return new LC_ActionPolylineDeleteNodeBetween(ctx);
            }
            case RS2::ActionPolylineTrim: {
                return new LC_ActionPolylineTrim(ctx);
            }
            case RS2::ActionPolylineEquidistant: {
                return new LC_ActionPolylineEquidistant(ctx);
            }
            case RS2::ActionPolylineSegment: {
                if (data == nullptr) {
                    return new LC_ActionPolylineFromSegment(ctx);
                }
                return new LC_ActionPolylineFromSegment(ctx, static_cast<RS_Entity*>(data));
            }
            case RS2::ActionPolylineArcsToLines: {
                return new LC_ActionPolylineArcsToLines(ctx);
            }
            case RS2::ActionPolylineChangeSegmentType: {
                return new LC_ActionPolylineChangeSegmentType(ctx);
            }
            case RS2::ActionDrawLinePolygonCenCor: {
                return new LC_ActionDrawLinePolygonCenterCorner(ctx);
            }
            case RS2::ActionDrawLinePolygonCenTan: {
                return new LC_ActionDrawLinePolygonCenterTangent(ctx);
            }
            case RS2::ActionDrawLinePolygonSideSide: {
                return new LC_ActionDrawLinePolygonSideSide(ctx);
            }
            case RS2::ActionDrawLinePolygonCorCor: {
                return new LC_ActionDrawLinePolygonCornerCorner(ctx);
            }
            case RS2::ActionDrawCircleCenterPoint: {
                return new LC_ActionDrawCircleCenterPoint(ctx);
            }
            case RS2::ActionDrawCircleCenterRadius: {
                return new LC_ActionDrawCircleCenterRadius(ctx);
            }
            case RS2::ActionDrawCircleByArc: {
                return new LC_ActionDrawCircleByArc(ctx);
            }
            case RS2::ActionDrawCircle2Points: {
                return new LC_ActionDrawCircle2Points(ctx);
            }
            case RS2::ActionDrawCircle2PointsRadius: {
                return new LC_ActionDrawCircle2PointsRadius(ctx);
            }
            case RS2::ActionDrawCircle3Points: {
                return new LC_ActionDrawCircle3Points(ctx);
            }
            case RS2::ActionDrawCircleTangental1Entity2Points: {
                return new LC_ActionDrawCircleTangental1Entity2Points(ctx);
            }
            case RS2::ActionDrawCircleTangental2Entities1Point: {
                return new LC_ActionDrawCircleTangental2Entities1Point(ctx);
            }
            case RS2::ActionDrawCircleInscribe: {
                return new LC_ActionDrawCircleInscribe(ctx);
            }
            case RS2::ActionDrawCircleTan2EntitiesRadius: {
                return new LC_ActionDrawCircleTangental2EntitiesRadius(ctx);
            }
            case RS2::ActionDrawCircleTan3Entities: {
                return new LC_ActionDrawCircleTangental3Entities(ctx);
            }
            case RS2::ActionDrawArc: {
                return new LC_ActionDrawArcCenterPointParam(ctx, RS2::ActionDrawArc);
            }
            case RS2::ActionDrawArcChord: {
                return new LC_ActionDrawArcCenterPointParam(ctx, RS2::ActionDrawArcChord);
            }
            case RS2::ActionDrawArcAngleLen: {
                return new LC_ActionDrawArcCenterPointParam(ctx, RS2::ActionDrawArcAngleLen);
            }
            case RS2::ActionDrawArc3P: {
                return new LC_ActionDrawArc3Points(ctx);
            }
            case RS2::ActionDrawArcTangential: {
                return new LC_ActionDrawArcTangential(ctx);
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
            case RS2::ActionDrawParabolaFocusDiretrix: {
                return new LC_ActionDrawParabolaFD(ctx);
            }
            case RS2::ActionDrawHyperbolaFoci2Points: {
              return new LC_ActionDrawHyperbolaFP(ctx);
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
                return new LC_ActionDrawSpline(ctx);
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
                return new LC_ActionDrawMText(ctx);
            }
            case RS2::ActionDrawText: {
                return new LC_ActionDrawText(ctx);
            }
            case RS2::ActionDrawHatch: {
                return new RS_ActionDrawHatch(ctx);
            }
            case RS2::ActionDrawImage: {
                return new LC_ActionDrawImage(ctx);
            }
            case RS2::ActionDrawDual: {
                return new LC_ActionDrawDual(ctx);
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
            case RS2::ActionGTDFCFrame: {
                return new LC_ActionDrawGDTFeatureControlFrame(ctx);
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
            case RS2::ActionDimStyleApply: {
                return new LC_ActionDimStyleApply(ctx);
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
                return new LC_ActionModifyAttributes(ctx);
            }
            case RS2::ActionModifyDelete:
                [[fallthrough]];
            case RS2::ActionModifyDeleteQuick: {
                return new LC_ActionModifyDelete(ctx);
            }
            case RS2::ActionModifyDeleteFree: {
                return new LC_ActionModifyDeleteFree(ctx);
            }
            case RS2::ActionModifyMove: {
                return new LC_ActionModifyMove(ctx);
            }
            case RS2::ActionModifyRevertDirection: {
                return new LC_ActionModifyRevertDirection(ctx);
            }
            case RS2::ActionModifyRotate: {
                return new LC_ActionModifyRotate(ctx);
            }
            case RS2::ActionModifyScale: {
                return new LC_ActionModifyScale(ctx);
            }
            case RS2::ActionModifyMirror: {
                return new LC_ActionModifyMirror(ctx);
            }
            case RS2::ActionModifyMoveRotate: {
                return new LC_ActionModifyMoveRotate(ctx);
            }
            case RS2::ActionModifyRotateTwice: {
                return new LC_ActionModifyRotateTwice(ctx);
            }
            case RS2::ActionModifyEntity: {
                return new LC_ActionModifyEntity(ctx);
            }
            case RS2::ActionModifyTrim: {
                return new LC_ActionModifyTrim(ctx, false);
            }
            case RS2::ActionModifyTrim2: {
                return new LC_ActionModifyTrim(ctx, true);
            }
            case RS2::ActionModifyTrimAmount: {
                return new LC_ActionModifyTrimAmount(ctx);
            }
            case RS2::ActionModifyCut: {
                return new LC_ActionModifyCut(ctx);
            }
            case RS2::ActionModifyStretch: {
                return new LC_ActionModifyStretch(ctx);
            }
            case RS2::ActionModifyBevel: {
                return new LC_ActionModifyBevel(ctx);
            }
            case RS2::ActionModifyRound: {
                return new LC_ActionModifyRound(ctx);
            }
            case RS2::ActionModifyOffset: {
                return new LC_ActionModifyOffset(ctx);
            }
            case RS2::ActionModifyExplodeText: {
                return new LC_ActionModifyExplodeText(ctx);
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
                return new LC_ActionInfoDistPointToEntity(ctx);
            }
            case RS2::ActionInfoDistPoint2Entity: {
                return new LC_ActionInfoDistPointToEntity(ctx, true);
            }
            case RS2::ActionInfoPoint: {
                return new LC_ActionInfoPoint(ctx);
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
                const auto a_layer = obtainLayer(ctx, data);
                if (a_layer != nullptr) {
                    return new RS_ActionLayersToggleView(ctx, a_layer);
                }
                break;
            }
            case RS2::ActionLayersToggleLock: {
                const auto a_layer = obtainLayer(ctx, data);
                if (a_layer != nullptr) {
                    return new RS_ActionLayersToggleLock(ctx, a_layer);
                }
                break;
            }
            case RS2::ActionLayersTogglePrint: {
                const auto a_layer = obtainLayer(ctx, data);
                if (a_layer != nullptr) {
                    return new RS_ActionLayersTogglePrint(ctx, a_layer);
                }
                break;
            }
            case RS2::ActionLayersToggleConstruction: {
                const auto a_layer = obtainLayer(ctx, data);
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
                return new LC_ActionBlockInsert(ctx);
            }
            case RS2::ActionBlocksToggleView: {
                return new RS_ActionBlocksToggleView(ctx);
            }
            case RS2::ActionBlocksCreate: {
                return new RS_ActionBlocksCreate(ctx);
            }
            case RS2::ActionBlocksExplode: {
                return new LC_ActionBlocksExplode(ctx);
            }
            case RS2::ActionLibraryInsert: {
                return new LC_ActionBlockLibraryInsert(ctx);
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
                const auto currentAction = ctx->getCurrentAction();
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
            case RS2::ActionLayerEntityHideOthers:
                [[fallthrough]];
            case RS2::ActionLayerEntityToggleLock: {
                return new LC_ActionLayerToggle(ctx, actionType);
            }
            case RS2::ActionDimOrdByOriginSelect: {
                return new LC_ActionSelectDimOrdinateSameOrigin(ctx);
            }
            case RS2::ActionDimOrdRebase: {
                return new LC_ActionDimOrdinateRebase(ctx);
            }
            case RS2::ActionInteractivePickPoint: {
                return new LC_ActionInteractivePickPosition(ctx, RS2::ActionInteractivePickPoint);
            }
            case RS2::ActionInteractivePickPoint_X: {
                return new LC_ActionInteractivePickPosition(ctx,RS2::ActionInteractivePickPoint_X);
            }
            case RS2::ActionInteractivePickPoint_Y: {
                return new LC_ActionInteractivePickPosition(ctx,RS2::ActionInteractivePickPoint_Y);
            }
            case RS2::ActionInteractivePickLength: {
                return new LC_ActionInteractivePickDistance(ctx);
            }
            case RS2::ActionInteractivePickAngle: {
                return new LC_ActionInteractivePickAngle(ctx);
            }
            case RS2::ActionModifyMoveAdjust: {
                if (data != nullptr) {
                    const auto* movementInfo = static_cast<LC_ActionModifyMoveAdjust::MovementInfo*>(data);
                    return new LC_ActionModifyMoveAdjust(ctx, *movementInfo);
                }
                break;
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
std::shared_ptr<RS_ActionInterface> LC_ActionsHandlerFactory::createActionInstance(const RS2::ActionType actionType, LC_ActionContext* ctx, void* data) {
    RS_ActionInterface* actionInstance = InnerFactory::doCreateActionInstance(actionType, ctx, data);
    if (actionInstance != nullptr) {
        actionInstance->postCreateInit();
    }
    std::shared_ptr<RS_ActionInterface> result{actionInstance};
    return result;
}
