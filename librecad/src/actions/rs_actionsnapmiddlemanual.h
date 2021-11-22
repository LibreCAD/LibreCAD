/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo
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


#pragma once


#include "rs_pen.h"
#include "rs_vector.h"
#include "rs_previewactioninterface.h"


/*
    This action class can snap (set) the relative-zero marker
    in the middle between two points chosen by the user.

    The middle point, by default, rests at a poistion 50% relative 
    to the position of the first point chosen, that is, 
    at the center of the imaginary line connecting the 
    two user-defined points.

    This percentage, however, can be modified by the user 
    in order to place the marker at a different position 
    along the imaginary line.
*/


class RS_ActionSnapMiddleManual : public RS_PreviewActionInterface
{
    Q_OBJECT

    public:

        /* Action states */
        enum Status
        {
            SetPercentage,   /* Setting the percentage */
            SetStartPoint,   /* Setting the startpoint */
            SetEndPoint      /* Setting the endpoint   */
        };

        RS_ActionSnapMiddleManual( RS_EntityContainer& container, 
                                   RS_GraphicView& graphicView);

       ~RS_ActionSnapMiddleManual() override;

        void init(int status = 0)   override;

        void mouseMoveEvent    (QMouseEvent* e) override;
        void mouseReleaseEvent (QMouseEvent* e) override;

        void coordinateEvent (RS_CoordinateEvent* e) override;
        void commandEvent    (RS_CommandEvent*    e) override;

        QStringList getAvailableCommands() override;

        void updateMouseButtonHints() override;

    private:

        double percentage;

        RS_Vector startPoint;
        RS_Vector endPoint;
};

