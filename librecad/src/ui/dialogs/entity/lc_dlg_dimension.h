/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_DLGDIMENSION_H
#define LC_DLGDIMENSION_H

#include "lc_dimstyleitem.h"
#include "lc_dimstylestreemodel.h"
#include "lc_entitypropertiesdlg.h"

namespace RS2{
    enum EntityType : unsigned;
}

class RS_Graphic;
class LC_DimStylePreviewGraphicView;
class RS_Dimension;

namespace Ui{
    class LC_DlgDimension;
}

class LC_DlgDimension : public LC_EntityPropertiesDlg {
    Q_OBJECT
public:
    explicit LC_DlgDimension(QWidget* parent, LC_GraphicViewport* viewport, RS_Dimension* dim);
    ~LC_DlgDimension() override;
    void updateEntity() override;
protected:
    void selectStyleItem(const QModelIndex& indexToSelect) const;
    void updateActiveStyleInfoLabel() const;
    void updateEntityStyleInfoLabels(const LC_DimStyleItem* item) const;
    void setEntity(RS_Dimension* dim);
    void expandStylesTree() const;
    QModelIndex setupStylesList();
    void getOverrideItemIndex(const LC_DimStyleTreeModel* model, const LC_DimStyleItem* entityStyleItem, QModelIndex& itemIndex);
    void updateActionButtons(const LC_DimStyleItem* item) const;
    void updateDimStylePreview(LC_DimStyle* dimStyle, LC_DimStyleTreeModel* model, bool override, const QString& baseName) const;
    void updateDimStylePreview(const RS_Pen& pen) const;
    void updateDimStylePreview(bool flipArrow1, bool flipArrow2) const;
    LC_DimStyleTreeModel* getDimStylesModel() const;
    bool isDimensionTypeMatchedToStyleType(RS2::EntityType styleType, RS2::EntityType dimensionType);
    RS2::EntityType adjustDimentityTypeForStyleName(RS2::EntityType entityType);
    void prepareDimStyleItems(QList<LC_DimStyleItem*>& items);
    void setupPreview();
    void showEvent(QShowEvent* event) override;
    void saveDimensionTypeDependentProperties() const;
    void clearDimStylesModel(LC_DimStyleTreeModel* model);
    void saveDimensionStyles();
    void setupDimensionTypeDependentUI(RS_Dimension* dim);
    void setupDimensionAttributesUI(const RS_Dimension* dim);
    QModelIndex getSelectedDimStyleIndex() const;
protected slots:
    void languageChange();
    void onDimStyleDoubleClick();
    void onDimStyleOverrideEdit(bool checked);
    void onDimStyleOverrideSave(bool val);
    void onDimStyleOverrideRemove(bool val) ;
    void onDimStyleOverrideNew(bool val);
    void onDimStylesListMenuRequested(const QPoint& pos);
    void onDimCurrentChanged(const QModelIndex& current, const QModelIndex& previous) const;
    void onDimStyleExport(bool val);
    void onDimStyleImport(bool val);
    void onDimStyleSet(bool val) ;
    void onDimStyleSetDefault(bool val) ;
    void onDimStyleEntitySelect(bool val);
    void onPenChanged() const;
    void onFlipArrowChanged(bool val) const;
private:
    Ui::LC_DlgDimension* ui;
    RS_Dimension* m_entity = nullptr;
    LC_DimStylePreviewGraphicView* m_previewView = nullptr;
    RS_Graphic* m_graphic = nullptr;

    bool m_structuredChanged = false;
};

#endif
