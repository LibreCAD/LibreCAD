/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
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
#ifndef QG_ARCTANGENTIALOPTIONS_H
#define QG_ARCTANGENTIALOPTIONS_H

#include "ui_qg_arctangentialoptions.h"
#include "rs_actiondrawarctangential.h"

class QG_ArcTangentialOptions : public QWidget, public Ui::QG_ArcTangentialOptions
{
    Q_OBJECT

public:
    QG_ArcTangentialOptions(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~QG_ArcTangentialOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateRadius( const QString & s );

protected:
    RS_ActionDrawArcTangential* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_ARCTANGENTIALOPTIONS_H
