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


#include "rs_dialogfactory.h"
#include "rs_debug.h"

RS_DialogFactory* RS_DialogFactory::uniqueInstance = NULL;
    


/**
 * Private constructor.
 */
RS_DialogFactory::RS_DialogFactory() {
	RS_DEBUG->print("RS_DialogFacgory::RS_DialogFactory");
	factoryObject = NULL;
	RS_DEBUG->print("RS_DialogFacgory::RS_DialogFactory: OK");
}



/**
 * @return Instance to the unique font list.
 */
RS_DialogFactory* RS_DialogFactory::instance() {
    RS_DEBUG->print("RS_DialogFactory::instance()");
    if (uniqueInstance==NULL) {
        uniqueInstance = new RS_DialogFactory();
    }

    RS_DEBUG->print("RS_DialogFactory::instance(): OK");

    return uniqueInstance;
}



/**
 * Sets the real factory object that can create and show dialogs.
 */
void RS_DialogFactory::setFactoryObject(RS_DialogFactoryInterface* fo) {
    RS_DEBUG->print("RS_DialogFactory::setFactoryObject");
    factoryObject = fo;
    RS_DEBUG->print("RS_DialogFactory::setFactoryObject: OK");
}



/**
 * @return Factory object. This is never NULL. If no factory
 * object was set, the default adapter will be returned.
 */
RS_DialogFactoryInterface* RS_DialogFactory::getFactoryObject() {
	RS_DEBUG->print("RS_DialogFactory::getFactoryObject");
    if (factoryObject!=NULL) {
		RS_DEBUG->print("RS_DialogFactory::getFactoryObject: "
			"returning factory object");
        return factoryObject;
    } else {
		RS_DEBUG->print("RS_DialogFactory::getFactoryObject: "
			"returning adapter");
        return &factoryAdapter;
    }
}



void RS_DialogFactory::commandMessage(const RS_String& m) {
	RS_DEBUG->print("RS_DialogFactory::commandMessage");

    if (factoryObject!=NULL) {
		factoryObject->commandMessage(m);
	}
}
