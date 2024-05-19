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
#include <QTextStream>
#include "lc_penpalettedata.h"
/**
 * Separator for fields of pen in persistent string
 */
static const char *const PEN_DATA_FIELDS_SEPARATOR = ",";

LC_PenPaletteData::LC_PenPaletteData(LC_PenPaletteOptions *opts){
    options = opts;
}

LC_PenPaletteData::~LC_PenPaletteData(){
    qDeleteAll(persistentItems);
    persistentItems.clear();
}
/**
 * Saves list of pens in the underlying file.
 * The file format is CSV, each line represents one pen
 */
bool LC_PenPaletteData::saveItems(){
    QString fileName = options->pensFileName;
    QFile file(fileName);
    bool result = false;
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif

        // just convert each pen to string and store in file
        int count = persistentItems.count();
        for (int i= 0; i < count; i++){
            LC_PenItem* item = persistentItems.at(i);
            out << toStringRepresentation(item);
        }
        file.close();
        result = true;
    }
    else{
        // handle error later
    }
    return result;
}

bool LC_PenPaletteData::loadItems(){
    QString fileName = options->pensFileName;
    bool result = false;
    QFile file(fileName);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    if (file.isOpen()){
        QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        in.setEncoding(QStringConverter::Utf8);
#else
        in.setCodec("UTF-8");
#endif
        while (!in.atEnd())
        {
            QString line = in.readLine();
            LC_PenItem* item = fromStringRepresentation(line);
            if (item != nullptr){ // if null - some parsing issue occurred
                persistentItems << item;
            }
        }
        file.close();
        return true;
    }
    else {
        createDefaultPens();
    }
    return result;
}

/**
 * Quite naive implementation that stores all attributes of pen in string using common separator.
 * @param item
 * @return
 */
QString LC_PenPaletteData::toStringRepresentation(LC_PenItem *item){

    RS2::LineType lineType = item->getLineType();
    RS2::LineWidth lineWidth = item->getLineWidth();
    RS_Color color = item->getColor();
    QString name = item->getName();

    QString lineTypeName = QString().setNum((short)lineType);
    QString lineWidthName =  QString().setNum(lineWidth);
    QString colorName = registry->getInternalColorString(color);

    QString result = lineTypeName.append(PEN_DATA_FIELDS_SEPARATOR).append(lineWidthName).append(PEN_DATA_FIELDS_SEPARATOR).append(colorName).append(
        PEN_DATA_FIELDS_SEPARATOR).append(name).append("\n");

    return result;
}
/**
 * Parses provided string to separate fields and constructs pen info item.
 * String is separated, order of field is:
 * Line Type, Line Width, Color, Pen Name
 *
 * @param str string to parse
 * @return created item or nullptr if parsing error occurred
 */
