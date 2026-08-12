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

#include <array>
#include <cstdint>
#include <memory>
#include <QDateTime>
#include <memory>

#include "lc_dimstyle.h"
#include "lc_dimstyleslist.h"
#include "lc_plot_settings.h"
#include "lc_textstylelist.h"
#include "lc_dwgadvancedmetadata.h"
#include "lc_ucslist.h"
#include "lc_viewslist.h"
#include "rs_blocklist.h"
#include "rs_document.h"
#include "rs_layerlist.h"
#include "rs_variabledict.h"

class RS_Dimension;
class LC_DimStyleToVariablesMapper;
class LC_DimStylesList;
class QString;

class LC_View;
class QG_LayerWidget;

/**
 * Thin alias over the DWG layout record persisted by libdxfrw's reader.
 *
 * Exposing the round-trip-grade record directly keeps the LibreCAD-side
 * API zero-cost (no field-copy boilerplate) while routing all mutations
 * through dedicated RS_Graphic setters that bump the modified flag. The
 * UI layer (PR 10 tab bar, PR 11 plot dialog) consumes this type via
 * RS_Graphic::layouts() / RS_Graphic::findLayout().
 */
using LC_Layout = LC_DwgAdvancedMetadata::LayoutRecord;


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
    explicit RS_Graphic(RS_EntityContainer* parent=nullptr);
    ~RS_Graphic() override;

    virtual void onLoadingCompleted();
    /** @return RS2::EntityGraphic */
    RS2::EntityType rtti() const override {return RS2::EntityGraphic;}

    virtual unsigned countLayerEntities(RS_Layer* layer) const;

    RS_LayerList* getLayerList() override {return &m_layerList;}
    RS_BlockList* getBlockList() override {return &m_blockList;}
    LC_ViewList* getViewList() override {return &m_namedViewsList;}
    LC_UCSList* getUCSList() override {return &m_ucsList;}
    LC_DimStylesList* getDimStyleList() override {return &m_dimstyleList;}
    LC_TextStyleList* getTextStyleList() override {return &m_textStyleList;}
    void addDimStyle(LC_DimStyle* style) {m_dimstyleList.addDimStyle(style);}
    void initForNewDocument() override;
    LC_DwgAdvancedMetadata& dwgAdvancedMetadata() {return m_dwgAdvancedMetadata;}
    const LC_DwgAdvancedMetadata& dwgAdvancedMetadata() const {return m_dwgAdvancedMetadata;}

    // ---- Paper-space layouts (PR 9 — DWG OBJECTS surface) -----------
    /**
     * Read-only view of the paper-space layouts loaded with the drawing.
     * Backed by LC_DwgAdvancedMetadata::layouts(); populated by
     * RS_FilterDXFRW::addLayout during DWG import.  UI code (PR 10 tab
     * bar) should call this then route mutations through the dedicated
     * setLayoutMargins / setActiveLayoutHandle setters below — those
     * bump the modified flag, whereas reaching directly into
     * dwgAdvancedMetadata() does not.
     */
    const std::vector<LC_Layout>& layouts() const {
        return m_dwgAdvancedMetadata.layouts();
    }
    /** @return pointer to the layout record matching @p handle, or nullptr. */
    const LC_Layout* findLayout(std::uint32_t handle) const;

    /**
     * Active layout handle (paper-space tab the UI currently shows).
     * Defaults to 0 — meaning the modelspace / first paper-space layout
     * is implicitly active, matching legacy behavior.  PR 10 wires this
     * into the new tab bar; PR 11 wires it into the plot dialog.
     */
    std::uint32_t activeLayoutHandle() const { return m_activeLayoutHandle; }
    void setActiveLayoutHandle(std::uint32_t handle);

    /** Per-layout paper margin setter — mutates the matching LayoutRecord
     *  in dwgAdvancedMetadata() and bumps the modified flag.  Returns
     *  false if @p handle has no matching layout. */
    bool setLayoutMargins(std::uint32_t handle,
                          double left, double top,
                          double right, double bottom);

    /**
     * Active layout's paper margins in millimeters (left/top/right/bottom).
     *
     * When activeLayoutHandle() resolves to a stored LayoutRecord, returns
     * that record's PlotSettings margins.  Otherwise (DXF / no layouts
     * loaded / unmatched handle) falls back to the legacy document-level
     * getMarginLeft/Top/Right/Bottom — preserving the existing plot dialog
     * behavior for DXF documents.
     */
    std::array<double, 4> activeLayoutMargins() const;

    /**
     * Active layout's paper margin setter — writes to the matching
     * LayoutRecord when activeLayoutHandle() resolves, falls back to
     * setMargins() otherwise.  Negative components are skipped to mirror
     * the legacy setMargins() / setLayoutMargins() convention.
     */
    void setActiveLayoutMargins(double left, double top,
                                double right, double bottom);

    // Wrappers for Layer functions:
    void clearLayers() {
        m_layerList.clear();
        validateSelection();
    }
    unsigned countLayers() const {return m_layerList.count();}
    RS_Layer* layerAt(const unsigned i) const {return m_layerList.at(i);}
    void activateLayer(const QString& name, const bool notify = false) {m_layerList.activate(name, notify);}
    void activateLayer(RS_Layer* layer, const bool notify = false) {m_layerList.activate(layer, notify);}
    RS_Layer* getActiveLayer() const {return m_layerList.getActive();}
    virtual void addLayer(RS_Layer* layer) {m_layerList.add(layer);}
    void addEntity(const RS_Entity* entity) override;
    void removeLayer(RS_Layer* layer);
    void editLayer(RS_Layer* layer, const RS_Layer& source) {m_layerList.edit(layer, source);}
    RS_Layer* findLayer(const QString& name) {return m_layerList.find(name);}
    void toggleLayer(const QString& name);
    void toggleLayer(RS_Layer* layer);
    void toggleLayerLock(RS_Layer* layer) {m_layerList.toggleLock(layer); validateSelection();}
    void toggleLayerPrint(RS_Layer* layer) {m_layerList.togglePrint(layer);}
    void toggleLayerConstruction(RS_Layer* layer) {m_layerList.toggleConstruction(layer);}
    void freezeAllLayers(bool freeze);
    void lockAllLayers(const bool lock) {m_layerList.lockAll(lock);validateSelection();}
    void toggleLockLayers(const QList<RS_Layer*>& layers){m_layerList.toggleLockMulti(layers);validateSelection();}
    void togglePrintLayers(const QList<RS_Layer*>& layers){m_layerList.togglePrintMulti(layers);validateSelection();}
    void toggleConstructionLayers(const QList<RS_Layer*>& layers){m_layerList.toggleConstructionMulti(layers);validateSelection();}
    void toggleFreezeLayers(const QList<RS_Layer*>& layers);
    void setFreezeLayers(const QList<RS_Layer*>& layersEnable, const QList<RS_Layer*>& layersDisable);
    void setLockLayers(const QList<RS_Layer*>& layersToUnlock, const QList<RS_Layer*>& layersToLock){m_layerList.setLockMulti(layersToUnlock, layersToLock);validateSelection();}
    void setPrintLayers(const QList<RS_Layer*>& layersNoPrint, const QList<RS_Layer*>& layersPrint){m_layerList.setPrintMulti(layersNoPrint, layersPrint);validateSelection();}
    void setConstructionLayers(const QList<RS_Layer*>& layersNoConstruction, const QList<RS_Layer*>& layersConstruction){m_layerList.setConstructionMulti(layersNoConstruction, layersConstruction);validateSelection();}

    void addLayerListListener(RS_LayerListListener* listener) {m_layerList.addListener(listener);}
    void removeLayerListListener(RS_LayerListListener* listener) {m_layerList.removeListener(listener);}
    void addViewListListener(LC_ViewListListener* listener) { m_namedViewsList.addListener(listener);}
    void removeViewListListener(LC_ViewListListener* listener) { m_namedViewsList.removeListener(listener);}

    // Wrapper for block functions:
    void clearBlocks() {m_blockList.clear();}
    unsigned countBlocks() const {return m_blockList.count();}
    RS_Block* blockAt(const unsigned i) {return m_blockList.at(i);}
    void activateBlock(const QString& name) {m_blockList.activate(name);}
    void activateBlock(RS_Block* block) {m_blockList.activate(block);}
    RS_Block* getActiveBlock() const {return m_blockList.getActive();}
    bool addBlock(RS_Block* block, const bool notify=true) {return m_blockList.add(block, notify);}
    void addBlockNotification() {m_blockList.addNotification();}
    void removeBlock(RS_Block* block) {m_blockList.remove(block);}
    RS_Block* findBlock(const QString& name) {return m_blockList.find(name);}
    QString newBlockName(const QString& suggestion = {}) {return m_blockList.newName(suggestion);}
    void toggleBlock(const QString& name);
    void toggleBlock(RS_Block* block);
    void toggleBlocks(const QList<RS_Block*>& blocks);
    void freezeAllBlocks(bool freeze);
    void addBlockListListener(RS_BlockListListener* listener) {m_blockList.addListener(listener);}
    void removeBlockListListener(RS_BlockListListener* listener) {m_blockList.removeListener(listener);}

        // Wrappers for variable functions:
    void clearVariables();
    QString getCustomProperty(const QString& key, const QString& defaultValue) const;
    void addCustomProperty(const QString& key, const QString& value);
    void removeCustomProperty(const QString& key);
    bool hasCustomProperty(const QString& key) const;
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

    void setVariableDictObject(const RS_VariableDict& inputVariableDict) {m_variableDict = inputVariableDict;}

    RS_VariableDict getVariableDictObject() const{
        return m_variableDict;
    }

    RS_VariableDict* getVariableDictObjectRef() {
        return &m_variableDict;
    }

    RS2::LinearFormat getLinearFormat() const;
    void setLinearFormat(RS2::LinearFormat linearFormat);
    void replaceCustomVars(const QHash<QString, QString>& vars);
    virtual void prepareForSave();

    static RS2::LinearFormat convertLinearFormatDXF2LC(int f);
    int getLinearPrecision() const;
    void setLinearPrecision(int value);
    RS2::AngleFormat getAngleFormat() const;
    void setAngleFormat(RS2::AngleFormat angleFormat);
    int getAnglePrecision() const;
    void addAnglePrecision(int value);

    RS_Vector getPaperInsertionBase() const;
    void setPaperInsertionBase(const RS_Vector& p);

    void setUnit(RS2::Unit u);
    RS2::Unit getUnit() const;

    bool isGridOn() const;
    void setGridOn(bool on);
    bool isIsometricGrid() const;
    void setIsometricGrid(bool on);
    void setCurrentUCS(const LC_UCS* ucs);
    LC_UCS* getCurrentUCS() const;
    RS2::IsoGridViewType getIsoView() const;
    void setIsoView(RS2::IsoGridViewType viewType);
    void centerToPage();
    bool fitToPage();

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

    /**
     * Import-time geometry repair (prepare / far re-base) mutates coordinates.
     * Set during fileImport when a repair switch is on and a mutation ran;
     * load path should mark the document modified after markSaved so Save
     * does not silently rewrite the user's file without notice.
     */
    void setImportGeometryMutated(bool mutated) { m_importGeometryMutated = mutated; }
    bool takeImportGeometryMutated() {
        const bool v = m_importGeometryMutated;
        m_importGeometryMutated = false;
        return v;
    }

    QDateTime getLastSaveTime(){return m_lastSaveTime;}
    void setLastSaveTime(const QDateTime &time) { m_lastSaveTime = time;}

    LC_PlotSettings* getPlotSettings() const;

    friend std::ostream& operator << (std::ostream& os, RS_Graphic& g);
    int clean();
    LC_View *findNamedView(const QString& viewName) const {return m_namedViewsList.find(viewName);}
    LC_UCS *findNamedUCS(const QString& ucsName) const {return m_ucsList.find(ucsName);}
    void addNamedView(LC_View *view) {m_namedViewsList.add(view);}
    void addUCS(LC_UCS *ucs) {m_ucsList.add(ucs);}

    double getAnglesBase() const;
    void setAnglesBase(double baseAngle);
    bool areAnglesCounterClockWise() const;
    void setAnglesCounterClockwise(bool on);
    RS_Vector getUserGridSpacing() const;
    void setUserGridSpacing(const RS_Vector& spacing);
    QString formatAngle(double angle) const;
    QString formatLinear(double linear) const;

    RS2::FormatType getFormatType() const;

    void setFormatType(RS2::FormatType formatType);

    /**
    * @return File name of the document currently loaded.
    * Note, that the default file name is empty.
    */
    QString getFilename() const {return m_filename;}

    /**
     * @return Auto-save file name of the document currently loaded.
     */
    QString getAutoSaveFileName() const {return m_autosaveFilename;}

    /**
     * Sets file name for the document currently loaded.
     */
    void setFilename(QString fn) {m_filename = std::move(fn);}

    const QString &getAutosaveFilename() const;

    void setAutosaveFileName(const QString &fileName);



    LC_DimStyle* getFallBackDimStyleFromVars() const;
    LC_DimStyle* getDimStyleByName(const QString &name, RS2::EntityType dimType = RS2::EntityUnknown) const;
    QString getDefaultDimStyleName() const;
    void setDefaultDimStyleName(const QString& name);
    LC_DimStyle* getEffectiveDimStyle(const QString &styleName, RS2::EntityType dimType, const LC_DimStyle* styleOverride) const;
    LC_DimStyle* getEffectiveDimStyleForEdit(const QString& styleName, RS2::EntityType dimType, const LC_DimStyle* styleOverride) const;
    virtual LC_DimStyle* getResolvedDimStyle(const QString &dimStyleName, RS2::EntityType dimType = RS2::EntityUnknown) const;
    void updateFallbackDimStyle(const LC_DimStyle* fromStyle);
    void replaceDimStylesList(const QString& defaultStyleName, const QList<LC_DimStyle*>& styles);
    void validateSelection() const {m_selectedSet->cleanup();}
