/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef DXFPARSERLIMITS_H
#define DXFPARSERLIMITS_H

#include <cstddef>

namespace DRW {

//! Maximum number of DXF 102 application-group pairs staged per record.
constexpr std::size_t kMaxDxfApplicationGroupPairs = 65536;
//! Maximum nesting depth accepted inside one DXF 102 application group.
constexpr int kMaxDxfApplicationGroupNesting = 64;
//! Maximum aggregate size of one DXF binary payload assembled from chunks.
constexpr std::size_t kMaxDxfBinaryPayloadBytes = 64u * 1024u * 1024u;
//! Maximum number of CLASS records staged before the section is committed.
constexpr std::size_t kMaxDxfClasses = 65536u;
//! Maximum number of 102 application groups retained on one entity.
constexpr std::size_t kMaxDxfApplicationGroups = 65536u;
//! Maximum number of INSERT handles retained by one BLOCK_RECORD BLKREFS group.
constexpr std::size_t kMaxDxfBlockRecordInsertHandles = 1000000u;

} // namespace DRW

#endif // DXFPARSERLIMITS_H
