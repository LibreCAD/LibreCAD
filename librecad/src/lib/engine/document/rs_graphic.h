/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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
 ******************************************************************************/


#ifndef RS_GRAPHIC_H
#define RS_GRAPHIC_H

#include <QDateTime>
#include "rs_blocklist.h"
#include "rs_layerlist.h"
#include "rs_variabledict.h"
#include "rs_document.h"
#include "lc_view.h"
#include "lc_viewslist.h"

class QG_LayerWidget;

/**
 * A graphic document which can contain entities layers and blocks.
 *
 * @author Andrew Mustun
 */
class RS_Graphic : public RS_Document {
public:
    RS_Graphic(RS_EntityContainer* parent=nullptr);
    virtual ~RS_Graphic();

    /** @return RS2::EntityGraphic */
    RS2::EntityType rtti() const override {return RS2::EntityGraphic;}

    virtual unsigned long int countLayerEntities(RS_Layer* layer);

    RS_LayerList* getLayerList() override {return &layerList;}
    RS_BlockList* getBlockList() override {return &blockList;}
    LC_ViewList* getViewList() override {return &namedViewsList;}

    void newDoc() override;
    bool save(bool isAutoSave = false) override;
    bool saveAs(const QString& filename, RS2::FormatType type, bool force = false) override;
    bool open(const QString& filename, RS2::FormatType type) override;
    bool loadTemplate(const QString &filename, RS2::FormatType type) override;

        // Wrappers for Layer functions:
    void clearLayers() {layerList.clear();}
    unsigned countLayers() const {return layerList.count();}
    RS_Layer* layerAt(unsigned i) {return layerList.at(i);}
    void activateLayer(const QString& name, bool notify = false) {layerList.activate(name, notify);}
    void activateLayer(RS_Layer* layer, bool notify = false) {layerList.activate(layer, notify);}
    RS_Layer* getActiveLayer() {return layerList.getActive();}
    virtual void addLayer(RS_Layer* layer) {layerList.add(layer);}
    void addEntity(RS_Entity* entity) override;
    virtual void removeLayer(RS_Layer* layer);
    virtual void editLayer(RS_Layer* layer, const RS_Layer& source) {layerList.edit(layer, source);}
    RS_Layer* findLayer(const QString& name) {return layerList.find(name);}
    void toggleLayer(const QString& name) {layerList.toggle(name);}
    void toggleLayer(RS_Layer* layer) {layerList.toggle(layer);}
    void toggleLayerLock(RS_Layer* layer) {layerList.toggleLock(layer);}
    void toggleLayerPrint(RS_Layer* layer) {layerList.togglePrint(layer);}
    void toggleLayerConstruction(RS_Layer* layer) {layerList.toggleConstruction(layer);}
    void freezeAllLayers(bool freeze) {layerList.freezeAll(freeze);}
    void lockAllLayers(bool lock) {layerList.lockAll(lock);}
    void addLayerListListener(RS_LayerListListener* listener) {layerList.addListener(listener);}
    void removeLayerListListener(RS_LayerListListener* listener) {layerList.removeListener(listener);}

    void addViewListListener(LC_ViewListListener* listener) { namedViewsList.addListener(listener);}
    void removeViewListListener(LC_ViewListListener* listener) { namedViewsList.removeListener(listener);}

    // Wrapper for block functions:
    void clearBlocks() {blockList.clear();}
    unsigned countBlocks() {return blockList.count();}
    RS_Block* blockAt(unsigned i) {return blockList.at(i);}
    void activateBlock(const QString& name) {blockList.activate(name);}
    void activateBlock(RS_Block* block) {blockList.activate(block);}
    RS_Block* getActiveBlock() {return blockList.getActive();}
    virtual bool addBlock(RS_Block* block, bool notify=true) {return blockList.add(block, notify);}
    virtual void addBlockNotification() {blockList.addNotification();}
    virtual void removeBlock(RS_Block* block) {blockList.remove(block);}
    RS_Block* findBlock(const QString& name) {return blockList.find(name);}
    QString newBlockName() {return blockList.newName();}
    void toggleBlock(const QString& name) {blockList.toggle(name);}
    void toggleBlock(RS_Block* block) {blockList.toggle(block);}
    void freezeAllBlocks(bool freeze) {blockList.freezeAll(freeze);}
    void addBlockListListener(RS_BlockListListener* listener) {blockList.addListener(listener);}
    void removeBlockListListener(RS_BlockListListener* listener) {blockList.removeListener(listener);}

