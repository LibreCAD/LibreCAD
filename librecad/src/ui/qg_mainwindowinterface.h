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

#ifndef QG_MAINWINDOWINTERFACE_H
#define QG_MAINWINDOWINTERFACE_H

#include <QMainWindow>

#include "rs_document.h"
#include "rs_mainwindowinterface.h"

#include "qg_graphicview.h"
#include "qg_actionhandler.h"



/**
 * Interface for main application windows.
 */
class QG_MainWindowInterface : public RS_MainWindowInterface {

public:
	QG_MainWindowInterface() = default;
	virtual ~QG_MainWindowInterface() = default;

	virtual QMainWindow* getMainWindow() = 0;
	virtual QMainWindow const* getMainWindow() const= 0;
	virtual QG_ActionHandler const* getActionHandler() const= 0;
	virtual QG_ActionHandler* getActionHandler()= 0;
	virtual void setFocus2() = 0;

        //virtual QToolBar* createToolBar(const QString& name) = 0;
	//virtual void addToolBarButton(QToolBar* tb) = 0;
};

#endif

