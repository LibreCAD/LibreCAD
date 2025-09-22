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

#include <memory>
#include <QDateTime>

#include "lc_dimstyle.h"
#include "lc_ucslist.h"
#include "lc_viewslist.h"
#include "rs_blocklist.h"
#include "rs_document.h"
#include "rs_layerlist.h"
#include "rs_variabledict.h"
#include "lc_dimstyleslist.h"
#include "lc_textstylelist.h"

class RS_Dimension;
class LC_DimStyleToVariablesMapper;
class LC_DimStylesList;
class QString;

class LC_View;
class QG_LayerWidget;

class LC_GraphicModificationListener {
public:
    virtual ~LC_GraphicModificationListener() = default;
    virtual void graphicModified(const RS_Graphic* g, bool modified) = 0;
    virtual void undoStateChanged(const RS_Graphic* g, bool undoAvailable, bool redoAvailable) = 0;
};

/**
 * A graphic document which can contain entities layers and blocks.
 *
 * @author Andrew Mustun
 */
// fixme - sand - refactor and extract entities, so it should be more natual to DXF.
// Paper-related things should be layouts, ui viewports should be also exposed explicitly ...
// At least "active" one, so viewport will be accessible via document, not via graphic view.
class RS_Graphic : public RS_Document {
public:
    RS_Graphic(RS_EntityContainer* parent=nullptr);
    ~RS_Graphic() override;

    virtual void onLoadingCompleted();
    /** @return RS2::EntityGraphic */
    RS2::EntityType rtti() const override {return RS2::EntityGraphic;}

    virtual unsigned countLayerEntities(RS_Layer* layer) const;

    RS_LayerList* getLayerList() override {return &layerList;}
    RS_BlockList* getBlockList() override {return &blockList;}
    LC_ViewList* getViewList() override {return &namedViewsList;}
    LC_UCSList* getUCSList() override {return &ucsList;}
    LC_DimStylesList* getDimStyleList() override {return &dimstyleList;}
    LC_TextStyleList* getTextStyleList() override {return &textStyleList;}
    void addDimStyle(LC_DimStyle* style) {dimstyleList.addDimStyle(style);}
    void newDoc() override;
    // Wrappers for Layer functions:
    void clearLayers() {layerList.clear();}
    unsigned countLayers() const {return layerList.count();}
    RS_Layer* layerAt(unsigned i) {return layerList.at(i);}
    void activateLayer(const QString& name, bool notify = false) {layerList.activate(name, notify);}
    void activateLayer(RS_Layer* layer, bool notify = false) {layerList.activate(layer, notify);}
    RS_Layer*   getActiveLayer() const {return layerList.getActive();}
    virtual void addLayer(RS_Layer* layer) {layerList.add(layer);}
    void addEntity(RS_Entity* entity) override;
    void removeLayer(RS_Layer* layer);
    void editLayer(RS_Layer* layer, const RS_Layer& source) {layerList.edit(layer, source);}
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
    RS_Block* getActiveBlock() const {return blockList.getActive();}
    bool addBlock(RS_Block* block, bool notify=true) {return blockList.add(block, notify);}
    void addBlockNotification() {blockList.addNotification();}
    void removeBlock(RS_Block* block) {blockList.remove(block);}
    RS_Block* findBlock(const QString& name) {return blockList.find(name);}
    QString newBlockName() {return blockList.newName();}
    void toggleBlock(const QString& name) {blockList.toggle(name);}
    void toggleBlock(RS_Block* block) {blockList.toggle(block);}
    void freezeAllBlocks(bool freeze) {blockList.freezeAll(freeze);}
    void addBlockListListener(RS_BlockListListener* listener) {blockList.addListener(listener);}
    void removeBlockListListener(RS_BlockListListener* listener) {blockList.removeListener(listener);}

        // Wrappers for variable functions:
    void clearVariables();
    QString getCustomProperty(const QString& key, const QString& defaultValue);
    void addCustomProperty(const QString& key, const QString& value);
    void removeCustomProperty(const QString& key);
    bool hasCustomProperty(const QString& key);
    const QHash<QString, RS_Variable>& getCustomProperties() const;
    int countVariables() const;

    void addVariable(const QString& key, const RS_Vector& value, int code);
    void addVariable(const QString& key, const QString& value, int code);
    void addVariable(const QString& key, int value, int code);
    void addVariable(const QString& key, bool value, int code);
    void addVariable(const QString& key, double value, int code);
    void removeVariable(const QString& key);

    QHash<QString, RS_Variable>& getVariableDict();
    RS_Vector getVariableVector(const QString& key, const RS_Vector& def) const;
    QString getVariableString(const QString& key, const QString& def) const;
    int getVariableInt(const QString& key, int def) const;
    bool getVariableBool(const QString& key, bool def) const;
    double getVariableDouble(const QString& key, double def) const;

