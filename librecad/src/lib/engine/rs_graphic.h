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


#ifndef RS_GRAPHIC_H
#define RS_GRAPHIC_H

#include <QDateTime>
#include "rs_blocklist.h"
#include "rs_layerlist.h"
#include "rs_variabledict.h"
#include "rs_document.h"
#include "rs_units.h"

class RS_VariableDict;
class QG_LayerWidget;

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
    virtual bool save(bool isAutoSave = false);
    virtual bool saveAs(const QString& filename, RS2::FormatType type, bool force = false);
    virtual bool open(const QString& filename, RS2::FormatType type);
    bool loadTemplate(const QString &filename, RS2::FormatType type);

        // Wrappers for Layer functions:
    void clearLayers() {
        layerList.clear();
    }
    unsigned countLayers() const {
        return layerList.count();
    }
    RS_Layer* layerAt(unsigned i) {
        return layerList.at(i);
    }
    void activateLayer(const QString& name) {
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
    virtual void addEntity(RS_Entity* entity);
    virtual void removeLayer(RS_Layer* layer);
    virtual void editLayer(RS_Layer* layer, const RS_Layer& source) {
        layerList.edit(layer, source);
    }
    RS_Layer* findLayer(const QString& name) {
        return layerList.find(name);
    }
    void toggleLayer(const QString& name) {
        layerList.toggle(name);
    }
    void toggleLayer(RS_Layer* layer) {
        layerList.toggle(layer);
    }
    void toggleLayerLock(RS_Layer* layer) {
        layerList.toggleLock(layer);
    }
    void toggleLayerPrint(RS_Layer* layer) {
        layerList.togglePrint(layer);
    }
    void toggleLayerConstruction(RS_Layer* layer) {
        layerList.toggleConstruction(layer);
    }
    void freezeAllLayers(bool freeze) {
        layerList.freezeAll(freeze);
    }
    void lockAllLayers(bool lock) {
        layerList.lockAll(lock);
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
    unsigned countBlocks() {
        return blockList.count();
    }
    RS_Block* blockAt(unsigned i) {
        return blockList.at(i);
    }
    void activateBlock(const QString& name) {
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
    RS_Block* findBlock(const QString& name) {
        return blockList.find(name);
    }
    QString newBlockName() {
        return blockList.newName();
    }
    void toggleBlock(const QString& name) {
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

    void addVariable(const QString& key, const RS_Vector& value, int code) {
        variableDict.add(key, value, code);
    }
    void addVariable(const QString& key, const QString& value, int code) {
        variableDict.add(key, value, code);
    }
    void addVariable(const QString& key, int value, int code) {
        variableDict.add(key, value, code);
    }
    void addVariable(const QString& key, double value, int code) {
        variableDict.add(key, value, code);
    }

    RS_Vector getVariableVector(const QString& key, const RS_Vector& def) {
        return variableDict.getVector(key, def);
    }
    QString getVariableString(const QString& key, const QString& def) {
        return variableDict.getString(key, def);
    }
    int getVariableInt(const QString& key, int def) {
        return variableDict.getInt(key, def);
    }
    double getVariableDouble(const QString& key, double def) {
        return variableDict.getDouble(key, def);
    }

    void removeVariable(const QString& key) {
        variableDict.remove(key);
    }

    QHash<QString, RS_Variable>& getVariableDict() {
        return variableDict.getVariableDict();
    }

    RS2::LinearFormat getLinearFormat();
    RS2::LinearFormat getLinearFormat(int f);
    int getLinearPrecision();
    RS2::AngleFormat getAngleFormat();
    int getAnglePrecision();

    RS_Vector getPaperSize();
    void setPaperSize(const RS_Vector& s);
    RS_Vector getPrintAreaSize(bool total=true);

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
    bool isIsometricGrid();
    void setIsometricGrid(bool on);
    void setCrosshairType(RS2::CrosshairType chType);
    RS2::CrosshairType getCrosshairType();

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
    bool fitToPage();

    bool isBiggerThanPaper();

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
    virtual QDateTime getModifyTime(void){
        return modifiedTime;
    }

    //if set to true, will refuse to modify paper scale
    void setPaperScaleFixed(bool fixed)
    {
        paperScaleFixed=fixed;
    }
    bool getPaperScaleFixed()
    {
        return paperScaleFixed;
    }

    /**
     * Paper margins in millimeters
     */
    void setMargins(double left, double top, double right, double bottom)
    {
        if (left >= 0.0) marginLeft = left;
        if (top >= 0.0) marginTop = top;
        if (right >= 0.0) marginRight = right;
        if (bottom >= 0.0) marginBottom = bottom;
    }
    double getMarginLeft() const
    {
        return marginLeft;
    }
    double getMarginTop() const
    {
        return marginTop;
    }
    double getMarginRight() const
    {
        return marginRight;
    }
    double getMarginBottom() const
    {
        return marginBottom;
    }

    /**
     * Paper margins in graphic units
     */
    void setMarginsInUnits(double left, double top, double right, double bottom);
    double getMarginLeftInUnits();
    double getMarginTopInUnits();
    double getMarginRightInUnits();
    double getMarginBottomInUnits();

    /**
     * Number of pages drawing occupies
     */
    void setPagesNum(int horiz, int vert);
    void setPagesNum(const QString &horizXvert);
    int getPagesNumHoriz() {
        return pagesNumH;
    }
    int getPagesNumVert() {
        return pagesNumV;
    }

    friend std::ostream& operator << (std::ostream& os, RS_Graphic& g);

    int clean();

private:

        bool BackupDrawingFile(const QString &filename);
        QDateTime modifiedTime;
        QString currentFileName; //keep a copy of filename for the modifiedTime

        RS_LayerList layerList;
        RS_BlockList blockList;
        RS_VariableDict variableDict;
        RS2::CrosshairType crosshairType; //corss hair type used by isometric grid
        //if set to true, will refuse to modify paper scale
        bool paperScaleFixed;

        // Paper margins in millimeters
        double marginLeft;
        double marginTop;
        double marginRight;
        double marginBottom;

        // Number of pages drawing occupies
        int pagesNumH;
        int pagesNumV;
};


#endif
