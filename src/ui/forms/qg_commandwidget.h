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
#ifndef QG_COMMANDWIDGET_H
#define QG_COMMANDWIDGET_H

#include "intermediate/ui/ui_qg_commandwidget.h"

class QG_CommandWidget : public QWidget, public Ui::QG_CommandWidget
{
    Q_OBJECT

public:
    QG_CommandWidget(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_CommandWidget();

    virtual bool checkFocus();

public slots:
    virtual void setFocus();
    virtual void setCommand( const QString & cmd );
    virtual void appendHistory( const QString & msg );
    virtual void trigger();
    virtual void tabPressed();
    virtual void escape();
    virtual void setActionHandler( QG_ActionHandler * ah );
    virtual void setCommandMode();
    virtual void setNormalMode();
    virtual void redirectStderr();
    virtual void processStderr();

protected slots:
    virtual void languageChange();

private:
    QG_ActionHandler* actionHandler;

    void init();

};

#endif // QG_COMMANDWIDGET_H
