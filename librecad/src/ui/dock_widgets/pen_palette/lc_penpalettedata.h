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
#ifndef LC_PENPALETTEDATA_H
#define LC_PENPALETTEDATA_H

#include <QObject>
#include <QList>

#include "lc_penitem.h"
#include "lc_penpaletteoptions.h"


/**
 * Internal holder of all named pens. Stores the list of them and also offers methods for persistence of pens list in file.
 * This class is used by LC_PenPaletteModel as underlying storage.
 */
class LC_PenPaletteData:public QObject
{
Q_OBJECT
public:
    explicit LC_PenPaletteData(LC_PenPaletteOptions *opts);
    ~LC_PenPaletteData() override;
    /**
     * Load items list from file
     */
    bool loadItems();
    /**
     * Saves list to file
     */
    bool saveItems();

    /**
     * Removes given pen from the list
     * @param item
     */
    void removeItem(LC_PenItem* item);
    /**
     * Add pen to the list
     * @param item
     */
    void addItem(LC_PenItem* item);
    /**
     * Handles editing of given pen
     * @param item
     */
    void itemEdited(LC_PenItem *item);
    /*
     * Returns the size of pens list
     */
    int getItemsCount();
    /**
     * Returns pen info at given position in the list
     * @param index
     * @return
     */
    LC_PenItem* getItemAt(int index);
    /**
     * Returns first item with given name. It is expected that names of pen items are unique
     * @param name name of pen
     * @return item if it is found or nullptr
     */
    LC_PenItem* findItemWithName(QString &name);
    LC_PenItem *createNewPenItem(QString name);

signals:
    void modelDataChange();

private:
    /**
     * List of named pens. Ordering of items is not important there.
     */
    QList<LC_PenItem*> persistentItems;
    /**
     * Parses given string and returns pen
     * @param str
     * @return
     */
    LC_PenItem *fromStringRepresentation(QString &str);
    /**
     * Converts pen into string
     * @param item
     * @return
     */
    QString toStringRepresentation(LC_PenItem *item);
    /**
     * pens info registry
     */
    LC_PenInfoRegistry* registry = LC_PenInfoRegistry::instance();
    /**
     *reference to options
     */
    LC_PenPaletteOptions *options = nullptr;
    void createDefaultPens();
    LC_PenItem *doCreateNewDefaultPenItem(QString penName, RS2::LineType lineType, RS2::LineWidth lineWidth, RS_Color color);
    void emitDataChange();
};

#endif // LC_PENPALETTEDATA_H
