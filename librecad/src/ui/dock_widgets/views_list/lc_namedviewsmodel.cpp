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

#include "lc_namedviewsmodel.h"
#include "rs_units.h"


LC_NamedViewsModel::LC_NamedViewsModel(LC_NamedViewsListOptions *modelOptions, QObject *parent):QAbstractTableModel(parent), options(modelOptions) {
//    viewPaperSpace = QIcon(":/icons/print.svg");
    iconViewPaperSpace = QIcon(":/icons/page_portait.svg");
    iconViewDrawingSpace = QIcon(":/icons/center_to_page.svg");
}

LC_NamedViewsModel::~LC_NamedViewsModel() {
    views.clear();
}

void LC_NamedViewsModel::setViewsList(LC_ViewList *viewsList,RS2::LinearFormat format, int precision, RS2::Unit drawingUnit) {
    linearFormat = format;
    prec = precision;
    unit = drawingUnit;
    this->viewsList = viewsList;
    beginResetModel();

    views.clear();
    if (viewsList != nullptr) {
        for (unsigned i=0; i < viewsList->count(); ++i) {
            views.append(viewsList->at(i));
        }
    }

    endResetModel();
}

QModelIndex LC_NamedViewsModel::parent([[maybe_unused]]const QModelIndex &child) const {
    return QModelIndex();
}

int LC_NamedViewsModel::rowCount([[maybe_unused]]const QModelIndex &parent) const {
    return views.size();
}

int LC_NamedViewsModel::columnCount([[maybe_unused]]const QModelIndex &parent) const {
    if (options->showTypeIcon) {
        return 2;
    }
    else{
        return 1;
    }
}

int LC_NamedViewsModel::translateColumn(int column) const{
    int result = column;
    if (!options->showTypeIcon){
        result = column + 1;
    }
    return result;
}

QVariant LC_NamedViewsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= views.size())
        return QVariant();

    LC_View* view {views.at(index.row())};
    int col = translateColumn(index.column());

    switch (role) {
        case Qt::DecorationRole: {
            switch (col) {
                case ICON: // show view type icon
                    return getTypeIcon(view);
                default:
                    break;
            }
            break;
        }
        case Qt::UserRole: {
            return view->getName();
        }
        case Qt::DisplayRole: {
            if (NAME == col) { // display name of item
                QString displayName = view->getName();
                return displayName;
            }
            break;
        }
        case Qt::BackgroundRole: {
            break;
        }
        case Qt::FontRole: {
            break;
        }
        case Qt::ForegroundRole:{
            break;
        }
        case Qt::ToolTipRole:{
            if (options->showViewInfoToolTip){
                QString centerX = RS_Units::formatLinear(view->getCenter().x,unit,linearFormat, prec);
                QString centerY = RS_Units::formatLinear(view->getCenter().y,unit,linearFormat, prec);
                QString sizeX = RS_Units::formatLinear(view->getSize().x,unit,linearFormat, prec);
                QString sizeY = RS_Units::formatLinear(view->getSize().y,unit,linearFormat, prec);

                QString toolTip = QString(tr("Name: ")).append("<b>").append(view->getName()).append("</b><br>")
                    .append(tr("Center: ")).append("<b>").append(centerX).append(",").append(centerY).append("</b><br>")
                    .append(tr("Size: ")).append("<b>").append(sizeX).append(",").append(sizeY).append("</b><br>")
                    .append(tr("In Paperspace: ")).append("<b>").append(view->isForPaperView()?tr("Yes"):tr("No")).append("</b>");
                return toolTip;
            }
            break;
        }
        default:
            // do nothing;
            break;
    }
    return QVariant();
}

Qt::ItemFlags LC_NamedViewsModel::flags(const QModelIndex &index) const {
    return QAbstractTableModel::flags(index);
}

LC_View *LC_NamedViewsModel::getItemForIndex(const QModelIndex &index) const{
    LC_View* result = nullptr;
    if (index.isValid()){
        int row = index.row();
        if (row < views.size()){
            result = views.at(row);
        }
    }
    return result;
}

void LC_NamedViewsModel::fillViewsList(QList<LC_View *> &list) const {
   for (auto v: views){
       list << v;
   }
}

QIcon LC_NamedViewsModel::getTypeIcon(LC_View *view) const {
    if (view->isForPaperView()){
        return iconViewPaperSpace;
    } else {
        return iconViewDrawingSpace;
    }
}

QModelIndex LC_NamedViewsModel::getIndexForView(LC_View *view) const {
    if (view != nullptr){
        for (unsigned int i = 0; i < viewsList->count(); i++){
            LC_View* v = viewsList->at(i);
            if (v == view){
                return createIndex(i, 0, view);
            }
        }
    }
    return QModelIndex();
}
