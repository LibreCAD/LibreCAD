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
#include <QFile>
#include <QFont>
#include <QRegularExpression>
#include <QStyledItemDelegate>
#include <QTextStream>

#include "lc_penpalettemodel.h"
#include "lc_peninforegistry.h"
#include "lc_penitem.h"
#include "lc_penpalettedata.h"
#include "lc_penpaletteoptions.h"

static int COLOR_ICON_SIZE = 24;

LC_PenPaletteModel::LC_PenPaletteModel(LC_PenPaletteOptions* modelOptions, LC_PenPaletteData* data, QObject* parent) :
    QAbstractTableModel(parent),
    m_filteringRegexp{std::make_unique<QRegularExpression>()},
    m_registry{LC_PenInfoRegistry::instance()} {
    m_options = modelOptions;
    m_penPaletteData = data;
    update(true);
}

LC_PenPaletteModel::~LC_PenPaletteModel() = default;

int LC_PenPaletteModel::rowCount ([[maybe_unused]] const QModelIndex & parent) const {
    return m_displayItems.size();
}

QModelIndex LC_PenPaletteModel::parent ( const QModelIndex & /*index*/ ) const {
    return QModelIndex();
}

QModelIndex LC_PenPaletteModel::index ( int row, int column, const QModelIndex & /*parent*/ ) const {
    if (row >= m_displayItems.size() || row < 0)
        return QModelIndex();
    return createIndex ( row, column);
}
/**
 * Method used to update model and recreate the list of items that will be displayed.
 * It relies on persistent list of pens stored in data and includes only matched items against regexp (if any) if
 * filtering mode of of the list is set.
 * @param updateNames flag that indicates whether it is necessary to resolve pen's attributes to their display names
 */
void LC_PenPaletteModel::update(bool updateNames){
    beginResetModel();
    // remove all from items list
    m_displayItems.clear();

    // simply iterate by pens data
    int count = m_penPaletteData->getItemsCount();
    for (int i=0; i < count; i++){
        LC_PenItem* item = m_penPaletteData->getItemAt(i);
        QString itemName = item->getName();

        if (m_hasRegexp){
            // check whether filtering is case-sensitive
            if (m_options->ignoreCaseOnMatch){
                itemName = itemName.toLower();
            }
            QRegularExpressionMatch match = m_filteringRegexp->match(itemName);
            bool hasRegexpMatch = match.hasMatch();
            item->setMatched(hasRegexpMatch);

            if (m_options->filterIsInHighlightMode){
                // just add item into the list to display
                m_displayItems << item;
            } else if (hasRegexpMatch){
                // add it item as highlighted
                m_displayItems << item;
            } else {
                continue; // skip this item at all as it was not matched, so it will not be shown in the table
            }
        }
        else{ // just add all items to display
            m_displayItems << item;
            item->setMatched(false);
        }

        // names should be updated on initial build and after options change
        if (updateNames){
            setupItemForDisplay(item);
        }
    }

    // sort collected items alphabetically
    std::sort(m_displayItems.begin(), m_displayItems.end(), [](const LC_PenItem *s1, const LC_PenItem *s2)-> bool{
        return s1->getName() < s2->getName();
    } );

    // notify that model was updated
    endResetModel();
    emitModelChange();
}

/**
 * Sets regexp string that should be used for items filtering or highlighting. This is setter only,
 * does not updates the model.
 * @param regexp
 */
void LC_PenPaletteModel::setFilteringRegexp(QString &regexp){
    QString pattern =/* QRegularExpression::wildcardToRegularExpression(*/regexp/*)*/;
    m_filteringRegexp->setPattern(pattern);
    QRegularExpression::PatternOptions option = m_options->ignoreCaseOnMatch
                                                    ? QRegularExpression::CaseInsensitiveOption
                                                    : QRegularExpression::NoPatternOption;
    // ensure that we'll use case-insensitive match
    m_filteringRegexp->setPatternOptions(option);
    m_hasRegexp = !regexp.trimmed().isEmpty();
}

/**
 * Utility method to notify that model is changed
 */
void LC_PenPaletteModel::emitModelChange(){
    emit modelChange();
}


/**
 * Return pen for given row
 * @param row
 * @return
 */
LC_PenItem *LC_PenPaletteModel::getPen(int row) const {
    if (row >= m_displayItems.size() || row < 0)
        return nullptr;
    return m_displayItems.at(row);
}

/**
 * All items are selectable and enabled
 * @param index
 * @return
 */
Qt::ItemFlags LC_PenPaletteModel::flags([[maybe_unused]] const QModelIndex &index) const{
    return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
}

/**
 * Model mega-method...
 * @param index
 * @param role
 * @return
 */
