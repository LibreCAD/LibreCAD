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

#ifndef LC_LAYERDIALOG_EX_H
#define LC_LAYERDIALOG_EX_H

#include "lc_dialog.h"
#include "rs_pen.h"
#include "lc_layertreeitem.h"
#include "ui_lc_layerdialog_ex.h"

class RS_LayerList;
class RS_Layer;
class LC_LayerTreeModel;

class LC_LayerDialogEx :public LC_Dialog, public Ui::LC_LayerDialogEx{
    Q_OBJECT
public:
    enum{
        MODE_RENAME_VIRTUAL,
        MODE_EDIT_LAYER,
        MODE_ADD_LAYER,
        MODE_ADD_CHILD_LAYER,
        MODE_ADD_SECONDARY_LAYER
    };
    LC_LayerDialogEx(QWidget* parent, const QString& name, LC_LayerTreeModel* model, LC_LayerTreeItem *treeItem, RS_LayerList* layerList);
    ~LC_LayerDialogEx() override = default;

    void setMode(int viewMode);
    void setLayerName(const QString& name) const;
    void setParentPath(const QString& name) const;
    void setLayerType(int type) const;
    int getEditedLayerType() const;
    void disableNames() const;
    void setLayer(const RS_Layer *layer) const;
    void allowChangingLayerType(bool value);
    RS_Pen getPen() const;
    bool isConstruction() const {return cbConstructionLayer->isChecked();}
    void setConstruction(const bool enable) const {cbConstructionLayer->setChecked(enable);}
    QString getLayerName() const;
public slots:
    void validate();
    void layerTypeChanged() const;

protected slots:
    void languageChange();

private:
    void init();
    int m_mode;
    LC_LayerTreeModel* m_layerTreeModel;
    LC_LayerTreeItem *m_editedTreeItem {nullptr};
    RS_Layer *m_editedLayer {nullptr};
    QString m_originalLayerName;
    LC_LayerTreeItem::LayerType m_originalLayerType {LC_LayerTreeItem::NOT_DEFINED_LAYER_TYPE};
    RS_LayerList* m_layerList;
    bool checkForDuplicatedNames(const QStringList &newLayerNamesList) const;
};

#endif
