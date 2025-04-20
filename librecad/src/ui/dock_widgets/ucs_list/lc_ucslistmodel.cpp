/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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

#include <QFont>
#include "lc_ucslistmodel.h"

#include "lc_ucslist.h"
#include "lc_ucslistoptions.h"
#include "rs_units.h"
#include "rs_math.h"

LC_UCSListModel::LC_UCSListModel(LC_UCSListOptions *modelOptions, QObject *parent):QAbstractTableModel(parent), m_options(modelOptions) {
    m_iconWCS = QIcon(":/icons/ucs_wcs.lci");
    m_iconUCS = QIcon(":/icons/ucs_ucs.lci");
    m_iconGridOrtho = QIcon(":/icons/grid_ortho.lci");
    m_iconGridISOTop = QIcon(":/icons/grid_iso_top.lci");
    m_iconGridISOLeft = QIcon(":/icons/grid_iso_left.lci");
    m_iconGridISORight = QIcon(":/icons/grid_iso_right.lci");
}

LC_UCSListModel::~LC_UCSListModel() {
    m_ucss.clear();
}

void LC_UCSListModel::setUCSList(LC_UCSList *ucsList, RS2::LinearFormat format, RS2::AngleFormat af,
                                 int precision,
                                 int anglePrecision, RS2::Unit drawingUnit) {
    m_linearFormat = format;
    m_angleFormat = af;
    m_prec = precision;
    m_anglePrec = anglePrecision;
    m_unit = drawingUnit;
    this->m_ucsList = ucsList;
    beginResetModel();

    m_ucss.clear();
    if (ucsList != nullptr) {
        for (unsigned i=0; i < ucsList->count(); ++i) {
            LC_UCS *ucs = ucsList->at(i);
            UCSItem* item = createUCSItem(ucs);
            m_ucss.append(item);
        }
    }

    endResetModel();
}

QModelIndex LC_UCSListModel::parent([[maybe_unused]]const QModelIndex &child) const {
    return QModelIndex();
}

int LC_UCSListModel::rowCount([[maybe_unused]]const QModelIndex &parent) const {
    return m_ucss.size();
}

int LC_UCSListModel::columnCount([[maybe_unused]]const QModelIndex &parent) const {
    int result = LAST;
    if (!m_options->showColumnTypeIcon) {
        result--;
    }
    if (!m_options->showColumnGridType){
        result--;
    }
    if (!m_options->showColumnPositionAndAngle){
        result--;
    }
    return result;
}

int LC_UCSListModel::translateColumn(int column) const{
    int result = column;
    if (!m_options->showColumnTypeIcon){
        if (result >= COLUMNS::ICON_TYPE){
            result++;
        }
    }
    if (!m_options->showColumnGridType){
        if (result >= COLUMNS::ICON_ORTHO_TYPE){
            result++;
        }
    }
    if (!m_options->showColumnPositionAndAngle){
        if (result >= COLUMNS::UCS_DETAILS){
            result++;
        }
    }
    return result;
}

QVariant LC_UCSListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_ucss.size())
        return QVariant();

    UCSItem* ucs {m_ucss.at(index.row())};
    int col = translateColumn(index.column());
    switch (role) {
        case Qt::DecorationRole: {
            switch (col) {
                case ICON_TYPE:
                    return ucs->iconType;
                case ICON_ORTHO_TYPE:
                    return ucs->iconGridType;
                default:
                    break;
            }
            break;
        }
        case Qt::UserRole: {
            return ucs->ucs->getName();
        }
        case Qt::DisplayRole: {
            switch (col){
                case (NAME):{
                    QString displayName = ucs->displayName;
                    return displayName;
                }
                case UCS_DETAILS:{
                   return ucs->ucsInfo;
                }
                default:
                    break;
            }
            break;
        }
        case Qt::BackgroundRole: {
            break;
        }
        case Qt::FontRole:{
            if (NAME == col) {
                QFont font;
                bool customFont = false;
                LC_UCS* activeUCS = m_ucsList->getActive();
                LC_UCS* currentUCS = ucs->ucs;
                if (activeUCS != nullptr && activeUCS == currentUCS) {
                    font.setBold(true);
                    customFont = true;
                }
                if (currentUCS->isTemporary()){
                    font.setItalic(true);
                    customFont = true;
                }
                if (customFont){
                    return font;
                }
            }

            break;
        }
        case Qt::ForegroundRole:{
            break;
        }
        case Qt::ToolTipRole:{
            if (m_options->showViewInfoToolTip){
                return ucs->toolTip;
            }
            break;
        }
        default:
            // do nothing;
            break;
    }
    return QVariant();
}

Qt::ItemFlags LC_UCSListModel::flags(const QModelIndex &index) const {
    return QAbstractTableModel::flags(index);
}

LC_UCS *LC_UCSListModel::getItemForIndex(const QModelIndex &index) const{
    LC_UCS* result = nullptr;
    if (index.isValid()){
        int row = index.row();
        if (row < m_ucss.size()){
            result = m_ucss.at(row)->ucs;
        }
    }
    return result;
}

