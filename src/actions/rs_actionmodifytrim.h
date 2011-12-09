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

#ifndef RS_ACTIONMODIFYTRIM_H
#define RS_ACTIONMODIFYTRIM_H

#include "rs_previewactioninterface.h"


/**
 * This action class can handle user events to trim entities.
 *
 * @author Andrew Mustun
 */
class RS_ActionModifyTrim : public RS_PreviewActionInterface {
        Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        ChooseLimitEntity,     /**< Choosing the limiting entity. */
        ChooseTrimEntity       /**< Choosing the entity to trim. */
    };

public:
    RS_ActionModifyTrim(RS_EntityContainer& container,
                        RS_GraphicView& graphicView,
                        bool both=false);
    ~RS_ActionModifyTrim();

    static QAction* createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/);

    virtual void init(int status=0);
    virtual void trigger();
    virtual void mouseMoveEvent(QMouseEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();
    virtual void updateToolBar();
//    virtual void finish(bool updateTB = true );


private:
    RS_Entity* limitEntity;
        RS_Vector limitCoord;
    RS_Entity* trimEntity;
        RS_Vector trimCoord;
        bool both;
};

#endif
