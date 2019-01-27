/****************************************************************************
**
 * Toggle whether a layer should appear on print (a construction layer doesn't appear on
 printout, and have straight lines of infinite length)

Copyright (C) 2011 Dongxu Li (dongxuli2011@gmail.com)
Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)

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


#ifndef RS_ACTIONLAYERSTOGGLEPRINT_H
#define RS_ACTIONLAYERSTOGGLEPRINT_H

#include "rs_actioninterface.h"

class RS_Layer;

/**
 * whether a layer should appear on print (a construction layer doesn't appear on
 printout, and have straight lines of infinite length)
 *
 * @author Dongxu Li
 */
class RS_ActionLayersTogglePrint : public RS_ActionInterface {
    Q_OBJECT
public:
    RS_ActionLayersTogglePrint(RS_EntityContainer& container,
                              RS_GraphicView& graphicView,
                               RS_Layer* layer);

	void init(int status=0) override;
	void trigger() override;

protected:
    RS_Layer* a_layer;

private:
    void deselectEntities(RS_Layer* layer);
};

#endif
