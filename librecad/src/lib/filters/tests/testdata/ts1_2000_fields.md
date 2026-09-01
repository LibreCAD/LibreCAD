<!--
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
 -->

# `ts1_2000_fields.dwg` provenance

This is an unmodified copy of LibreDWG's
`test/test-data/2000/TS1.dwg`, retrieved from the public
[`LibreDWG/libredwg`](https://github.com/LibreDWG/libredwg) repository.  It is
distributed under GPL-3.0-or-later by LibreDWG.  LibreCAD uses it solely as a
physical AC1015/R2000 reader witness for typed `FIELD` and `FIELDLIST` objects.

SHA-256: `22852ead1a0e316c18f588a9073507912a9cda05c2b942424f118589e8d7ecb7`

The fixture contains four `FIELD` records (`14E`, `14F`, `15E`, and `15F`) and
one `FIELDLIST` (`150`) pointing at them in that order. Its FIELD values use
the supported scalar variants, so it is not evidence for raw-only fallback or
opaque cross-version replay.
