/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

#ifndef QC_ACTIONGETENT_H
#define QC_ACTIONGETENT_H

#include "rs_previewactioninterface.h"
#include "rs_modification.h"

 class Plugin_Entity;
 class Doc_plugin_interface;

/**
 * This action class can handle user events to select entities from plugin.
 *
 * @author  Rallaz
 */

class QC_ActionGetEnt : public RS_ActionInterface {
	Q_OBJECT
//public:
    /**
     * Action States.
     */
/*    enum Status {
        Select
    };*/

public:
    QC_ActionGetEnt(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
    ~QC_ActionGetEnt() {}

    virtual void updateMouseButtonHints();

/*    virtual void init(int status=0);
	
    virtual void mouseReleaseEvent(QMouseEvent* e);
	
    virtual void updateMouseCursor();
    virtual void updateToolBar();*/

    virtual void trigger();
    virtual void keyPressEvent(QKeyEvent* e);
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void updateMouseCursor();

    void setMesage(QString msg);
    bool isCompleted(){return completed;}
    Plugin_Entity *getSelected(Doc_plugin_interface* d);

private:
    bool completed;
    QString mesage;
    RS_Entity* en;

};

#endif
