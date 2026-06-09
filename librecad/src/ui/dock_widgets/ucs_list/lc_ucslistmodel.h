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

#ifndef LC_UCSLISTMODEL_H
#define LC_UCSLISTMODEL_H

#include <QAbstractTableModel>
#include <QIcon>

#include "lc_formatter.h"

class LC_UCS;
class LC_UCSList;
class LC_UCSListOptions;

class LC_UCSListModel:public QAbstractTableModel{
    Q_OBJECT
public:
    explicit LC_UCSListModel(LC_UCSListOptions *modelOptions, QObject * parent = nullptr);

    ~LC_UCSListModel() override;
    void setUCSList(LC_UCSList *ucsList, LC_Formatter* formatter);

    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    LC_UCS *getItemForIndex(const QModelIndex &index) const;
    int translateColumn(int column) const;
    void fillUCSsList(QList<LC_UCS *> &list) const;
    QIcon getTypeIcon(const LC_UCS *ucs) const;
    QIcon getOrthoTypeIcon(const LC_UCS *ucs) const;
    QModelIndex getIndexForUCS(const LC_UCS* ucs) const;
    void markActive(const LC_UCS *ucs);
    LC_UCS* getActiveUCS() const;
    int count() const;
    LC_UCS *getWCS() const;

    /**
   * Columns that are shown in the table
   */
    enum COLUMNS{
        ICON_TYPE,
        ICON_ORTHO_TYPE,
        NAME,
        UCS_DETAILS,
        LAST
    };

    QString getUCSInfo(const LC_UCS *ucs) const;

    struct UCSItem{
        LC_UCS* ucs;
        QIcon iconType;
        QIcon iconGridType;
        QString ucsInfo;
        QString toolTip;
        QString displayName;
    };

protected:
    LC_UCSList* m_ucsList {nullptr};
    QList<UCSItem*> m_ucss;
    QIcon m_iconWCS;
    QIcon m_iconUCS;
    QIcon m_iconGridOrtho;
    QIcon m_iconGridISOTop;
    QIcon m_iconGridISOLeft;
    QIcon m_iconGridISORight;
    LC_UCSListOptions* m_options {nullptr};
    LC_Formatter* m_formatter {nullptr};
    QString getGridViewType(int orthoType);
    UCSItem* createUCSItem(LC_UCS *ucs);
};

#endif
