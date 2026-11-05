/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
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

#ifndef LC_HATCHDENSITYTEXT_H
#define LC_HATCHDENSITYTEXT_H

#include <QString>

struct LC_HatchDensityReport;

/**
 * User-visible strings for the hatch density guard. Engine code (lib/engine)
 * stays free of QStrings and QObject::tr; this layer builds the texts that
 * Draw Hatch, the hatch dialog, the import summary, Quick Info and the
 * property sheet all show.
 *
 * Every string uses QCoreApplication::translate with the context
 * "LC_HatchDensity" so translators see them under one group.
 */
namespace LC_HatchDensityText {

/**
 * The "Hatch pattern too dense: about N pieces (limit M); shown as a fill.
 * Use scale >= S." line. S comes from LC_HatchDensity::minimumScale(report,
 * currentScale, limit) and is rounded to 4 significant digits.
 */
QString tooDenseMessage(const LC_HatchDensityReport& report,
                        double currentScale);

/**
 * Status string for the Quick Info and property sheet "Pattern lines" row.
 * Returns "not generated (~ N > limit M)" when the report tripped the guard,
 * "generated" otherwise.
 */
QString patternLinesStatus(const LC_HatchDensityReport& report);

/**
 * One-shot summary printed to the command line on import when any hatch of
 * the opened drawing tripped the density guard. n is the count of such
 * hatches, found by walking model-space, every block and every top-level
 * insert's hatch children.
 */
QString importSummary(int n);

/**
 * Warning printed when the DXF R12 writer sees any pattern hatch with the
 * density tripped. In R12 those hatches are written as empty blocks, so the
 * user needs to know their fill will not appear in the file.
 */
QString r12Warning(int n);

} // namespace LC_HatchDensityText

#endif // LC_HATCHDENSITYTEXT_H
