/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
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
#ifndef QG_DLGSPOLYLINE_H
#define QG_DLGSPOLYLINE_H

class RS_Polyline;

#include "ui_qg_dlgpolyline.h"

class QG_DlgPolyline : public QDialog, public Ui::QG_DlgPolyline
{
    Q_OBJECT

public:
    QG_DlgPolyline(QWidget* parent = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgPolyline();

public slots:
    virtual void setPolyline( RS_Polyline & e );
    virtual void updatePolyline();

protected slots:
    virtual void languageChange();

private:
    RS_Polyline* polyline;

};

#endif // QG_DLGSPOLYLINE_H
