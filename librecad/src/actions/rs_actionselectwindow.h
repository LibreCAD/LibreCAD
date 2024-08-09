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

#ifndef RS_ACTIONSELECTWINDOW_H
#define RS_ACTIONSELECTWINDOW_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to select all entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionSelectWindow:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionSelectWindow(
        RS_EntityContainer &container,
        RS_GraphicView &graphicView,
        bool select);
    RS_ActionSelectWindow(
        enum RS2::EntityType typeToSelect, RS_EntityContainer &container,
        RS_GraphicView &graphicView,
        bool select);
    ~RS_ActionSelectWindow() override;
    void init(int status) override;
    void trigger() override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    enum RS2::EntityType getTypeToSelect();
protected:
    /**
 * Action States.
 */
    enum Status {
        SetCorner1,     /**< Setting the 1st corner of the window.  */
        SetCorner2      /**< Setting the 2nd corner of the window. */
    };

    struct Points;
    std::unique_ptr<Points> pPoints;
    enum RS2::EntityType typeToSelect = RS2::EntityType::EntityUnknown;
    bool select = false;
    bool selectIntersecting = false;

    RS2::CursorType doGetMouseCursor(int status) override;
    void onMouseLeftButtonRelease(int status, QMouseEvent *e) override;
    void onMouseRightButtonRelease(int status, QMouseEvent *e) override;
    void updateMouseButtonHints() override;
};
#endif