LC_PenItem* LC_PenPaletteData::fromStringRepresentation(QString &str){
    LC_PenItem* result = nullptr;
    const QStringList stringParts = str.split(PEN_DATA_FIELDS_SEPARATOR);

    // it is expected that only 4 fields should be in the string
    if (stringParts.size() == 4){

        // first proceed with linetype
        RS2::LineType lineType;
        QString lineTypeStr = stringParts.at(0).trimmed();
        bool conversionOk = false;
        int type = lineTypeStr.toInt(&conversionOk, 10);
        if (conversionOk){
            // we've converted to int, now we'll check that int value is one of valid line types
            if (registry->hasLineType(type)){
                lineType = static_cast<RS2::LineType>(type);

                // now we'll parse lien width
                QString widthStr = stringParts.at(1).trimmed();
                conversionOk = false;
                int width = widthStr.toInt(&conversionOk, 10);
                if (conversionOk){
                    // if converted to it fine, check whether int is valid width
                    if (registry->hasLineWidth(width)){ // allow valid line width only
                        RS2::LineWidth lineWidth = static_cast<RS2::LineWidth>(width);

                        // here we'll parse color
                        QString colorStr = stringParts.at(2), trimmed;
                        RS_Color color = registry->getColorFromInternalString(colorStr);
                        bool colorValid = color.isValid();
                        if (!colorValid){
                            // the color itself may be invalid, yet it might include important flags, so check for them
                            if (color.isByBlock() || color.isByLayer() || LC_PenInfoRegistry::isUnchangedColor(color)){
                                colorValid = true;
                            }
                        }
                        if (colorValid){

                            // ok, we'll proceed with name
                            QString name = stringParts.at(3);
                            if (!name.isEmpty()){ // skip empty pens without names
                                // check that name of pen is not duplicated
                                if (findItemWithName(name) == nullptr){

                                    // if we're here, all fine and we may create pen
                                    result = new LC_PenItem(name);
                                    result->setLineType(lineType);
                                    result->setLineWidth(lineWidth);
                                    result->setColor(color);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

/**
 * Utility method to notify that model is changed
 */
void LC_PenPaletteData::emitDataChange(){
    emit modelDataChange();
}

/**
 * Removes item from the list and stores updated list in file
 * @param item item to remove
 */
void LC_PenPaletteData::removeItem(LC_PenItem *item){
    persistentItems.removeAll(item);
    delete item;
    emitDataChange();

}
/**
 * Add provided item to the list and stores list in file
 * @param item
 */
void LC_PenPaletteData::addItem(LC_PenItem *item){
    persistentItems << item;
    emitDataChange();
}

/**
 * Simply stores update list in file
 * @param item
 */
void LC_PenPaletteData::itemEdited([[maybe_unused]] LC_PenItem *item){
    emitDataChange();
}

int LC_PenPaletteData::getItemsCount(){
    return persistentItems.count();
}

LC_PenItem *LC_PenPaletteData::getItemAt(int index){
    return persistentItems.at(index);
}

/**
 * Find item with given name. It is expected that names of pens are unique
 * @param name name to search for
 * @return item with given name, or nullptr if nothing is found
 */
LC_PenItem *LC_PenPaletteData::findItemWithName(QString &name){

    int count = persistentItems.count();
    for (int i = 0; i < count; i++){
        LC_PenItem* item = persistentItems.at(i);
        QString itemName = item->getName();
        if (itemName == name){
            return item;
        }
    }
    return nullptr;
}
/**
 * Simply creates new pen with given name
 * @param name name of pen
 * @return created item
 */
LC_PenItem *LC_PenPaletteData::createNewPenItem(QString penName){
    LC_PenItem* penItem = new LC_PenItem(penName);
    return penItem;
}

/**
 * Utility method for creation and initialization of default pen item
 * @param penName name of pen (will be translated internally)
 * @param lineType line type
 * @param lineWidth line width
 * @param color color
 * @return created pen item
 */
LC_PenItem *LC_PenPaletteData::doCreateNewDefaultPenItem(QString penName, RS2::LineType lineType, RS2::LineWidth lineWidth, RS_Color color){
    LC_PenItem* penItem = new LC_PenItem(QObject::tr(penName.toStdString().c_str()));
    penItem->setLineWidth(lineWidth);
    penItem->setLineType(lineType);
    penItem->setColor(color);
    return penItem;
}


/**
 * Creates default pens if nothing is loaded from pens file
 */
void LC_PenPaletteData::createDefaultPens(){
    LC_PenItem *unchangedItem = doCreateNewDefaultPenItem("Unchanged", RS2::LineType::LineTypeUnchanged, RS2::LineWidth::WidthUnchanged,
                                                          LC_PenInfoRegistry::createUnchangedColor());
    persistentItems << unchangedItem;

    LC_PenItem *byLayerItem = doCreateNewDefaultPenItem("By Layer", RS2::LineType::LineByLayer, RS2::LineWidth::WidthByLayer,
                                                        RS_Color(RS2::FlagByLayer));
    persistentItems << byLayerItem;

    LC_PenItem *byBlockItem = doCreateNewDefaultPenItem("By Block", RS2::LineType::LineByBlock, RS2::LineWidth::WidthByBlock,
                                                        RS_Color(RS2::FlagByBlock));

    persistentItems << byBlockItem;
}