void LC_UCSListModel::fillUCSsList(QList<LC_UCS *> &list) const {
    for (auto v: m_ucss){
        list << v->ucs;
    }
    list.removeAt(0); // remove WCS
}

QIcon LC_UCSListModel::getTypeIcon(LC_UCS *ucs) const {
    if (ucs->isUCS()){
        return m_iconUCS;
    } else {
        return m_iconWCS;
    }
}

QIcon LC_UCSListModel::getOrthoTypeIcon(LC_UCS *ucs) const {
    if (ucs->isUCS()){
        int orthoType = ucs->getOrthoType();
        switch (orthoType){
            case LC_UCS::NON_ORTHO:{
                return m_iconGridOrtho;
            }
            case LC_UCS::TOP:
            case LC_UCS::BOTTOM:{
                return m_iconGridISOTop;
            }
            case LC_UCS::LEFT:{
                return m_iconGridISOLeft;
            }
            case LC_UCS::RIGHT:{
                return m_iconGridISORight;
            }
            case LC_UCS::FRONT:
            case LC_UCS::BACK:{
                // todo - replace icon as soon as support of these iso projections will be added
                return m_iconGridISOTop;
            }
            default:
                return m_iconGridOrtho;
        }
    } else {
        return m_iconGridOrtho;
    }
}

QString LC_UCSListModel::getGridViewType(int orthoType){
    switch (orthoType) {
        case LC_UCS::FRONT:
        case LC_UCS::BACK:
            return tr("Ortho");
        case LC_UCS::LEFT: {
            return tr("Left");
        }
        case LC_UCS::RIGHT: {
            return tr("Right");
        }
        case LC_UCS::TOP:
        case LC_UCS::BOTTOM: {
            return tr("Top");
        }
        default:
            return tr("Ortho");
    }
}

QModelIndex LC_UCSListModel::getIndexForUCS(LC_UCS *ucs) const {
    if (ucs != nullptr){
        for (unsigned int i = 0; i < m_ucsList->count(); i++){
            auto v = m_ucsList->at(i);
            if (v == ucs){
                return createIndex(i, 0, ucs);
            }
        }
    }
    return QModelIndex();
}

void LC_UCSListModel::markActive(LC_UCS *ucs) {
    m_ucsList->tryToSetActive(ucs);
    QModelIndex topLeft = createIndex(0,0);
    QModelIndex bottomRight = createIndex( m_ucsList->count(), columnCount(topLeft));
    emit dataChanged(topLeft, bottomRight);
}

LC_UCS *LC_UCSListModel::getWCS() {
    return m_ucsList->getWCS();
}

LC_UCS* LC_UCSListModel::getActiveUCS(){
    LC_UCS* result = nullptr;
    if (m_ucsList != nullptr) {
        result = m_ucsList->getActive();
    }
    return result;
}
int LC_UCSListModel::count(){
    return m_ucss.count();
}

QString LC_UCSListModel::getUCSInfo(LC_UCS *ucs) {
    QString originX = RS_Units::formatLinear(ucs->getOrigin().x, m_unit, m_linearFormat, m_prec);
    QString originY = RS_Units::formatLinear(ucs->getOrigin().y, m_unit, m_linearFormat, m_prec);
    QString angle = RS_Units::formatAngle(ucs->getXAxis().angle(), m_angleFormat, m_anglePrec);
    QString origin = originX.append(", "). append(originY).append(" < ").append(angle);
    return origin;
}

LC_UCSListModel::UCSItem *LC_UCSListModel::createUCSItem(LC_UCS *ucs) {
    auto* result = new UCSItem();
    result->ucs  = ucs;

    result->iconType = getTypeIcon(ucs);
    result->iconGridType = getOrthoTypeIcon(ucs);

    QString name = ucs->getName();
    if (name.isEmpty()){
        name = tr("<No name>");
    }

    result->displayName = name;

    double angleValue = RS_Math::correctAnglePlusMinusPi(ucs->getXAxis().angle());
    QString originX = RS_Units::formatLinear(ucs->getOrigin().x, m_unit, m_linearFormat, m_prec);
    QString originY = RS_Units::formatLinear(ucs->getOrigin().y, m_unit, m_linearFormat, m_prec);
    QString angle = RS_Units::formatAngle(angleValue, m_angleFormat, m_anglePrec);

    QString ucsInfo;
    ucsInfo.append(originX).append(" , "). append(originY).append(" < ").append(angle);

    result->ucsInfo = ucsInfo;

    QString orthoType = getGridViewType(ucs->getOrthoType());

    QString toolTip = QString(tr("Name: ")).append("<b>").append(name).append("</b><br>")
        .append(tr("Origin X: ")).append("<b>").append(originX).append("</b><br>")
        .append(tr("Origin Y: ")).append("<b>").append(originY).append("</b><br>")
        .append(tr("Angle: ")).append("<b>").append(angle).append("</b><br>")
        .append(tr("Grid: ")).append("<b>").append(orthoType).append("</b><br>")
        .append(tr("UCS: ")).append("<b>").append(ucs->isUCS() ? tr("Yes") : tr("No")).append("</b>");

    result->toolTip = toolTip;

    return result;
}
