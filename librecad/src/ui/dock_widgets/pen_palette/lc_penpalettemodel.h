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
#ifndef LC_PENPALETTEMODEL_H
#define LC_PENPALETTEMODEL_H

#include <memory>

#include <QAbstractTableModel>
#include <QIcon>

#include "lc_penitem.h"
#include "lc_peninforegistry.h"
#include "lc_penpaletteoptions.h"
#include "lc_penpalettedata.h"

class QRegularExpression;

/**
 * Table model for pen palette
 */
class LC_PenPaletteModel: public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * Columns that are shown in the table
     */
    enum COLUMNS{
        COLOR_ICON,
        COLOR_NAME,
        WIDTH_ICON,
        WIDTH_NAME,
        TYPE_ICON,
        TYPE_NAME,
        NAME,
        LAST
    };

    // the default icon size
    constexpr static int ICON_WIDTH = 24;

    explicit LC_PenPaletteModel(LC_PenPaletteOptions *modelOptions, LC_PenPaletteData * data, QObject * parent = nullptr);

    ~LC_PenPaletteModel() override;

    Qt::ItemFlags flags (const QModelIndex & index) const override;
    int columnCount(const QModelIndex &) const  override;
    int rowCount (const QModelIndex &) const override;
    QVariant data ( const QModelIndex & index, int role) const override;
    QModelIndex parent ( const QModelIndex & index ) const override;
    QModelIndex index ( int row, int column, const QModelIndex & parent) const override;

    void setActivePen(LC_PenItem* l);
    void setFilteringRegexp(QString &regexp);
    int translateColumn(int column) const;

    void update(bool updateNames);
    LC_PenPaletteOptions* getOptions(){return options;};

    LC_PenItem *createNewItem(QString qString);
    void addItem(LC_PenItem *item);
    void itemEdited(LC_PenItem * item);
    void removeItem(LC_PenItem *pItem);

    LC_PenItem* getActivePen() const;
    LC_PenItem *getPen( int row ) const;
    LC_PenItem *findPenForName(QString &name);
    LC_PenItem *getItemForIndex(QModelIndex index);
signals:
    void modelChange();
private:
    /**
     * List of items that will be displayed in the table - may exclude some pens if filter by name is applied
     */
    QList<LC_PenItem*> displayItems;
    /**
     * Underlying pen data holder
     */
    LC_PenPaletteData *penPaletteData = nullptr;
    /**
     * Pen item that is currently active in table view
     */
    LC_PenItem* activePen {nullptr};
    /**
     * Options that controls model and widget
     */
    LC_PenPaletteOptions* options {nullptr};
    /**
     * regexp string that is used for items filtering/highlighting
     */
    std::unique_ptr<QRegularExpression> m_filteringRegexp;
    /**
     * Flag that indicates regexp presence
     */
    bool hasRegexp{false};
    /**
     * Reference for registry of meta information for line widths, colors and line types
     */
    LC_PenInfoRegistry* registry = LC_PenInfoRegistry::instance();
    void setupItemForDisplay(LC_PenItem *penItem);

    void emitModelChange();

};

#endif // LC_PENPALETTEMODEL_H
