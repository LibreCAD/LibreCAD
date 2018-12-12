/****************************************************************************
This file is part of the LibreCAD project, a 2D CAD program

** Copyright (C) 2012 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#ifndef RS_ACTIONBLOCKSSAVE_H
#define RS_ACTIONBLOCKSSAVE_H

#include "rs_actioninterface.h"

class RS_Insert;

/**
 * This action class can handle user events to save the active block to a file.
 *
 * @author Dongxu Li
 */
class RS_ActionBlocksSave : public RS_ActionInterface {
	Q_OBJECT
public:
    RS_ActionBlocksSave(RS_EntityContainer& container,
                        RS_GraphicView& graphicView);

	void init(int status=0) override;
	void trigger() override;

private:
    void addBlock(RS_Insert* in, RS_Graphic* g);
};

#endif