    void setVariableDictObject(RS_VariableDict inputVariableDict) {m_variableDict = inputVariableDict;}

    RS_VariableDict getVariableDictObject() const{
        return m_variableDict;
    }

    RS_VariableDict* getVariableDictObjectRef() {
        return &m_variableDict;
    }

    RS2::LinearFormat getLinearFormat() const;
    void replaceCustomVars(const QHash<QString, QString>& hash);
    virtual void prepareForSave();

    static RS2::LinearFormat convertLinearFormatDXF2LC(int f);
    int getLinearPrecision() const;
    RS2::AngleFormat getAngleFormat() const;
    int getAnglePrecision() const;

    RS_Vector getPaperSize() const;
    void setPaperSize(const RS_Vector& s);
    RS_Vector getPrintAreaSize(bool total=true) const;

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
    void setCurrentUCS(LC_UCS* ucs);
    LC_UCS* getCurrentUCS() const;
    RS2::IsoGridViewType getIsoView() const;
    void setIsoView(RS2::IsoGridViewType viewType);
    void centerToPage();
    bool fitToPage();
    bool isBiggerThanPaper();
    /**
     * @retval true The document has been modified since it was last saved.
     * @retval false The document has not been modified since it was last saved.
     */
    bool isModified() const override;
    /**
     * Sets the documents modified status to 'm'.
     */
    void setModified(bool m) override;
    void markSaved(const QDateTime &lastSaveTime);

    QDateTime getLastSaveTime(){return lastSaveTime;}
    void setLastSaveTime(const QDateTime &time) { lastSaveTime = time;}

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
    LC_UCS *findNamedUCS(QString ucsName) {return ucsList.find(ucsName);};
    void addNamedView(LC_View *view) {namedViewsList.add(view);};
    void addUCS(LC_UCS *ucs) {ucsList.add(ucs);};

    double getAnglesBase() const;
    void setAnglesBase(double baseAngle);
    bool areAnglesCounterClockWise() const;
    void setAnglesCounterClockwise(bool on);
    QString formatAngle(double angle) const;
    QString formatLinear(double linear) const;

    RS2::FormatType getFormatType() const;

    void setFormatType(RS2::FormatType formatType);

    /**
    * @return File name of the document currently loaded.
    * Note, that the default file name is empty.
    */
    QString getFilename() const {return filename;}

    /**
     * @return Auto-save file name of the document currently loaded.
     */
    QString getAutoSaveFileName() const {return autosaveFilename;}

    /**
     * Sets file name for the document currently loaded.
     */
    void setFilename(QString fn) {filename = std::move(fn);}

    const QString &getAutosaveFilename() const;

    void setAutosaveFileName(const QString &autosaveFilename);

    void setModificationListener(LC_GraphicModificationListener * listener) {m_modificationListener = listener;}

    LC_DimStyle* getFallBackDimStyleFromVars() const;
    LC_DimStyle* getDimStyleByName(const QString &name, RS2::EntityType dimType = RS2::EntityUnknown) const;
    QString getDefaultDimStyleName() const;
    void setDefaultDimStyleName(QString name);
    LC_DimStyle* getEffectiveDimStyle(const QString &styleName, RS2::EntityType dimType, LC_DimStyle* styleOverride) const;
    virtual LC_DimStyle* getResolvedDimStyle(const QString &dimStyleName, RS2::EntityType dimType = RS2::EntityUnknown) const;
    void updateFallbackDimStyle(LC_DimStyle* get_copy);
    void replaceDimStylesList(const QString& defaultStyleName, const QList<LC_DimStyle*>& styles);
protected:
    void fireUndoStateChanged(bool undoAvailable, bool redoAvailable) const override;
private:
    QDateTime lastSaveTime;
    QString currentFileName; //keep a copy of filename for the modifiedTime

    // fixme - sand - files - change to unique_ptrs?
    RS_LayerList layerList{};
    RS_BlockList blockList{true};
    RS_VariableDict m_variableDict;
    RS_VariableDict m_customVariablesDict;
    LC_ViewList namedViewsList;
    LC_UCSList ucsList;
    LC_DimStylesList dimstyleList;
    LC_TextStyleList textStyleList;

    //if set to true, will refuse to modify paper scale
    bool paperScaleFixed = false;

    /** Format type */
    RS2::FormatType formatType = RS2::FormatUnknown;

    // Paper margins in millimeters
    double marginLeft = 0.;
    double marginTop = 0.;
    double marginRight = 0.;
    double marginBottom = 0.;

    // Number of pages drawing occupies
    int pagesNumH = 1;
    int pagesNumV = 1;

    /** File name of the document or empty for a new document. */
    QString filename;
    /** Auto-save file name of document. */
    QString autosaveFilename;

    LC_GraphicModificationListener* m_modificationListener = nullptr;
};
#endif