protected:
    void fireGraphicModified(bool modified) const;
private:
    /** Rebuild INSERT visibility and refresh cached drawing state after layer changes. */
    void refreshLayerVisibility();
    /** Rebuild derived INSERT children after block visibility changes. */
    void refreshBlockVisibility();

    QDateTime m_lastSaveTime;
    QString m_currentFileName; //keep a copy of filename for the modifiedTime

    // fixme - sand - files - change to unique_ptrs?
    RS_LayerList m_layerList;
    RS_BlockList m_blockList{true};
    RS_VariableDict m_variableDict;
    RS_VariableDict m_customVariablesDict;
    LC_ViewList m_namedViewsList;
    LC_UCSList m_ucsList;
    LC_DimStylesList m_dimstyleList;
    LC_TextStyleList m_textStyleList;

    LC_DwgAdvancedMetadata m_dwgAdvancedMetadata;

    /** Active paper-space layout handle.  0 == "no explicit selection"
     *  (legacy behavior — UI defaults to modelspace / first layout). */
    std::uint32_t m_activeLayoutHandle = 0;

    //if set to true, will refuse to modify paper scale
    bool paperScaleFixed = false;

    /** Format type */
    RS2::FormatType m_formatType = RS2::FormatUnknown;



    /** File name of the document or empty for a new document. */
    QString m_filename;
    /** Auto-save file name of document. */
    QString m_autosaveFilename;

    bool m_anglesCounterClockWize;

    /** True when import repairs rewrote block defs and/or model placement. */
    bool m_importGeometryMutated = false;

    std::unique_ptr<LC_PlotSettings> m_plotSettings;
};
#endif
