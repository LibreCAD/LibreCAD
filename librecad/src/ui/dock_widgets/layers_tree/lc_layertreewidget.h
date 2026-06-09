/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
**
** This file is free software; you can redistribute it and/or modify
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

#ifndef LC_LAYERTREEWIDGET_H
#define LC_LAYERTREEWIDGET_H

#include "lc_graphicviewawarewidget.h"
#include "rs_document.h"
#include "rs_layerlistlistener.h"

class RS_Graphic;
class QCheckBox;
class QToolButton;
class QG_ActionHandler;
class LC_LayerTreeView;
class LC_LayerTreeModel;
class LC_LayerTreeItem;
class QLineEdit;
class RS_LayerList;

/**
 * This is the Qt implementation of a widget which can view layers in tree mode
 * and provides interface for various layer-related operations
 */
class LC_LayerTreeWidget : public LC_GraphicViewAwareWidget, public RS_LayerListListener {
    Q_OBJECT
public:
    enum DropIndicatorPosition {
        OnItem, AboveItem, BelowItem, OnViewport, InvalidDrop
    };

    LC_LayerTreeWidget(const QG_ActionHandler* ah, QWidget* parent, const char* name = nullptr, Qt::WindowFlags f = {});
    ~LC_LayerTreeWidget() override = default;
    void activateLayer(RS_Layer* layer) const;
    void layerActivated(RS_Layer* layer) override;
    void layerAdded(RS_Layer* layer) override;
    void layerEdited(RS_Layer*) override;
    void layerRemoved(RS_Layer*) override;
    void layerToggled(RS_Layer*) override;
    void layerToggledLock(RS_Layer*) override;
    void layerToggledPrint(RS_Layer*) override;
    void layerToggledConstruction(RS_Layer*) override;
    void onDragEnterEvent(const QModelIndex& dropIndex) const;
    void onDropEvent(const QModelIndex& dropIndex, DropIndicatorPosition position) const;
    void setGraphicView(RS_GraphicView* gview) override;
    void removeActiveLayer(bool removeWithChildren);
signals :
    void escape();
public slots :
    void slotTreeClicked(const QModelIndex& layerIdx);
    void slotTreeDoubleClicked(const QModelIndex& index);
    void slotFilteringMaskChanged() const;
    void expandAllLayers() const;
    void collapseAllLayers() const;
    void collapseSecondaryLayers() const;
    void onCustomContextMenu(const QPoint& point);
    void addChildLayerForSelectedItem();
    void editSelectedLayer();
    void addInformationalLayerForSelectedItem();
    void addAddAlternativePositionLayerForSelectedItem();
    void showAllLayers();
    void hideAllLayers();
    void hideOtherThanSelectedLayers();
    void unlockAllLayers();
    void lockAllLayers();
    void printAllLayers();
    void noPrintAllLayers();
    void addLayer();
    void toggleFlatView();
    void removeEmptyLayers();
protected:
    void setLayerList(RS_LayerList* ll);
    void update() const;
    void keyPressEvent(QKeyEvent* e) override;
    void expandItems(int depth) const;
    QModelIndex getSelectedItemIndex() const;
    QList<RS_Layer*> collectLayersForSelectedItem() const;
    void addDimensionalLayerForSelectedItem();
    void addDimensionalLayerForActiveLayer();
    void selectLayersEntities();
    void convertLayerTypeToDimensional();
    void convertLayerTypeToInformational();
    void convertLayerTypeToAlternativePosition();
    void convertLayerTypeToNormal();
    void createLayerCopy();
    void createLayerDuplicate();
    void renameVirtualLayer();
    void removeLayersForSelectedItem();
    void removeActiveLayers();
    void removeChildLayersForSelected();
    void toggleSecondaryLayersVisibility() const;
    void moveSelectionToLayer();
    void duplicateSelectionToLayer();
    void exportSelectedLayer();
    void exportLayersList(QList<RS_Layer*> layersToExport) const;
    void exportLayerSubTree();
    void exportVisibleLayers();
    QLayout* getTopLevelLayout() const override;
private:
    RS_LayerList* m_layerList{nullptr};
    QLineEdit* m_matchLayerName{nullptr};
    QCheckBox* m_matchModeCheckBox{nullptr};
    LC_LayerTreeView* m_layerTreeView{nullptr};
    LC_LayerTreeModel* m_layerTreeModel{nullptr};
    RS_GraphicView* m_graphicView{nullptr};
    RS_Document* m_document{nullptr}; // fixme - do we need it at all??
    RS_Graphic* m_graphic{nullptr};
    const QG_ActionHandler* m_actionHandler{nullptr};
    bool m_flatListMode{false};
    QToolButton* m_btnCollapseSecondary{nullptr};
    QToolButton* m_btnCollapseAll{nullptr};
    QToolButton* m_btnExpandAll{nullptr};
    QToolButton* m_btnListMode{nullptr};
    QToolButton* m_btnShowSecondaryLayers{nullptr};
    QToolButton* m_btnAddDimensional{nullptr};

