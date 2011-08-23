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
#ifndef QG_DLGMOVEROTATE_H
#define QG_DLGMOVEROTATE_H

#include "ui_qg_dlgmoverotate.h"

class RS_MoveRotateData;

class QG_DlgMoveRotate : public QDialog, public Ui::QG_DlgMoveRotate
{
    Q_OBJECT

public:
    QG_DlgMoveRotate(QWidget* parent = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgMoveRotate();

public slots:
    virtual void setData( RS_MoveRotateData * d );
    virtual void updateData();

protected slots:
    virtual void languageChange();

private:
    bool useCurrentAttributes;
    bool useCurrentLayer;
    int numberMode;
    QString copies;
    RS_MoveRotateData* data;
    QString angle;

    void init();
    void destroy();

};

#endif // QG_DLGMOVEROTATE_H
