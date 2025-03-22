/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2017 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2017 taoxumuye (tfy.hi@163.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
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
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef LC_ACTIONDRAWLINEPOLYGON3_H
#define LC_ACTIONDRAWLINEPOLYGON3_H

#include "lc_actiondrawlinepolygonbase.h"

/**
 * This action class can handle user events to draw polygons.
 *
 */
class LC_ActionDrawLinePolygonCenTan : public LC_ActionDrawLinePolygonBase {
    Q_OBJECT
public:
    LC_ActionDrawLinePolygonCenTan(LC_ActionContext *actionContext);
    ~LC_ActionDrawLinePolygonCenTan() override;
protected:
    /* Status
    SetPoint1 - Setting center.
    SetPoint2 - Setting tangent.
*/
    QString getPoint2Hint() const override;
    void preparePolygonInfo(PolygonInfo &polygonInfo, const RS_Vector &snap) override;
};
#endif // LC_ACTIONDRAWLINEPOLYGON3_H
