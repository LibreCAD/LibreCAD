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

#include <QString>
#include "ui_lc_layerdialog_ex.h"
#include "rs_layer.h"
#include "lc_layertreemodel.h"

class LC_LayerDialogEx :public QDialog, public Ui::LC_LayerDialogEx
{
    Q_OBJECT

public:
    enum{
        MODE_RENAME_VIRTUAL,
        MODE_EDIT_LAYER,
        MODE_ADD_LAYER,
        MODE_ADD_CHILD_LAYER,
        MODE_ADD_SECONDARY_LAYER
    };
    LC_LayerDialogEx(QWidget* parent, QString name, LC_LayerTreeModel* model, LC_LayerTreeItem *editedTreeItem, RS_LayerList* layerList);
    ~LC_LayerDialogEx() = default;

    void setMode(int mode);
    void setLayerName(QString name);
    void setParentPath(QString name);
    void setLayerType(int type);
    int getEditedLayerType();
    void disableNames();
    void setLayer(RS_Layer *layer);
    void allowChangingLayerType(bool value);
    RS_Pen getPen();
    bool isConstruction() {return cbConstructionLayer->isChecked();};
    void setConstruction(bool enable) {cbConstructionLayer->setChecked(enable);};
    QString getLayerName();
public slots:
    virtual void validate();
    void layerTypeChanged();

protected slots:
    virtual void languageChange();

private:
    void init();

    int mode;    
    LC_LayerTreeModel* layerTreeModel;
    LC_LayerTreeItem *editedTreeItem {nullptr};
    RS_LayerList* layerList;
    bool checkForDuplicatedNames(const QStringList &newLayerNamesList);
};

#endif // LC_LAYERDIALOG_EX_H
