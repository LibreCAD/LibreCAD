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

#ifndef NO_COMPLEX_ENTITIES

#ifndef RS_ACTIONPARISDEBUGCREATECONTAINER_H
#define RS_ACTIONPARISDEBUGCREATECONTAINER_H

#include "rs_actioninterface.h"

/**
 * Creates an entity group without using blocks.
 *
 * @author Tobias Schulze, shhinfo.de
 */
class RS_ActionPARISDebugCreateContainer : public RS_ActionInterface {
	Q_OBJECT
public:
    RS_ActionPARISDebugCreateContainer(RS_EntityContainer& container,
                         RS_GraphicView& graphicView);
    virtual ~RS_ActionPARISDebugCreateContainer();
	
	virtual RS2::ActionType rtti() {
		return RS2::ActionPARISDebugCreateContainer;
	}
};

#endif
#endif
