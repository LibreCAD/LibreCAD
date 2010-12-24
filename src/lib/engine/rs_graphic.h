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


#ifndef RS_GRAPHIC_H
#define RS_GRAPHIC_H

#include "rs_blocklist.h"
#include "rs_layerlist.h"
#include "rs_variabledict.h"
#include "rs_document.h"
#include "rs_units.h"
#ifdef RS_CAM
#include "rs_camdata.h"
#endif

//class RS_CamData;


/**
 * A graphic document which can contain entities layers and blocks.
 *
 * @author Andrew Mustun
 */
class RS_Graphic : public RS_Document {
public:
    RS_Graphic(RS_EntityContainer* parent=NULL);
    virtual ~RS_Graphic();

    //virtual RS_Entity* clone() {
    //	return new RS_Graphic(*this);
    //}

    /** @return RS2::EntityGraphic */
    virtual RS2::EntityType rtti() const {
        return RS2::EntityGraphic;
    }

    virtual unsigned long int countLayerEntities(RS_Layer* layer);

    virtual RS_LayerList* getLayerList() {
        return &layerList;
    }
    virtual RS_BlockList* getBlockList() {
        return &blockList;
    }

    virtual void newDoc();
    virtual bool save();
    virtual bool saveAs(const RS_String& filename, RS2::FormatType type);
    virtual bool open(const RS_String& filename, RS2::FormatType type);
	
	// Wrappers for Layer functions:
    void clearLayers() {
		layerList.clear();
	}
    uint countLayers() const {
        return layerList.count();
    }
    RS_Layer* layerAt(uint i) {
        return layerList.at(i);
    }
    void activateLayer(const RS_String& name) {
		layerList.activate(name);
	}
    void activateLayer(RS_Layer* layer) {
		layerList.activate(layer);
	}
    RS_Layer* getActiveLayer() {
        return layerList.getActive();
    }
    virtual void addLayer(RS_Layer* layer) {
		layerList.add(layer);
	}
    virtual void removeLayer(RS_Layer* layer);
    virtual void editLayer(RS_Layer* layer, const RS_Layer& source) {
		layerList.edit(layer, source);
	}
    RS_Layer* findLayer(const RS_String& name) {
		return layerList.find(name);
	}
    void toggleLayer(const RS_String& name) {
		layerList.toggle(name);
	}
    void toggleLayer(RS_Layer* layer) {
		layerList.toggle(layer);
	}
    void toggleLayerLock(RS_Layer* layer) {
		layerList.toggleLock(layer);
	}
    void freezeAllLayers(bool freeze) {
		layerList.freezeAll(freeze);
	}

    void addLayerListListener(RS_LayerListListener* listener) {
		layerList.addListener(listener);
	}
    void removeLayerListListener(RS_LayerListListener* listener) {
		layerList.removeListener(listener);
	}


	// Wrapper for block functions:
    void clearBlocks() {
		blockList.clear();
	}
    uint countBlocks() {
        return blockList.count();
    }
    RS_Block* blockAt(uint i) {
        return blockList.at(i);
    }
    void activateBlock(const RS_String& name) {
		blockList.activate(name);
	}
    void activateBlock(RS_Block* block) {
		blockList.activate(block);
	}
    RS_Block* getActiveBlock() {
        return blockList.getActive();
    }
    virtual bool addBlock(RS_Block* block, bool notify=true) {
		return blockList.add(block, notify);
	}
    virtual void addBlockNotification() {
		blockList.addNotification();
	}
    virtual void removeBlock(RS_Block* block) {
		blockList.remove(block);
	}
    RS_Block* findBlock(const RS_String& name) {
		return blockList.find(name);
	}
    RS_String newBlockName() {
		return blockList.newName();
	}
    void toggleBlock(const RS_String& name) {
		blockList.toggle(name);
	}
    void toggleBlock(RS_Block* block) {
		blockList.toggle(block);
	}
    void freezeAllBlocks(bool freeze) {
		blockList.freezeAll(freeze);
	}
    void addBlockListListener(RS_BlockListListener* listener) {
		blockList.addListener(listener);
	}
    void removeBlockListListener(RS_BlockListListener* listener) {
		blockList.removeListener(listener);
	}

	// Wrappers for variable functions:
    void clearVariables() {
		variableDict.clear();
	}
    int countVariables() {
		return variableDict.count();
    }

    void addVariable(const RS_String& key, const RS_Vector& value, int code) {
		variableDict.add(key, value, code);
	}
    void addVariable(const RS_String& key, const RS_String& value, int code) {
		variableDict.add(key, value, code);
	}
    void addVariable(const RS_String& key, int value, int code) {
		variableDict.add(key, value, code);
	}
    void addVariable(const RS_String& key, double value, int code) {
		variableDict.add(key, value, code);
	}

    RS_Vector getVariableVector(const RS_String& key, const RS_Vector& def) {
		return variableDict.getVector(key, def);
	}
    RS_String getVariableString(const RS_String& key, const RS_String& def) {
		return variableDict.getString(key, def);
	}
    int getVariableInt(const RS_String& key, int def) {
		return variableDict.getInt(key, def);
	}
    double getVariableDouble(const RS_String& key, double def) {
		return variableDict.getDouble(key, def);
	}

    void removeVariable(const RS_String& key) {
		variableDict.remove(key);
	}

	RS_Dict<RS_Variable>& getVariableDict() {
		return variableDict.getVariableDict();
	}

	RS2::LinearFormat getLinearFormat();
	int getLinearPrecision();
	RS2::AngleFormat getAngleFormat();
	int getAnglePrecision();

	RS_Vector getPaperSize();
	void setPaperSize(const RS_Vector& s);
	
	RS_Vector getPaperInsertionBase();
	void setPaperInsertionBase(const RS_Vector& p);
	
	RS2::PaperFormat getPaperFormat(bool* landscape);
	void setPaperFormat(RS2::PaperFormat f, bool landscape);

	double getPaperScale();
	void setPaperScale(double s);

    virtual void setUnit(RS2::Unit u);
    virtual RS2::Unit getUnit();

	bool isGridOn();
	void setGridOn(bool on);
	
	bool isDraftOn();
	void setDraftOn(bool on);

    /** Sets the unit of this graphic's dimensions to 'u' */
    /*virtual void setDimensionUnit(RS2::Unit u) {
		addVariable("$INSUNITS", (int)u, 70);
        dimensionUnit = u;
    }*/

    /** Gets the unit of this graphic's dimension */
    /*virtual RS2::Unit getDimensionUnit() {
        return dimensionUnit;
    }*/

	void centerToPage();
	void fitToPage();
	
	/**
	 * @retval true The document has been modified since it was last saved.
	 * @retval false The document has not been modified since it was last saved.
	 */
    virtual bool isModified() const {
        return modified || layerList.isModified() || blockList.isModified();
    }
	
	/**
	 * Sets the documents modified status to 'm'.
	 */
	virtual void setModified(bool m) {
		modified = m;
		layerList.setModified(m);
		blockList.setModified(m);
	}

#ifdef RS_CAM
	RS_CamData& getCamData() {
		return camData;
	}
	void setCamData(const RS_CamData& d) {
		camData = d;
	}
#endif

    friend std::ostream& operator << (std::ostream& os, RS_Graphic& g);

private:
	RS_LayerList layerList;
	RS_BlockList blockList;
	RS_VariableDict variableDict;
#ifdef RS_CAM
	RS_CamData camData;
#endif
};


#endif
