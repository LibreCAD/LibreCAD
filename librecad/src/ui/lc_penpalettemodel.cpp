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

static int COLOR_ICON_SIZE = 24;

LC_PenPaletteModel::LC_PenPaletteModel(LC_PenPaletteOptions *modelOptions, LC_PenPaletteData* data, QObject * parent) :
    QAbstractTableModel(parent),
    m_filteringRegexp{std::make_unique<QRegularExpression>()}
{
    options = modelOptions;
    penPaletteData = data;
    update(true);
}

LC_PenPaletteModel::~LC_PenPaletteModel() = default;

int LC_PenPaletteModel::rowCount ([[maybe_unused]] const QModelIndex & parent) const {
    return displayItems.size();
}

QModelIndex LC_PenPaletteModel::parent ( const QModelIndex & /*index*/ ) const {
    return QModelIndex();
}

QModelIndex LC_PenPaletteModel::index ( int row, int column, const QModelIndex & /*parent*/ ) const {
    if (row >= displayItems.size() || row < 0)
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
    displayItems.clear();

    // simply iterate by pens data
    int count = penPaletteData->getItemsCount();
    for (int i=0; i < count; i++){
        LC_PenItem* item = penPaletteData->getItemAt(i);
        QString itemName = item->getName();

        if (hasRegexp){
            // check whether filtering is case-sensitive
            if (options->ignoreCaseOnMatch){
                itemName = itemName.toLower();
            }
            QRegularExpressionMatch match = m_filteringRegexp->match(itemName);
            bool hasRegexpMatch = match.hasMatch();
            item->setMatched(hasRegexpMatch);

            if (options->filterIsInHighlightMode){
                // just add item into the list to display
                displayItems << item;
            } else if (hasRegexpMatch){
                // add it item as highlighted
                displayItems << item;
            } else {
                continue; // skip this item at all as it was not matched, so it will not be shown in the table
            }
        }
        else{ // just add all items to display
            displayItems << item;
            item->setMatched(false);
        }

        // names should be updated on initial build and after options change
        if (updateNames){
            setupItemForDisplay(item);
        }
    }

    // sort collected items alphabetically
    std::sort(displayItems.begin(), displayItems.end(), [](const LC_PenItem *s1, const LC_PenItem *s2)-> bool{
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
    QRegularExpression::PatternOptions option = options->ignoreCaseOnMatch
                                                    ? QRegularExpression::CaseInsensitiveOption
                                                    : QRegularExpression::NoPatternOption;
    // ensure that we'll use case-insensitive match
    m_filteringRegexp->setPatternOptions(option);
    hasRegexp = !regexp.trimmed().isEmpty();
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
    if (row >= displayItems.size() || row < 0)
        return nullptr;
    return displayItems.at(row);
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
    if (!index.isValid() || index.row() >= displayItems.size())
        return QVariant();

    LC_PenItem *item{displayItems.at(index.row())};

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
            if (activePen && activePen == item){
                return options->activeItemBGColor;
            }
            break;
        }
        case Qt::ForegroundRole: {
            if (item->isMatched()){
                // highlighting of items that are matched by filter regexp
                if (options->filterIsInHighlightMode){
                    return options->matchedItemColor;
                }
            }
            break;
        }
        case Qt::ToolTipRole: {
            // optional tooltip displaying
            if (options->showToolTip){
                QString toolTip = QString("Name: <b>").append(item->getName()).append("</b><br>")
                    .append("Line Type: <b>").append(item->getLineTypeName()).append("</b><br>")
                    .append("Line Width: <b>").append(item->getLineWidthName()).append("</b><br>");
                RS_Color color = item->getColor();
                if (color.isValid()){
                    toolTip = toolTip.append("Color: <b><font color='").append(color.name()).append("'>").append(item->getColorName()).append("</font></b>");
                } else {
                    toolTip = toolTip.append("Color: <b>").append(item->getColorName()).append("</b>");
                }
                return toolTip;
            }
            break;
        }
        case Qt::FontRole: {
            // fonts - bold for active and italic for special attribute values
            QFont font;
            bool fontChanged = false;
            if (activePen && activePen == item){
                if (options->showEntireRowBold){
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
    if (!options->showColorIcon){
       if (result >= COLUMNS::COLOR_ICON){
         result++;
       }
    }
    if (!options->showColorName){
        if (result >= COLUMNS::COLOR_NAME){
            result++;
        }
    }
    if (!options->showWidthIcon){
        if (result >= COLUMNS::WIDTH_ICON){
            result++;
        }
    }
    if (!options->showWidthName){
        if (result >= COLUMNS::WIDTH_NAME){
            result++;
        }
    }

    if (!options->showTypeIcon){
        if (result >= COLUMNS::TYPE_ICON){
            result++;
        }
    }
    if (!options->showTypeName){
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
    if (!options->showColorIcon){
        result --;
    }
    if (!options->showColorName){
        result --;
    }
    if (!options->showTypeIcon){
        result --;
    }
    if (!options->showTypeName){
        result --;
    }
    if (!options->showWidthIcon){
        result --;
    }
    if (!options->showWidthName){
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
    QIcon iconLineType = registry->getLineTypeIcon(lineType);
    QString lineTypeName = registry->getLineTypeText(lineType);

    newPen->setLineTypeIcon(iconLineType);
    newPen->setLineTypeName(lineTypeName);

    // handling line width
    RS2::LineWidth lineWidth = newPen->getLineWidth();

    QIcon iconLineWidth = registry->getLineWidthIcon(lineWidth);
    newPen ->setLineWidthIcon(iconLineWidth);
    QString lineWidthName = registry->getLineWidthText(lineWidth);
    newPen -> setLineWidthName(lineWidthName);

    // handling color
    RS_Color color = newPen->getColor();
    QIcon iconColor = registry->getColorIcon(color, COLOR_ICON_SIZE, COLOR_ICON_SIZE);
    newPen->setColorIcon(iconColor);

    QString colorName = registry->getColorName(color, options->colorNameDisplayMode);
    newPen->setColorName(colorName);
}

/**
 * Adds new item to the model. Perform setup of item, delegates to storage and make this item active
 * @param item
 */
void LC_PenPaletteModel::addItem(LC_PenItem *item){
    setupItemForDisplay(item);
    penPaletteData->addItem(item);
    activePen = item;
    update(false);
    emitModelChange();
}
/**
 * Finds pen with given name. Name is expected to be unique
 * @param name name to find
 * @return item or null if not found
 */
LC_PenItem *LC_PenPaletteModel::findPenForName(QString &name){
    return penPaletteData->findItemWithName(name);
}

/**
 * Handles editing of pen item
 * @param item
 */
void LC_PenPaletteModel::itemEdited(LC_PenItem* item){
    setupItemForDisplay(item);
    penPaletteData->itemEdited(item);
    activePen = item;
    update(false);
    emitModelChange();
}
/**
 * Removes given item from model and underlying storage
 * @param item
 */
void LC_PenPaletteModel::removeItem(LC_PenItem *item){
    bool activePenRemoval = item == activePen;
    penPaletteData->removeItem(item);
    update(false);
    if (activePenRemoval){
        // is it the last one?
        if (displayItems.isEmpty()){
            activePen = nullptr;
        } else {
            activePen = displayItems.at(0);
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
        if (row < displayItems.size()){
            result = displayItems.at(row);
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
    activePen = l;
    emitModelChange();
    endResetModel();
}

/**
 * Returns active pen item
 * @return
 */
LC_PenItem *LC_PenPaletteModel::getActivePen() const{
    return activePen;
}

/**
 * Delegates creation of new pen to underlying data
 * @param qString
 * @return
 */
LC_PenItem *LC_PenPaletteModel::createNewItem(QString name){
    return penPaletteData->createNewPenItem(name);
}
