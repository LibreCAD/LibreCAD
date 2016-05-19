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


#include "rs_dialogfactory.h"
#include "rs_debug.h"

/**
 * Private constructor.
 */
RS_DialogFactory::RS_DialogFactory():
	factoryObject{nullptr}
{
	RS_DEBUG->print("RS_DialogFacgory::RS_DialogFactory");
	RS_DEBUG->print("RS_DialogFacgory::RS_DialogFactory: OK");
}



/**
 * @return Instance to the unique font list.
 */
RS_DialogFactory* RS_DialogFactory::instance()
{
	static RS_DialogFactory* uniqueInstance = new RS_DialogFactory{};
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
 * @return Factory object. This is never nullptr. If no factory
 * object was set, the default adapter will be returned.
 */
RS_DialogFactoryInterface* RS_DialogFactory::getFactoryObject()
{
	return factoryObject ? factoryObject : &factoryAdapter;
}



void RS_DialogFactory::commandMessage(const QString& m) {
	RS_DEBUG->print("RS_DialogFactory::commandMessage");

	if (factoryObject)
		factoryObject->commandMessage(m);
}
