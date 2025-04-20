/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo
** Copyright (C) 2024 Dongxu Li <dongxuli2011@gmail.com>
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

#include <memory>
#include "rs_previewactioninterface.h"

class RS_Pen;

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
class LC_ActionSnapMiddleManual : public RS_PreviewActionInterface{
Q_OBJECT
public:
    LC_ActionSnapMiddleManual(LC_ActionContext *actionContext);
    ~LC_ActionSnapMiddleManual() override;
    void init(int status)   override;
protected:
    /* Action states */
    enum Status
    {
        SetPercentage,   /* Setting the percentage */
        SetStartPoint,   /* Setting the startpoint */
        SetEndPoint      /* Setting the endpoint   */
    };
    struct SnapMiddleManualData;
    std::unique_ptr<SnapMiddleManualData> m_actionData;

    bool doProcessCommand(int status, const QString& command) override;
    void onMouseLeftButtonRelease(int status, LC_MouseEvent* e) override;
    void fireUnsetMiddleManual();
    void onMouseRightButtonRelease(int status, LC_MouseEvent *e) override;
    void onMouseMoveEvent(int status, LC_MouseEvent *event) override;
    void onCoordinateEvent(int status, bool isZero, const RS_Vector &pos) override;
    void updateMouseButtonHints() override;
    QStringList doGetAvailableCommands(int status) override;
};
