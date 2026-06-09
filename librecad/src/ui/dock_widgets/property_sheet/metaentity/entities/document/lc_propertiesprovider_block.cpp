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

#include "lc_propertiesprovider_block.h"

#include "lc_propertyprovider_utils.h"
#include "rs_block.h"

const QString LC_PropertiesProviderBlock::SECTION_BLOCK = "_secBlock";

using namespace LC_PropertyProviderUtils;

void LC_PropertiesProviderBlock::fillDocumentProperties(LC_PropertyContainer* container, RS_Graphic* graphic, RS_Block* block) {
    const LC_Property::Names names = {SECTION_BLOCK, tr("Block"), tr("Block settings")};
    const auto cont = createSection(container, names);

    createName(cont, block, graphic);
    createBasePoint(cont, block, graphic);

    // fixme - this is just a basic placeholder implementation. Think which properties from rs_graphic should be also shown for block!
}

void LC_PropertiesProviderBlock::createName(LC_PropertyContainer* cont, RS_Block* block,[[maybe_unused]] RS_Graphic* graphic) {
    const LC_Property::Names names = {"blkName", tr("Name"), tr("Name of the block")};
    const QString blockName = block->getName();
    createDirectDelegatedReadonlyString(cont, names, blockName);
}

void LC_PropertiesProviderBlock::createBasePoint(LC_PropertyContainer* cont, RS_Block* block, [[maybe_unused]] RS_Graphic* graphic) const {
    const auto formatter = m_actionContext->getFormatter();
    const QString strVector = formatter->formatUCSVector(block->getBasePoint());
    const QString originString = QString("[%1]").arg(strVector);

    const LC_Property::Names namesOrigin = {"blkBasePoint", tr("Base point"), tr("Base point for the block (0,0). This point will be in block insertion point.")};
    createDirectDelegatedReadonlyString(cont, namesOrigin, originString);
}
