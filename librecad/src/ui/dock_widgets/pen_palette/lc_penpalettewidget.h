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
#ifndef LC_PENPALETTEWIDGET_H
#define LC_PENPALETTEWIDGET_H

#include "lc_graphicviewawarewidget.h"
#include "rs_pen.h"
#include "ui_lc_penpalettewidget.h"

class LC_PenPaletteData;
class LC_PenPaletteModel;
class LC_PenItem;
class RS_LayerList;
/**
 * Central widget for Pens Palette
 */
class LC_PenPaletteWidget :public LC_GraphicViewAwareWidget, public Ui::LC_PenPaletteWidget{
    Q_OBJECT
public:
    explicit LC_PenPaletteWidget(const QString& title, QWidget* parent);
    ~LC_PenPaletteWidget() override = default ;
    void setGraphicView(RS_GraphicView *gview) override;
    void persist();
signals:
    void escape();
public slots:
    void onTableClicked(QModelIndex modelIndex);
    void onTableSelectionChanged(const QItemSelection &selected,const QItemSelection &deselected);
    void onPenEditorChanged();
    void keyPressEvent(QKeyEvent* e) override;
    void fillPenEditorBySelectedEntityAttributesPen();
    void fillPenEditorBySelectedEntityDrawingPen();
    void fillPenEditorByPenToolBarPen();
    void invokeOptionsDialog(bool focusOnFile=false);
    void createOrUpdatePenItem();
    void applyEditorPenToSelection();
    void selectEntitiesWithAttributesPenByPenEditor();
    void selectEntitiesWithDrawingPenByPenEditor();
    void applyEditorPenToPenToolBar();
    void removeActivePenItem();

    void applySelectedPenToSelection();
    void selectEntitiesWithAttributesPenBySelectedPenItem();
    void selectEntitiesWithDrawingPenBySelectedPenItem();
    void applySelectedPenItemToPenToolBar();
    void removeSelectedPenItem();
    void removeSelectedPenItems();
    void editSelectedPenItem();
    void filterMaskChanged();

    void onTableViewContextMenuInvoked(const QPoint &pos);
    void onPenEditorColorChanged(int index);
    void onPenEditorWidthChanged(int index);
    void onPenEditorLineTypeChanged(int index);
    void applySelectedPenItemToActiveLayer();
    void fillPenEditorByActiveLayer();
    void applyEditorPenToActiveLayer();
    void onModelChanged();
    void doDoubleClick();
    void updatePenToolbarByActiveLayer();
    void updateWidgetSettings();

protected:
    // mouse click counter used for handling both single click and double-click on table view
    int m_clicksCount {0};
    RS_GraphicView* m_graphicView{nullptr};
    LC_PenPaletteModel* m_penPaletteModel{nullptr};
    LC_PenPaletteData* m_penPaletteData{nullptr};
    RS_LayerList* m_layerList{nullptr};
    bool m_inEditorControlsSetup = false;
    bool m_editorChanged = false;
    void initTableView();
    void initFilteringSection();
    void fillPenEditorByPenItem(LC_PenItem *pen);
    void markEditingPenChanged(bool changed);
    void updateModel();
    void initPenEditor();
    void doUpdatePenEditorByPenAttributes(const RS_Color &color, RS2::LineWidth &width, RS2::LineType &lineType);
    void doFillPenEditorByPen(RS_Pen pen);
    RS_Pen createPenByEditor(const RS_Pen &originalPen);
    int invokeItemRemovalDialog(QString &penName);
    void doSelectEntitiesThatMatchToPenAttributes(
        const RS2::LineType &lineType, const RS2::LineWidth &width, const RS_Color &color, bool colorCheck, bool resolvePens, bool resolveLayers) const;
    void redrawDrawing() const;
    void doApplyPenAttributesToSelection(RS2::LineType lineType, RS2::LineWidth width, RS_Color color, bool modifyColor);
    void showEntitySelectionInfoDialog();
    QModelIndex getSelectedItemIndex();
    LC_PenItem *getSelectedPenItem();
    RS_Pen createPenByPenItem(RS_Pen &pen, LC_PenItem *pItem);
    void doRemovePenItem(LC_PenItem *penItem);
    void doRemovePenItems(QList<LC_PenItem *> &penItems);
    int invokeItemMultiRemovalDialog(QList<LC_PenItem *> &penItems);
    QList<LC_PenItem *> getSelectedPenItems();
    void doFillPenEditorBySelectedEntity(bool resolvePenOnEntitySelect);
    void doSelectEntitiesByPenEditor(bool resolvePens, bool resolveLayers);
    void doSelectEntitiesBySelectedPenItem(bool resolvePens, bool resolveLayers);
    void showNoSelectionDialog(bool hasOnFrozenLayers, bool hasOnLockedLayers) const;
    void initToolBar() const;
    void onTableRowDoubleClicked();
    void onPersistentItemsChanged();
    bool invokeUnableToSavePenDataDialog();
    void setLayerList(RS_LayerList *ll);
};
#endif // LC_PENPALETTEWIDGET_H
