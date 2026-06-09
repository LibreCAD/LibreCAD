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

#include "lc_namedviewslistoptions.h"
#include "lc_ucslist.h"
#include "lc_viewslist.h"

LC_NamedViewsModel::LC_NamedViewsModel(LC_NamedViewsListOptions* modelOptions, QObject* parent) : QAbstractTableModel(parent),
    m_options(modelOptions) {
    m_iconViewPaperSpace = QIcon(":/icons/page_portait.lci");
    m_iconViewDrawingSpace = QIcon(":/icons/center_to_page.lci");

    m_iconWCS = QIcon(":/icons/ucs_wcs.lci");
    m_iconUCS = QIcon(":/icons/ucs_ucs.lci");
    m_iconGridOrtho = QIcon(":/icons/grid_ortho.lci");
    m_iconGridISOTop = QIcon(":/icons/grid_iso_top.lci");
    m_iconGridISOLeft = QIcon(":/icons/grid_iso_left.lci");
    m_iconGridISORight = QIcon(":/icons/grid_iso_right.lci");
}

LC_NamedViewsModel::~LC_NamedViewsModel() {
    qDeleteAll(m_views);
    m_views.clear();
}

void LC_NamedViewsModel::setViewsList(LC_ViewList* viewsList, LC_Formatter* formatter) {
    m_formatter = formatter;
    this->m_viewsList = viewsList;
    beginResetModel();

    m_views.clear();
    if (viewsList != nullptr) {
        for (unsigned i = 0; i < viewsList->count(); ++i) {
            LC_View* view = viewsList->at(i);
            ViewItem* item = createViewItem(view);
            m_views.append(item);
        }
    }

    endResetModel();
}

QModelIndex LC_NamedViewsModel::parent([[maybe_unused]] const QModelIndex& child) const {
    return QModelIndex();
}

int LC_NamedViewsModel::rowCount([[maybe_unused]] const QModelIndex& parent) const {
    return m_views.size();
}

int LC_NamedViewsModel::columnCount([[maybe_unused]] const QModelIndex& parent) const {
    int result = LAST;
    if (!m_options->showColumnIconType) {
        result--;
    }
    if (!m_options->showColumnGridType) {
        result--;
    }
    if (!m_options->showColumnUCSDetails) {
        result--;
    }
    if (!m_options->showColumnUCSType) {
        result--;
    }
    return result;
}

int LC_NamedViewsModel::translateColumn(const int column) const {
    int result = column;
    if (!m_options->showColumnIconType) {
        if (result >= COLUMNS::ICON_TYPE) {
            result++;
        }
    }
    if (!m_options->showColumnGridType) {
        if (result >= COLUMNS::ICON_GRID_TYPE) {
            result++;
        }
    }
    if (!m_options->showColumnUCSType) {
        if (result >= COLUMNS::ICON_UCS_TYPE) {
            result++;
        }
    }
    if (!m_options->showColumnViewDetails) {
        if (result >= COLUMNS::VIEW_INFO) {
            result++;
        }
    }
    if (!m_options->showColumnUCSDetails) {
        if (result >= COLUMNS::UCS_INFO) {
            result++;
        }
    }
    return result;
}

QString LC_NamedViewsModel::getUCSInfo(const LC_UCS* ucs) const {
    QString originX = m_formatter->formatLinear(ucs->getOrigin().x);
    const QString originY = m_formatter->formatLinear(ucs->getOrigin().y);
    const QString angle = m_formatter->formatRawAngle(ucs->getXAxis().angle());
    QString origin = originX.append(", ").append(originY).append(" < ").append(angle);
    return origin;
}

