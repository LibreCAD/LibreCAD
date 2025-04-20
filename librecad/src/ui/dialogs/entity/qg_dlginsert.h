/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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
#ifndef QG_DLGINSERT_H
#define QG_DLGINSERT_H

class RS_Insert;

#include "ui_qg_dlginsert.h"
#include "lc_entitypropertiesdlg.h"

class QG_DlgInsert : public LC_EntityPropertiesDlg, public Ui::QG_DlgInsert{
    Q_OBJECT
public:
    QG_DlgInsert(QWidget *parent, LC_GraphicViewport *pViewport, RS_Insert* insert);
    ~QG_DlgInsert() override;
public slots:
    void updateEntity() override;
protected slots:
    void languageChange();
protected:
    RS_Insert* m_entity;
    void setEntity(RS_Insert *i);
};

#endif // QG_DLGINSERT_H
