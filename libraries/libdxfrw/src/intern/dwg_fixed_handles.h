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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef DWG_FIXED_HANDLES_H
#define DWG_FIXED_HANDLES_H

#include <cstdint>

namespace DRW {

// Canonical R2000+ DWG handles emitted by the writer. Keep these in a narrow
// header so changing the table does not fan out through drw_base.h.
inline constexpr std::uint32_t DwgBlockControlHandle = 0x01;
inline constexpr std::uint32_t DwgLayerControlHandle = 0x02;
inline constexpr std::uint32_t DwgStyleControlHandle = 0x03;
inline constexpr std::uint32_t DwgLtypeControlHandle = 0x05;
inline constexpr std::uint32_t DwgViewControlHandle = 0x06;
inline constexpr std::uint32_t DwgUcsControlHandle = 0x07;
inline constexpr std::uint32_t DwgVportControlHandle = 0x08;
inline constexpr std::uint32_t DwgAppIdControlHandle = 0x09;
inline constexpr std::uint32_t DwgDimstyleControlHandle = 0x0A;
inline constexpr std::uint32_t DwgViewportEntityHeaderControlHandle = 0x0B;
inline constexpr std::uint32_t DwgLtypeByBlockHandle = 0x0F;
inline constexpr std::uint32_t DwgLtypeByLayerHandle = 0x10;
inline constexpr std::uint32_t DwgLtypeContinuousHandle = 0x11;
inline constexpr std::uint32_t DwgLayer0Handle = 0x12;
inline constexpr std::uint32_t DwgStandardTextStyleHandle = 0x13;
inline constexpr std::uint32_t DwgAcadAppIdHandle = 0x14;
inline constexpr std::uint32_t DwgStandardDimstyleHandle = 0x15;
inline constexpr std::uint32_t DwgActiveVportHandle = 0x16;
inline constexpr std::uint32_t DwgModelSpaceBlockEntityHandle = 0x1B;
inline constexpr std::uint32_t DwgModelSpaceEndBlockEntityHandle = 0x1C;
inline constexpr std::uint32_t DwgPaperSpaceBlockEntityHandle = 0x1D;
inline constexpr std::uint32_t DwgPaperSpaceEndBlockEntityHandle = 0x1E;

} // namespace DRW

#endif // DWG_FIXED_HANDLES_H