QVariant LC_NamedViewsModel::data(const QModelIndex& index, const int role) const {
    if (!index.isValid() || index.row() >= m_views.size()) {
        return QVariant();
    }

    ViewItem* view{m_views.at(index.row())};
    const int col = translateColumn(index.column());

    switch (role) {
        case Qt::DecorationRole: {
            switch (col) {
                case ICON_TYPE:
                    return view->typeIcon;
                case ICON_GRID_TYPE:
                    return view->gridTypeIcon;
                case ICON_UCS_TYPE: // show view type icon
                    return view->ucsTypeIcon;
                default:
                    break;
            }
            break;
        }
        case Qt::UserRole: {
            return view->view->getName();
        }
        case Qt::DisplayRole: {
            switch (col) {
                case NAME: {
                    QString displayName = view->displayName;
                    return displayName;
                }
                case VIEW_INFO: {
                    return view->viewInfo;
                }
                case UCS_INFO: {
                    return view->ucsInfo;
                }
                default:
                    break;
            }
            break;
        }
        case Qt::BackgroundRole: {
            break;
        }
        case Qt::FontRole: {
            break;
        }
        case Qt::ForegroundRole: {
            break;
        }
        case Qt::ToolTipRole: {
            if (m_options->showViewInfoToolTip) {
                return view->tooltip;
            }
            break;
        }
        default:
            // do nothing;
            break;
    }
    return QVariant();
}

Qt::ItemFlags LC_NamedViewsModel::flags(const QModelIndex& index) const {
    return QAbstractTableModel::flags(index);
}

LC_View* LC_NamedViewsModel::getItemForIndex(const QModelIndex& index) const {
    LC_View* result = nullptr;
    if (index.isValid()) {
        const int row = index.row();
        if (row < m_views.size()) {
            result = m_views.at(row)->view;
        }
    }
    return result;
}

void LC_NamedViewsModel::fillViewsList(QList<LC_View*>& list) const {
    for (const auto v : m_views) {
        list << v->view;
    }
}

QIcon LC_NamedViewsModel::getTypeIcon(const LC_View* view) const {
    if (view->isForPaperView()) {
        return m_iconViewPaperSpace;
    }
    return m_iconViewDrawingSpace;
}

QIcon LC_NamedViewsModel::getUCSTypeIcon(const LC_View* view) const {
    if (view->isHasUCS()) {
        const LC_UCS* ucs = view->getUCS();
        if (LC_WCS::instance.isSameTo(ucs)) {
            return m_iconWCS;
        }
        return m_iconUCS;
    }
    return m_iconWCS;
}

QIcon LC_NamedViewsModel::getGridTypeIcon(const LC_View* view) const {
    if (view->isHasUCS()) {
        const LC_UCS* ucs = view->getUCS();
        if (LC_WCS::instance.isSameTo(ucs)) {
            return m_iconGridOrtho;
        }
        const int orthoType = ucs->getOrthoType();
        switch (orthoType) {
            case LC_UCS::NON_ORTHO: {
                return m_iconGridOrtho;
            }
            case LC_UCS::TOP:
            case LC_UCS::BOTTOM: {
                return m_iconGridISOTop;
            }
            case LC_UCS::LEFT: {
                return m_iconGridISOLeft;
            }
            case LC_UCS::RIGHT: {
                return m_iconGridISORight;
            }
            case LC_UCS::FRONT:
            case LC_UCS::BACK: {
                // todo - replace icon as soon as support of these iso projections will be added
                return m_iconGridISOTop;
            }
            default:
                return m_iconGridOrtho;
        }
    }
    return m_iconGridOrtho;
}

QModelIndex LC_NamedViewsModel::getIndexForView(const LC_View* view) const {
    if (view != nullptr) {
        for (unsigned int i = 0; i < m_viewsList->count(); i++) {
            const LC_View* v = m_viewsList->at(i);
            if (v == view) {
                return createIndex(i, 0, view);
            }
        }
    }
    return QModelIndex();
}

void LC_NamedViewsModel::updateViewsUCSNames(LC_UCSList* ucsList) {
    for (unsigned int i = 0; i < m_views.count(); i++) {
        ViewItem* v = m_views.at(i);
        LC_View* view = v->view;
        LC_UCS* viewUCS = view->getUCS();
        if (viewUCS != nullptr) {
            const LC_UCS* existingListUCS = ucsList->findExisting(viewUCS);
            if (existingListUCS == nullptr) {
                viewUCS->setName(tr("<No name>"));
            }
            else {
                QString ucsName = existingListUCS->getName();
                viewUCS->setName(ucsName);
            }
        }
        setupViewItem(view, v);
    }
}

