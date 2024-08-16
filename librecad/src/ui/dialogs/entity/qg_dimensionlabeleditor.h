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
#ifndef QG_DIMENSIONLABELEDITOR_H
#define QG_DIMENSIONLABELEDITOR_H

#include "ui_qg_dimensionlabeleditor.h"
class RS_Dimension;

class QG_DimensionLabelEditor : public QWidget, public Ui::QG_DimensionLabelEditor
{
    Q_OBJECT

public:
    QG_DimensionLabelEditor(QWidget* parent = nullptr, Qt::WindowFlags fl = {});
    ~QG_DimensionLabelEditor();

    void setRadialType(const RS_Dimension&);
    virtual QString getLabel();

public slots:
    virtual void setLabel( const QString & l );
    virtual void insertSign( const QString & s );

protected slots:
    virtual void languageChange();

};

#endif // QG_DIMENSIONLABELEDITOR_H
