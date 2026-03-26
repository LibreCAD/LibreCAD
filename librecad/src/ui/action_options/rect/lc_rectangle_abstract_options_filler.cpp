/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_rectangle_abstract_options_filler.h"

#include "lc_action_draw_rectangle_1point.h"

void LC_RectangleAbstractOptionsFiller::createEdgesModeOption(LC_PropertyContainer* container, LC_ActionDrawRectangleAbstract* action) {
    static LC_EnumDescriptor edgesModeDescriptor = {
        "cornerTypeDescriptor",
        {
            {LC_ActionDrawRectangleAbstract::EdgesMode::EDGES_BOTH, tr("Both")},
            {LC_ActionDrawRectangleAbstract::EdgesMode::EDGES_VERT, tr("Vertical")},
            {LC_ActionDrawRectangleAbstract::EdgesMode::EDGES_HOR, tr("Horizontal")}
        }
    };

    addEnum({
                "a_cornersMode",
                tr("Edges"),
                tr(
                    "Defines which edges of rectangle should be drawn (so it is possible to create just two parallel lines instead of rectangle)")
            }, &edgesModeDescriptor, [action]() -> LC_PropertyEnumValueType {
                return action->getEdgesDrawMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setEdgesDrawMode(v);
            }, container);
}

void LC_RectangleAbstractOptionsFiller::fillCornersMode(LC_PropertyContainer* container, LC_ActionDrawRectangleAbstract* action) {

    static LC_EnumDescriptor cornderModeDescriptor = {
        "cornerTypeDescriptor",
        {
            {LC_ActionDrawRectangleAbstract::CornersMode::CORNER_STRAIGHT, tr("Straight")},
            {LC_ActionDrawRectangleAbstract::CornersMode::CORNER_RADIUS, tr("Round")},
            {LC_ActionDrawRectangleAbstract::CornersMode::CORNER_BEVEL, tr("Bevel")}
        }
    };

    addEnum({"a_cornersMode", tr("Corners"), tr("Controls how corners of rectangle should be drawn")}, &cornderModeDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getCornersMode();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setCornersMode(v);
            }, container);

    const int cornersMode = action->getCornersMode();

    switch (cornersMode) {
        case LC_ActionDrawRectangleAbstract::CornersMode::CORNER_STRAIGHT: {
            createEdgesModeOption(container, action);
            break;
        }
        case LC_ActionDrawRectangleAbstract::CornersMode::CORNER_RADIUS: {
            addLinearDistance({"a_radius", tr("Radius"), tr("Radius of rounded corners")}, [action]() {
                                  return action->getCornerRadius();
                              }, [action](double val) {
                                  action->setCornerRadius(val);
                              }, container);

            addBoolean({
                           "a_snapShift",
                           tr("Snap Shift"),
                           tr("If checked, specifies that snap point should be shifted by radius of corners.")
                       }, [action]()-> bool {
                           return action->isSnapToCornerArcCenter();
                       }, [action](bool val)-> void {
                           action->setSnapToCornerArcCenter(val);
                       }, container);

            if (action->rtti() == RS2::ActionDrawRectangle1Point) {
                auto action1p = static_cast<LC_ActionDrawRectangle1Point*>(action);
                addBoolean({
                               "a_sizeInner",
                               tr("Size inner"),
                               tr(
                                   "If checked, specified height and width of rectangle defines distance between centers of arcs for rounding corners. Otherwise, these values defines outer size of the rectangle")
                           }, [action1p]()-> bool {
                               return action1p->isSizeInner();
                           }, [action1p](bool val)-> void {
                               action1p->setSizeInner(val);
                           }, container);
            }
            break;
        }
        case LC_ActionDrawRectangle1Point::CornersMode::CORNER_BEVEL: {
            addLinearDistance({"a_bevelLenX", tr("Length X"), tr("Length of bevel corner for X direction")}, [action]() {
                                  return action->getCornerBevelLengthX();
                              }, [action](double val) {
                                  action->setCornerBevelLengthX(val);
                              }, container);

            addLinearDistance({"a_bevelLenX", tr("Length Y"), tr("Length of bevel corner for Y direction")}, [action]() {
                                  return action->getCornerBevelLengthY();
                              }, [action](double val) {
                                  action->setCornerBevelLengthY(val);
                              }, container);
            break;
        }
        default:
            break;
    }
}