void LC_NamedViewsModel::clear() {
    m_views.clear();
}

QString LC_NamedViewsModel::getGridViewType(const int orthoType) {
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

LC_NamedViewsModel::ViewItem* LC_NamedViewsModel::createViewItem(LC_View* view) {
    const auto result = new ViewItem();
    setupViewItem(view, result);
    return result;
}

void LC_NamedViewsModel::setupViewItem(LC_View* view, ViewItem* result) {
    result->view = view;
    result->typeIcon = getTypeIcon(view);
    result->ucsTypeIcon = getUCSTypeIcon(view);
    result->gridTypeIcon = getGridTypeIcon(view);

    QString name = view->getName();
    if (name.isEmpty()) {
        name = tr("<No name>");
    }

    result->displayName = name;

    const QString centerX = m_formatter->formatLinear(view->getCenter().x);
    const QString centerY = m_formatter->formatLinear(view->getCenter().y);
    const QString sizeX = m_formatter->formatLinear(view->getSize().x);
    const QString sizeY = m_formatter->formatLinear(view->getSize().y);

    QString viewInfo;
    viewInfo.append(centerX).append(", ").append(centerY).append(" | ").append(sizeX).append(" x ").append(sizeY);

    result->viewInfo = viewInfo;

    const LC_UCS* ucs = view->getUCS();
    QString ucsInfo;
    if (ucs == nullptr) {
        ucsInfo = "WCS";
    }
    else {
        QString ucsName = ucs->getName();
        const QString ucsDetails = getUCSInfo(ucs);
        if (ucsName.isEmpty()) {
            ucsInfo = tr("<No name>").append(" | ").append(ucsDetails);
        }
        else {
            ucsInfo = ucsName.append(" | ").append(ucsDetails);
        }
    }

    result->ucsInfo = ucsInfo;

    QString ucsType;

    int orthoType = LC_UCS::UCSOrthoType::NON_ORTHO;
    bool currentIsWCS = true;
    if (ucs == nullptr) {
        ucsType = tr("WCS");
    }
    else {
        if (LC_WCS::instance.isSameTo(ucs)) {
            ucsType = tr("WCS");
        }
        else {
            currentIsWCS = false;
            ucsType = tr("UCS");
            orthoType = ucs->getOrthoType();
        }
    }

    QString toolTip = QString(tr("Name: ")).append("<b>").append(name).append("</b><br>").append(tr("Center X: ")).append("<b>").
                                            append(centerX).append("</b><br>").append(tr("Center Y: ")).append("<b>").append(centerY).
                                            append("</b><br>").append(tr("Width: ")).append("<b>").append(sizeX).append("</b><br>").
                                            append(tr("Height: ")).append("<b>").append(sizeY).append("</b><br>");

    if (currentIsWCS) {
        toolTip.append(tr("UCS: ")).append("<b>").append(ucsType).append("</b><br>");
    }
    else {
        const QString ucsOriginX = m_formatter->formatLinear(ucs->getOrigin().x);
        const QString ucsOriginY = m_formatter->formatLinear(ucs->getOrigin().y);
        const QString ucsAngle = m_formatter->formatRawAngle(ucs->getXAxis().angle());

        const QString viewGridType = getGridViewType(orthoType);

        toolTip.append(tr("UCS: ")).append("<b>").append(ucsType).append("</b><br>").append(tr("UCS Name: ")).append("<b>").append(name).
                append("</b><br>").append(tr("Grid View: ")).append("<b>").append(viewGridType).append("</b><br>").
                append(tr("UCS Origin X: ")).append("<b>").append(ucsOriginX).append("</b><br>").append(tr("UCS Origin Y: ")).append("<b>").
                append(ucsOriginY).append("</b><br>").append(tr("UCS Angle: ")).append("<b>").append(ucsAngle).append("</b><br>");
    }

    toolTip.append(tr("In Paperspace: ")).append("<b>").append(view->isForPaperView() ? tr("Yes") : tr("No")).append("</b>");
    result->tooltip = toolTip;
}