        // Wrappers for variable functions:
    void clearVariables();
    int countVariables();

    void addVariable(const QString& key, const RS_Vector& value, int code);
    void addVariable(const QString& key, const QString& value, int code);
    void addVariable(const QString& key, int value, int code);
    void addVariable(const QString& key, double value, int code);
    void removeVariable(const QString& key);

    QHash<QString, RS_Variable>& getVariableDict();
    RS_Vector getVariableVector(const QString& key, const RS_Vector& def) const;
    QString getVariableString(const QString& key, const QString& def) const;
    int getVariableInt(const QString& key, int def) const;
    double getVariableDouble(const QString& key, double def) const;

    RS_VariableDict getVariableDictObject() {return variableDict;}

    void setVariableDictObject(RS_VariableDict inputVariableDict) {variableDict = inputVariableDict;}

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

    double getPaperScale() const;
    void setPaperScale(double s);

    virtual void setUnit(RS2::Unit u);
    virtual RS2::Unit getUnit() const;

    bool isGridOn() const;
    void setGridOn(bool on);
    bool isIsometricGrid() const;
    void setIsometricGrid(bool on);
    RS2::IsoGridViewType getIsoView() const;
    void setIsoView(RS2::IsoGridViewType viewType);
    void centerToPage();
    bool fitToPage();
    bool isBiggerThanPaper();
    /**
     * @retval true The document has been modified since it was last saved.
     * @retval false The document has not been modified since it was last saved.
     */
    bool isModified() const override {return modified || layerList.isModified() || blockList.isModified() || namedViewsList.isModified();}

    /**
     * Sets the documents modified status to 'm'.
     */
    void setModified(bool m) override{
        modified = m;
        if (!m) {
            layerList.setModified(m);
            blockList.setModified(m);
            namedViewsList.setModified(m);
        }
    }

    virtual QDateTime getModifyTime(){return modifiedTime;}

    //if set to true, will refuse to modify paper scale
    void setPaperScaleFixed(bool fixed){paperScaleFixed=fixed;}
    bool getPaperScaleFixed() const{return paperScaleFixed;}

    /**
     * Paper margins in millimeters
     */
    void setMargins(double left, double top, double right, double bottom){
        if (left >= 0.0) marginLeft = left;
        if (top >= 0.0) marginTop = top;
        if (right >= 0.0) marginRight = right;
        if (bottom >= 0.0) marginBottom = bottom;
    }

    double getMarginLeft() const{return marginLeft;}
    double getMarginTop() const{return marginTop;}
    double getMarginRight() const{ return marginRight;}
    double getMarginBottom() const{return marginBottom;}

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
    int getPagesNumHoriz() const {return pagesNumH;}
    int getPagesNumVert() const {return pagesNumV;}
    friend std::ostream& operator << (std::ostream& os, RS_Graphic& g);
    int clean();

    LC_View *findNamedView(QString viewName) {return namedViewsList.find(viewName);};

    void addNamedView(LC_View *view) {namedViewsList.add(view);};

private:

    bool BackupDrawingFile(const QString &filename);
    QDateTime modifiedTime;
    QString currentFileName; //keep a copy of filename for the modifiedTime

    RS_LayerList layerList;
    RS_BlockList blockList;
    RS_VariableDict variableDict;
    LC_ViewList namedViewsList;
    //if set to true, will refuse to modify paper scale
    bool paperScaleFixed = false;

    // Paper margins in millimeters
    double marginLeft = 0.;
    double marginTop = 0.;
    double marginRight = 0.;
    double marginBottom = 0.;

    // Number of pages drawing occupies
    int pagesNumH = 1;
    int pagesNumV = 1;


};
#endif
