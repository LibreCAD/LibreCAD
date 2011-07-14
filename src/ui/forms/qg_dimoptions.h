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
#ifndef QG_DIMOPTIONS_H
#define QG_DIMOPTIONS_H

#include "ui_qg_dimoptions.h"
#include "rs_actiondimension.h"

class QG_DimOptions : public QWidget, public Ui::QG_DimOptions
{
    Q_OBJECT

public:
    QG_DimOptions(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~QG_DimOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateLabel();
    virtual void insertSign( const QString & c );

protected:
    RS_ActionDimension* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_DIMOPTIONS_H
