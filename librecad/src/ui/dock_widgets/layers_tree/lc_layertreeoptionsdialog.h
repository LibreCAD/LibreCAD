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

#ifndef LC_LAYERTREEOPTIONSDIALOG_H
#define LC_LAYERTREEOPTIONSDIALOG_H

#include "lc_dialog.h"
#include <ui_lc_layertreeoptionsdialog.h>

struct LC_LayerTreeModelOptions;
class QComboBox;

class LC_LayerTreeOptionsDialog :public LC_Dialog, public Ui::LC_LayerTreeOptionsDialog{
    Q_OBJECT
public:
    explicit LC_LayerTreeOptionsDialog(QWidget *parent, LC_LayerTreeModelOptions *options);
    ~LC_LayerTreeOptionsDialog();
public slots:
   void pb_highlightedColorClicked();
   void pb_gridColorClicked();
   void pb_selectedItemColorClicked();
   void pbSelectedItemsBgColorClicked();
   void pbActiveLayerBgColorClicked();
   void showIndentedClicked() const;
   void validate();
protected slots:
    void languageChange();
private:    
    LC_LayerTreeModelOptions* m_options;
    void init();
    void initComboBox(QComboBox* cb, const QColor color);
    void set_color(QComboBox* combo, QColor custom);
    void showInvalidColorMessage(const QString& name);
};
#endif // LC_LAYERTREEOPTIONSDIALOG_H
