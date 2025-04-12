/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
**
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
#ifndef QG_DLGPOLYLINE_H
#define QG_DLGPOLYLINE_H

class RS_Polyline;

#include "ui_qg_dlgpolyline.h"
#include "lc_entitypropertiesdlg.h"

class QG_DlgPolyline : public LC_EntityPropertiesDlg, public Ui::QG_DlgPolyline{
    Q_OBJECT
public:
    QG_DlgPolyline(QWidget *parent, LC_GraphicViewport *pViewport, RS_Polyline* polyline);
    ~QG_DlgPolyline() override;
public slots:
    void updateEntity() override;
protected slots:
    void languageChange();
protected:
    RS_Polyline* m_entity;
    void setEntity(RS_Polyline *e);
};

#endif // QG_DLGPOLYLINE_H
