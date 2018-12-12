/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
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

#ifndef RS_ACTIONDIMANGULAR_H
#define RS_ACTIONDIMANGULAR_H

#include "rs_actiondimension.h"
#include "rs_line.h"

struct RS_DimAngularData;

/**
 * This action class can handle user events to draw angular dimensions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimAngular : public RS_ActionDimension
{
    Q_OBJECT

private:
    enum Status {
        SetLine1,      ///< Choose 1st line
        SetLine2,      ///< Choose 2nd line
        SetPos,        ///< Choose position
        SetText        ///< Setting text label in consle
    };

public:
    RS_ActionDimAngular(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
    ~RS_ActionDimAngular() override;

    void reset() override;

    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e) override;
    QStringList getAvailableCommands() override;

    void hideOptions() override;
    void showOptions() override;

    void updateMouseButtonHints() override;

private:
    RS_Line     line1;                          ///< 1st chosen line
    RS_Line     line2;                          ///< 2nd chosen line
    RS_Vector   click1;                         ///< 1st click pos
    RS_Vector   click2;                         ///< 2nd click pos
    RS_Vector   center;                         ///< Center of arc
    std::unique_ptr<RS_DimAngularData> edata;   ///< Data of new dimension
    Status      lastStatus;                     ///< Last status before entering text
    std::vector<double> angles;                 ///< Array to sort line angles
    int         quadrantOffset {0};             ///< Offset on starting quadrant

    void justify( RS_Line &line, const RS_Vector &click);
    void lineOrder(const RS_Vector &dimPos);
    int quadrant(const double angle);
    bool setData(const RS_Vector& dimPos, const bool calcCenter = false);
};

#endif
