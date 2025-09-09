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

#ifndef LC_DLGNEWDIMSTYLE_H
#define LC_DLGNEWDIMSTYLE_H

#include "lc_dialog.h"
#include "rs.h"

class LC_StylesListModel;
class QListModel;
class LC_DimStyleItem;

namespace Ui{
    class LC_DlgNewDimStyle;
}

class LC_DlgNewDimStyle : public LC_Dialog{
    Q_OBJECT

public:
    explicit LC_DlgNewDimStyle(QWidget *parent = nullptr);
    ~LC_DlgNewDimStyle() override;
public slots:
    void onUsedForChanged(int index);
    void onBasedOnChanged(int index);
    void onStyleNameTextChanged(const QString &);
    void setup(LC_DimStyleItem* initialStyle, QList<LC_DimStyleItem*>& items);
    QString getStyleName() const;
    void onAccept();
    RS2::EntityType getDimensionType(){return dimType;}
    LC_DimStyleItem* getBaseDimStyle(){return baseDimStyle;}
private:
    Ui::LC_DlgNewDimStyle *ui;
    bool nameWasEntered = false;
    LC_StylesListModel* m_dimItemsListModel{nullptr};
    RS2::EntityType dimType = RS2::EntityUnknown;
    LC_DimStyleItem* baseDimStyle{nullptr};
};

#endif // LC_DLGNEWDIMSTYLE_H
