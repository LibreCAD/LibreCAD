/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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


#ifndef RS_LAYERLISTLISTENER_H
#define RS_LAYERLISTLISTENER_H

#include "rs_layer.h"

/**
 * This class is an interface for classes that are interested in
 * knowing about changes in the layer list. 
 */
class RS_LayerListListener {
public:
    RS_LayerListListener() {}
    virtual ~RS_LayerListListener() {}

    /**
     * Called when the active layer changes.
     */
    virtual void layerActivated(RS_Layer*) {}

    /**
     * Called when a new layer is added to the list.
     */
    virtual void layerAdded(RS_Layer*) {}

    /**
     * Called when a layer is removed from the list.
     */
    virtual void layerRemoved(RS_Layer*) {}

    /**
     * Called when a layer's attributes are modified.
     */
    virtual void layerEdited(RS_Layer*) {}

    /**
     * Called when a layer's visibility is toggled.
     */
    virtual void layerToggled(RS_Layer*) {}

    /**
     * Called when a layer's lock attribute is toggled.
     */
    virtual void layerToggledLock(RS_Layer*) {}

    /**
     * Called when a layer's printing attribute is toggled.
     */
    virtual void layerToggledPrint(RS_Layer*) {}

    /**
     * Called when a layer's construction attribute is toggled.
     */
    virtual void layerToggledConstruction(RS_Layer*) {}

    /**
     * Called when layer list is modified.
     */
    virtual void layerListModified(bool) {}
}
;

#endif
