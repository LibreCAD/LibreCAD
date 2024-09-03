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
#ifndef RS_ACTIONPOLYLINETRIM_H
#define RS_ACTIONPOLYLINETRIM_H

#include "rs_previewactioninterface.h"

class RS_AtomicEntity;
class RS_Entity;

/**
 * This action class can handle user events to move entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionPolylineTrim:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionPolylineTrim(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView);
    void init(int status) override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void finish(bool updateTB) override;
protected:
    /**
     * Action States.
     */
    enum Status {
        ChooseEntity,   /**< Choosing existing polyline to trim. */
        SetSegment1,    /**< Setting first segment. */
        SetSegment2       /**< Setting second segment. */
    };
    RS_Polyline *polylineToModify = nullptr;
    RS_AtomicEntity *Segment1 = nullptr;
    RS_AtomicEntity *Segment2 = nullptr;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void updateMouseButtonHints() override;
};
#endif