QVariant LC_PenPaletteModel::data(const QModelIndex &index, int role) const{
    if (!index.isValid() || index.row() >= m_displayItems.size())
        return QVariant();

    LC_PenItem *item{m_displayItems.at(index.row())};

    // take care of columns visibility
    int col = translateColumn(index.column());

    switch (role) {
        case Qt::DecorationRole:
            // return proper items where needed
            switch (col) {
                case COLUMNS::TYPE_ICON:
                    return item->getLineTypeIcon();
                case COLUMNS::WIDTH_ICON:
                    return item->getLineWidthIcon();
                case COLUMNS::COLOR_ICON:
                    return item->getColorIcon();
                default:
                    break;            }
            break;
        case Qt::DisplayRole:
            // names for attributes and pen
            switch (col) {
                case COLUMNS::NAME:
                    return item->getName();
                case COLUMNS::TYPE_NAME:
                    return item->getLineTypeName();
                case COLUMNS::WIDTH_NAME:
                    return item->getLineWidthName();
                case COLUMNS::COLOR_NAME: {
                    QString colorName = item->getColorName();
                    return colorName;
                }
                default:
                    break;
            }
            break;
        case Qt::BackgroundRole: {
            // highlight pen that was active (in pen editor) by appropriate background
            if (m_activePen && m_activePen == item){
                return m_options->activeItemBGColor;
            }
            break;
        }
        case Qt::ForegroundRole: {
            if (item->isMatched()){
                // highlighting of items that are matched by filter regexp
                if (m_options->filterIsInHighlightMode){
                    return m_options->matchedItemColor;
                }
            }
            break;
        }
        case Qt::ToolTipRole: {
            // optional tooltip displaying
            if (m_options->showToolTip){
                QString toolTip = QString(tr("Name:")).append("<b>").append(item->getName()).append("</b><br>")
                    .append(tr("Line Type:")).append("<b>").append(item->getLineTypeName()).append("</b><br>")
                    .append(tr("Line Width:")).append("<b>").append(item->getLineWidthName()).append("</b><br>");
                RS_Color color = item->getColor();
                if (color.isValid()){
                    toolTip = toolTip.append(tr("Color:")).append("<b><font color='").append(color.name()).append("'>").append(item->getColorName()).append("</font></b>");
                } else {
                    toolTip = toolTip.append(tr("Color:")).append("<b>").append(item->getColorName()).append("</b>");
                }
                return toolTip;
            }
            break;
        }
        case Qt::FontRole: {
            // fonts - bold for active and italic for special attribute values
            QFont font;
            bool fontChanged = false;
            if (m_activePen && m_activePen == item){
                if (m_options->showEntireRowBold){
                    font.setBold(true);
                    fontChanged = true;
                } else {
                    if (col == NAME){
                        font.setBold(true);
                        fontChanged = true;
                    }
                }
            }
            switch (col) {
                case COLUMNS::TYPE_NAME: {
                    RS2::LineType lineType = item->getLineType();
                    if (lineType == RS2::LineType::LineByLayer){
                        font.setItalic(true);
                        fontChanged = true;
                    } else if (lineType == RS2::LineType::LineByBlock){
                        font.setItalic(true);
                        fontChanged = true;
                    } else if (lineType == RS2::LineType::LineTypeUnchanged){
                        font.setStrikeOut(true);
                        fontChanged = true;
                    }
                    break;
                }
                case COLUMNS::WIDTH_NAME: {
                    RS2::LineWidth lineWidth = item->getLineWidth();
                    if (lineWidth == RS2::LineWidth::WidthByLayer){
                        font.setItalic(true);
                        fontChanged = true;
                    } else if (lineWidth == RS2::LineWidth::WidthByBlock){
                        font.setItalic(true);
                        fontChanged = true;
                    } else if (lineWidth == RS2::LineWidth::WidthUnchanged){
                        font.setStrikeOut(true);
                        fontChanged = true;
                    }
                    break;
                }
                case COLUMNS::COLOR_NAME: {
                    RS_Color color = item->getColor();
                    if (LC_PenInfoRegistry::isUnchangedColor(color)){
                        font.setStrikeOut(true);
                        fontChanged = true;
                    }
                    break;
                }
                default:
                    break;
            }
            if (fontChanged){
                return font;
            }
        }
        default:
            break;
    }

    return QVariant();
}

/**
 * Translates given real column (from model index) to virtual column index taking care of layers visibility
 * @param column
 * @return
 */