    /*
    * layer activation is invoked from the widget, by click on item.
    * this flag is used to prevent model rebuild in order to
    * receive subsequent double-click (if any). Otherwise, it will not be emitted
    * for activated item...
    */
    bool m_withinSelfActivation{false};
    QLayout* initButtonsBar();
    QLayout* initFilterAndSettingsSection();
    LC_LayerTreeView* initTreeView();

    void updateToolBarButtons() const;
    void doConvertSelectedItemLayerToNewType(int newType) const;
    void deselectEntitiesOnLockedLayer(RS_Layer* layer) const;
    void deselectEntities(RS_Layer* layer) const;
    void manageLayersVisibilityFlag(const QList<RS_Layer*>& layersToEnable, const QList<RS_Layer*>& layersToDisable, bool toggleMode) const;
    void manageLayersConstructionFlag(const QList<RS_Layer*>& layersToBeConstruction, const QList<RS_Layer*>& layersNonConstruction,
                                      bool toggleMode);
    void manageLayersLockFlag(const QList<RS_Layer*>& layersToLockOrToggle, const QList<RS_Layer*>& layersToUnlock, bool toggleMode);
    void manageLayersPrintFlag(const QList<RS_Layer*>& layersToPrint, const QList<RS_Layer*>& layersNoPrint, bool toggleMode);
    void doSelectLayersEntities(QList<RS_Layer*>& layers) const;
    void copyLayerAttributes(RS_Layer* copyLayer, const RS_Layer* sourceLayer);
    void redrawView() const;
    void doCreateLayersCopy(const QModelIndex& sourceIndex, bool duplicateEntities);
    void duplicateLayerEntities(const RS_Layer* sourceLayer, RS_Layer* copyLayer, LC_DocumentModificationBatch& ctx) const;
    void doMoveSelectionToLayer(const LC_LayerTreeItem* layerItem, bool duplicate, bool resolvePens = false) const;
    void doRemoveLayersFromSource(LC_LayerTreeItem* source, bool removeChildrenOnly);
    void doRemoveLayers(QList<RS_Layer*>& layers) const;
    void editActiveLayer();
    void showActiveLayerOnly() const;
    int invokeLayersRemovalDialog(const QStringList& layerNames);
    void invokeLayerEditOrRenameDialog(LC_LayerTreeItem* pItem, bool edit);
    void invokeLayerAddDialog(LC_LayerTreeItem* parentItem, int layerType);
    void invokeSettingsDialog();
    void doAddSecondaryLevelForSelectedItem(int layerType);
    void initTreeModel();
    void moveOrDuplicateSelectionToSelectedItemLayer(bool duplicate, const QString& title);
    void doRemoveLayerItems(QList<LC_LayerTreeItem*>& itemsToRemove);
};

#endif
