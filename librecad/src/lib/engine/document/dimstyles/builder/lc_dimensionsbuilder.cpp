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

#include "lc_dimensionsbuilder.h"

#include "lc_dimarc.h"
#include "lc_dimordinate.h"
#include "rs_debug.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimension.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_graphic.h"
#include "rs_leader.h"
#include "rs_line.h"

LC_DimensionsBuilder::LC_DimensionsBuilder(RS_Graphic* graphic, RS_Dimension* dim) : m_graphic{graphic}, m_dimension{dim},
                                                                                     m_dimStyle(nullptr) {
}

void LC_DimensionsBuilder::build() {
    const RS2::EntityType rtti = m_dimension->rtti();
    switch (rtti) {
        case RS2::EntityDimAligned: {
            auto* dim = static_cast<RS_DimAligned*>(m_dimension);
            buildAligned(dim);
            break;
        }
        case RS2::EntityDimLinear: {
            const auto dim = static_cast<RS_DimLinear*>(m_dimension);
            buildLinear(dim);
            break;
        }
        case RS2::EntityDimRadial: {
            const auto dim = static_cast<RS_DimRadial*>(m_dimension);
            buildRadial(dim);
            break;
        }
        case RS2::EntityDimDiametric: {
            const auto dim = static_cast<RS_DimDiametric*>(m_dimension);
            buildDiametric(dim);
            break;
        }
        case RS2::EntityDimAngular: {
            const auto dim = static_cast<RS_DimAngular*>(m_dimension);
            buildAngular(dim);
            break;
        }
        case RS2::EntityDimArc: {
            const auto dim = static_cast<LC_DimArc*>(m_dimension);
            buildArc(dim);
            break;
        }
        case RS2::EntityDimOrdinate: {
            const auto dim = static_cast<LC_DimOrdinate*>(m_dimension);
            buildOrdinate(dim);
            break;
        }
            // fixme - sand - dims - restore leader!!
        /*case RS2::EntityDimLeader: {
            auto dim = static_cast<RS_Leader*>(dimension);
            buildLeader(dim);
            break;
        }*/
        default: {
            LC_ERR << "Unexpected dimension type for building dimension!. Value: " << rtti;
        }
    }
}

void LC_DimensionsBuilder::buildLinear([[maybe_unused]]RS_DimLinear* dimLinear) {
}

void LC_DimensionsBuilder::buildAligned([[maybe_unused]]RS_DimAligned* dimLinear) {
}

void LC_DimensionsBuilder::buildDiametric([[maybe_unused]]RS_DimDiametric* dimLinear) {
}

void LC_DimensionsBuilder::buildAngular([[maybe_unused]]RS_DimAngular* dimLinear) {
}

void LC_DimensionsBuilder::buildArc([[maybe_unused]]LC_DimArc* dimLinear) {
}

void LC_DimensionsBuilder::buildLeader([[maybe_unused]]RS_Leader* dimLinear) {
}

void LC_DimensionsBuilder::buildRadial([[maybe_unused]]RS_DimRadial* dimLinear) {
}

void LC_DimensionsBuilder::buildOrdinate([[maybe_unused]]LC_DimOrdinate* dimLinear) {
}


RS_Line* LC_DimensionsBuilder::addDimComponentLine(RS_Vector start, RS_Vector end, const RS_Pen &pen) const {
    const auto line = new RS_Line(m_dimension, {start, end});
    line->setPen(pen);
    line->setLayer(nullptr);
    m_dimension->addEntity(line);
    return line;
}


RS_Pen LC_DimensionsBuilder::getPenForText() const {
    const auto text = m_dimStyle->text();
    RS_Pen result(text->color(), RS2::WidthByBlock, RS2::SolidLine);
    return result;
}

RS_Pen LC_DimensionsBuilder::getPenExtensionLine() const {
    const auto extLine = m_dimStyle->extensionLine();
    RS_Pen result(extLine->color(), extLine->lineWidth(), RS2::LineByBlock);
    return result;
}

RS_Pen LC_DimensionsBuilder::getPenDimensionLine() const {
    const auto dimLine = m_dimStyle->dimensionLine();
    RS_Pen result(dimLine->color(), dimLine->lineWidth(), RS2::LineByBlock);
    return result;
}
