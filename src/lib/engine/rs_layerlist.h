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


#ifndef RS_LAYERLIST_H
#define RS_LAYERLIST_H

//#include <vector.h>


#include "rs_layer.h"
#include "rs_layerlistlistener.h"
#include "rs_entity.h"
#include "rs_ptrlist.h"

/**
 * A list of layers.
 *
 * @author Andrew Mustun
 */
class RS_LayerList {
public:
    RS_LayerList();
    virtual ~RS_LayerList() {}

    void clear();

    /**
     * @return Number of layers in the list.
     */
    uint count() const {
        return layers.count();
    }

    /**
     * @return Layer at given position or NULL if i is out of range.
     */
    RS_Layer* at(uint i) {
        return layers.at(i);
    }

    void activate(const RS_String& name, bool notify = false);
    void activate(RS_Layer* layer, bool notify = false);
    //! @return The active layer of NULL if no layer is activated.
    RS_Layer* getActive() {
        return activeLayer;
    }
    virtual void add(RS_Layer* layer);
    virtual void remove(RS_Layer* layer);
    virtual void edit(RS_Layer* layer, const RS_Layer& source);
    RS_Layer* find(const RS_String& name);
    int getIndex(const RS_String& name);
    int getIndex(RS_Layer* layer);
    void toggle(const RS_String& name);
    void toggle(RS_Layer* layer);
    void toggleLock(RS_Layer* layer);
    void freezeAll(bool freeze);
    //! @return First layer of the list.
    //RS_Layer* firstLayer() {
    //    return layers.first();
    //}
    /** @return Next layer from the list after
     * calling firstLayer() or nextLayer().
     */
    //RS_Layer* nextLayer() {
    //    return layers.next();
    //}

    void addListener(RS_LayerListListener* listener);
    void removeListener(RS_LayerListListener* listener);
	
	/**
	 * Sets the layer lists modified status to 'm'.
	 */
	void setModified(bool m) {
		modified = m;
	}
	
	/**
	 * @retval true The layer list has been modified.
	 * @retval false The layer list has not been modified.
	 */
    virtual bool isModified() const {
        return modified;
    }

    friend std::ostream& operator << (std::ostream& os, RS_LayerList& l);

private:
    //! layers in the graphic
    RS_PtrList<RS_Layer> layers;
    //! List of registered LayerListListeners
    RS_PtrList<RS_LayerListListener> layerListListeners;
    //! Currently active layer
    RS_Layer* activeLayer;
    /** Flag set if the layer list was modified and not yet saved. */
    bool modified;
};

#endif
