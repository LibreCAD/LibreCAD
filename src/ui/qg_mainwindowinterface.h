/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
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

#ifndef QG_MAINWINDOWINTERFACE_H
#define QG_MAINWINDOWINTERFACE_H

#include <qmainwindow.h>

#include "rs_document.h"
#include "rs_mainwindowinterface.h"

#include "qg_graphicview.h"
#include "qg_actionhandler.h"



/**
 * Interface for main application windows.
 */
class QG_MainWindowInterface : public RS_MainWindowInterface {

public:
    QG_MainWindowInterface() {}
    virtual ~QG_MainWindowInterface() {}

    virtual QMainWindow* getMainWindow() = 0;
    virtual QG_ActionHandler* getActionHandler() = 0;
	virtual void setFocus2() = 0;

	//virtual QToolBar* createToolBar(const RS_String& name) = 0;
	//virtual void addToolBarButton(QToolBar* tb) = 0;
};

#endif