int LC_PenPaletteModel::translateColumn(int column) const{
    int result = column;    
    if (!m_options->showColorIcon){
       if (result >= COLUMNS::COLOR_ICON){
         result++;
       }
    }
    if (!m_options->showColorName){
        if (result >= COLUMNS::COLOR_NAME){
            result++;
        }
    }
    if (!m_options->showWidthIcon){
        if (result >= COLUMNS::WIDTH_ICON){
            result++;
        }
    }
    if (!m_options->showWidthName){
        if (result >= COLUMNS::WIDTH_NAME){
            result++;
        }
    }

    if (!m_options->showTypeIcon){
        if (result >= COLUMNS::TYPE_ICON){
            result++;
        }
    }
    if (!m_options->showTypeName){
        if (result >= COLUMNS::TYPE_NAME){
            result++;
        }
    }
    return result;
}
/**
 * Returns amount of colors that should be present in the table according to columns visibility settings
 * @return
 */
int LC_PenPaletteModel::columnCount(const QModelIndex &) const{
    int result = COLUMNS::LAST;
    if (!m_options->showColorIcon){
        result --;
    }
    if (!m_options->showColorName){
        result --;
    }
    if (!m_options->showTypeIcon){
        result --;
    }
    if (!m_options->showTypeName){
        result --;
    }
    if (!m_options->showWidthIcon){
        result --;
    }
    if (!m_options->showWidthName){
        result --;
    }
    return result;
}

/**
 * Method performs resolving of internal attributes to their display values (icon and text)
 * @param newPen
 */
void LC_PenPaletteModel::setupItemForDisplay(LC_PenItem *newPen){

    // handling line type
    RS2::LineType lineType = newPen->getLineType();
    QIcon iconLineType = m_registry->getLineTypeIcon(lineType);
    QString lineTypeName = m_registry->getLineTypeText(lineType);

    newPen->setLineTypeIcon(iconLineType);
    newPen->setLineTypeName(lineTypeName);

    // handling line width
    RS2::LineWidth lineWidth = newPen->getLineWidth();

    QIcon iconLineWidth = m_registry->getLineWidthIcon(lineWidth);
    newPen ->setLineWidthIcon(iconLineWidth);
    QString lineWidthName = m_registry->getLineWidthText(lineWidth);
    newPen -> setLineWidthName(lineWidthName);

    // handling color
    RS_Color color = newPen->getColor();
    QIcon iconColor = m_registry->getColorIcon(color, COLOR_ICON_SIZE, COLOR_ICON_SIZE);
    newPen->setColorIcon(iconColor);

    QString colorName = m_registry->getColorName(color, m_options->colorNameDisplayMode);
    newPen->setColorName(colorName);
}

/**
 * Adds new item to the model. Perform setup of item, delegates to storage and make this item active
 * @param item
 */
void LC_PenPaletteModel::addItem(LC_PenItem *item){
    setupItemForDisplay(item);
    m_penPaletteData->addItem(item);
    m_activePen = item;
    update(false);
    emitModelChange();
}
/**
 * Finds pen with given name. Name is expected to be unique
 * @param name name to find
 * @return item or null if not found
 */
LC_PenItem *LC_PenPaletteModel::findPenForName(QString &name){
    return m_penPaletteData->findItemWithName(name);
}

/**
 * Handles editing of pen item
 * @param item
 */
void LC_PenPaletteModel::itemEdited(LC_PenItem* item){
    setupItemForDisplay(item);
    m_penPaletteData->itemEdited(item);
    m_activePen = item;
    update(false);
    emitModelChange();
}
/**
 * Removes given item from model and underlying storage
 * @param item
 */
void LC_PenPaletteModel::removeItem(LC_PenItem *item){
    bool activePenRemoval = item == m_activePen;
    m_penPaletteData->removeItem(item);
    update(false);
    if (activePenRemoval){
        // is it the last one?
        if (m_displayItems.isEmpty()){
            m_activePen = nullptr;
        } else {
            m_activePen = m_displayItems.at(0);
        }
    }
    emitModelChange();
}

/**
 * Returns item for given model index
 * @param index model index
 * @return item or nullptr if index is not valid
 */
LC_PenItem *LC_PenPaletteModel::getItemForIndex(QModelIndex index){
    LC_PenItem* result = nullptr;
    if (index.isValid()){
        int row = index.row();
        if (row < m_displayItems.size()){
            result = m_displayItems.at(row);
        }
    }
    return result;
}

/**
 * Set given pen as active
 * @param l
 */
void LC_PenPaletteModel::setActivePen(LC_PenItem *l){
    beginResetModel();
    m_activePen = l;
    emitModelChange();
    endResetModel();
}

/**
 * Returns active pen item
 * @return
 */
LC_PenItem *LC_PenPaletteModel::getActivePen() const{
    return m_activePen;
}

/**
 * Delegates creation of new pen to underlying data
 * @param qString
 * @return
 */
LC_PenItem *LC_PenPaletteModel::createNewItem(QString name){
    return m_penPaletteData->createNewPenItem(name);
}
