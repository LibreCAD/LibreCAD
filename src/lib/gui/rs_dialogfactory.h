/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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


#ifndef RS_DIALOGFACTORY_H
#define RS_DIALOGFACTORY_H

#include "rs_dialogfactoryinterface.h"
#include "rs_dialogfactoryadapter.h"
#include "rs_debug.h"


#define RS_DIALOGFACTORY RS_DialogFactory::instance()->getFactoryObject()

/**
 * Interface for objects that can create and show dialogs.
 */
class RS_DialogFactory {

private:
    RS_DialogFactory();

public:
    virtual ~RS_DialogFactory() {}

    static RS_DialogFactory* instance();

	void setFactoryObject(RS_DialogFactoryInterface* fo);
	RS_DialogFactoryInterface* getFactoryObject();

	void commandMessage(const RS_String& m);

private:
	RS_DialogFactoryInterface* factoryObject;
	RS_DialogFactoryAdapter factoryAdapter;
	static RS_DialogFactory* uniqueInstance;
};


#endif
