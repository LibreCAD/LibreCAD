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
#ifndef QG_LINEPARALLELTHROUGHOPTIONS_H
#define QG_LINEPARALLELTHROUGHOPTIONS_H

#include "ui_qg_lineparallelthroughoptions.h"

class RS_ActionInterface;
class RS_ActionDrawLineParallelThrough;

class QG_LineParallelThroughOptions : public QWidget, public Ui::QG_LineParallelThroughOptions
{
    Q_OBJECT

public:
    QG_LineParallelThroughOptions(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~QG_LineParallelThroughOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateNumber( int n );

protected:
    RS_ActionDrawLineParallelThrough* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_LINEPARALLELTHROUGHOPTIONS_H
