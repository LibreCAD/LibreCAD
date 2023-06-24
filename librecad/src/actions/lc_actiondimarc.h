/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2021 Melwyn Francis Carlo <carlo.melwyn@outlook.com>
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

#ifndef LC_ACTIONDIMARC_H
#define LC_ACTIONDIMARC_H

#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "lc_dimarc.h"
#include "rs_actiondimension.h"


class LC_ActionDimArc : public RS_ActionDimension
{
    Q_OBJECT

    private:

        enum Status
        {
            SetEntity, 
            SetPos 
        };


    public:

    LC_ActionDimArc(RS_EntityContainer& container, RS_GraphicView& graphicView);
   ~LC_ActionDimArc() override;

    void reset()   override;
    void trigger() override;

    void mouseMoveEvent(QMouseEvent* e)    override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    void showOptions() override;
    void hideOptions() override;

    void coordinateEvent(RS_CoordinateEvent* e) override;
    void commandEvent(RS_CommandEvent* e)       override;

    QStringList getAvailableCommands() override;

    void updateMouseButtonHints() override;


    private:

        RS_Entity* selectedArcEntity;

        LC_DimArcData dimArcData;

        void setRadius(const RS_Vector& selectedPosition);
};
#endif //LC_ACTIONDIMARC_H
