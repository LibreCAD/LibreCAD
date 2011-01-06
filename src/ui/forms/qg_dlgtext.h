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
#ifndef QG_DLGTEXT_H
#define QG_DLGTEXT_H

#include "intermediate/ui/ui_qg_dlgtext.h"

class QG_DlgText : public QDialog, public Ui::QG_DlgText
{
    Q_OBJECT

public:
    QG_DlgText(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgText();

    virtual int getAlignment();

public slots:
    virtual void updateUniCharComboBox( int );
    virtual void setText( RS_Text & t, bool isNew );
    virtual void updateText();
    virtual void setAlignmentTL();
    virtual void setAlignmentTC();
    virtual void setAlignmentTR();
    virtual void setAlignmentML();
    virtual void setAlignmentMC();
    virtual void setAlignmentMR();
    virtual void setAlignmentBL();
    virtual void setAlignmentBC();
    virtual void setAlignmentBR();
    virtual void setAlignment( int a );
    virtual void setFont( const QString & f );
    virtual void defaultChanged( bool );
    virtual void loadText();
    virtual void load( const QString & fn );
    virtual void saveText();
    virtual void save( const QString & fn );
    virtual void insertSymbol( int );
    virtual void updateUniCharButton( int );
    virtual void insertChar();

protected slots:
    virtual void languageChange();

private:
    bool isNew;
    RS_Text* text;
    RS_Font* font;

    void init();
    void destroy();

};

#endif // QG_DLGTEXT_H
