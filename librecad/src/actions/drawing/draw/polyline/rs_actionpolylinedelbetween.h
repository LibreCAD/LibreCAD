/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
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
#ifndef RS_ACTIONPOLYLINEDELBETWEEN_H
#define RS_ACTIONPOLYLINEDELBETWEEN_H


#include "lc_actionpolylinedeletebase.h"

class RS_AtomicEntity;

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionPolylineDelBetween:public LC_ActionPolylineDeleteBase {
Q_OBJECT

public:
    RS_ActionPolylineDelBetween(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    ~RS_ActionPolylineDelBetween() override;
    void init(int status) override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;
protected:
    RS_Vector vertexToDelete2 = RS_Vector(false);
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void collectEntitiesToRemove(RS_Vector vector, RS_Vector vector1, QList<RS_Entity *> &list);
    void updateMouseButtonHints() override;
};

#endif
