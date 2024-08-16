/****************************************************************************
*
* Actions that collects coordinates by mouse click on drawing using current
* snap mode
*
Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef LC_ACTIONINFOPICKCOORDINATES_H
#define LC_ACTIONINFOPICKCOORDINATES_H

#include "lc_abstractactionwithpreview.h"

class LC_ActionInfoPickCoordinates:public LC_AbstractActionWithPreview
{
public:
    LC_ActionInfoPickCoordinates(RS_EntityContainer &container, RS_GraphicView &graphicView);
    void init(int status) override;
    void resume() override;
protected:
    void doPreparePreviewEntities(QMouseEvent *e, RS_Vector &snap, QList<RS_Entity *> &list, int status) override;
    void doOnLeftMouseButtonRelease(QMouseEvent *e, int status, const RS_Vector &snapPoint) override;
    RS_Vector doGetMouseSnapPoint(QMouseEvent *e) override;
    void doFinish(bool updateTB) override;
    void updateMouseButtonHints() override;
private:
    /**
     * collected points
     */
    QVector<RS_Vector> points;
    /*
     * flag from options that indicates whether a line between points should be drawsn
     */
    bool drawPointsPath = true;

    void updateQuickInfoWidget(const RS_Vector &coord);
    void updateCollectedPointsByWidget();
};

#endif // LC_ACTIONINFOPICKCOORDINATES_H
