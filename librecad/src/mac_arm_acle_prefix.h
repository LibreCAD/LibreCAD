/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
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
**********************************************************************/

// Prefix header force-included on macOS builds (see librecad/src/src.pro).
//
// Qt6's qyieldcpu.h calls __yield() without including <arm_acle.h>, which
// breaks compilation on Apple Silicon. Pulling the header in here fixes that,
// while the __aarch64__ guard makes this a no-op for the x86_64 slice so the
// same force-include works for arm64-only, x86_64-only and universal
// (arm64 + x86_64) builds alike.

#if defined(__aarch64__)
#include <arm_acle.h>
#endif
