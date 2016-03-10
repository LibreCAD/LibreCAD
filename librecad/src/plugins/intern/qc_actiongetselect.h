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

#ifndef QC_ACTIONGETSELECT_H
#define QC_ACTIONGETSELECT_H

#include "rs_previewactioninterface.h"
#include "rs_modification.h"

class Plug_Entity;
class Doc_plugin_interface;


/**
 * This action class can handle user events to select entities from plugin.
 *
 * @author  Rallaz
 */
class QC_ActionGetSelect : public RS_ActionInterface {
	Q_OBJECT
public:
    /**
     * Action States.
     */
    enum Status {
        Select
    };

public:
    QC_ActionGetSelect(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);
	~QC_ActionGetSelect();

    virtual void init(int status=0);
	
    virtual void mouseReleaseEvent(QMouseEvent* e);
    virtual void keyPressEvent(QKeyEvent* e);
	
    virtual void updateMouseButtonHints();
    virtual void updateMouseCursor();

    void setMesage(QString msg);
	bool isCompleted() const{return completed;}
	void getSelected(QList<Plug_Entity *> *se, Doc_plugin_interface* d) const;

private:
    bool completed;
	QString message;

};

#endif
