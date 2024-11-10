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

#ifndef LC_NAMEDVIEWSMODEL_H
#define LC_NAMEDVIEWSMODEL_H

#include <QAbstractTableModel>
#include <QIcon>
#include "lc_viewslist.h"
#include "lc_namedviewslistoptions.h"
#include "rs.h"

class LC_NamedViewsModel:public QAbstractTableModel {
    Q_OBJECT
public:

    explicit LC_NamedViewsModel(LC_NamedViewsListOptions *modelOptions, QObject * parent = nullptr);

    ~LC_NamedViewsModel() override;
    void setViewsList(LC_ViewList *viewsList,RS2::LinearFormat format, int precision, RS2::Unit drawingUnit);

    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    LC_View *getItemForIndex(const QModelIndex &index) const;
    int translateColumn(int column) const;
    void fillViewsList(QList<LC_View *> &list) const;
    QIcon getTypeIcon(LC_View *view) const;
    QModelIndex getIndexForView(LC_View* view) const;
    int count(){
        return views.count();
    }
    /**
   * Columns that are shown in the table
   */
    enum COLUMNS{
        ICON,
        NAME,
        INFO,
        LAST
    };

protected:
    RS2::LinearFormat linearFormat;
    int prec;
    RS2::Unit unit;
    LC_ViewList* viewsList {nullptr};
    QList<LC_View*> views;
    QIcon iconViewPaperSpace;
    QIcon iconViewDrawingSpace;
    LC_NamedViewsListOptions* options {nullptr};
};

#endif // LC_NAMEDVIEWSMODEL_H
