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
#ifndef QG_DLGIMAGE_H
#define QG_DLGIMAGE_H

#include <memory>
#include "ui_qg_dlgimage.h"
#include "lc_dialog.h"

class RS_Image;

class QG_DlgImage : public LC_Dialog, public Ui::QG_DlgImage
{
    Q_OBJECT

public:
    QG_DlgImage(QWidget *parent = nullptr);

public slots:
    virtual void setImage( RS_Image & e );
    virtual void changeWidth();
    virtual void changeHeight();
    virtual void changeScale();
    virtual void changeDPI();
    virtual void updateImage();
    virtual void setImageFile();


protected slots:
    virtual void languageChange();

private:
    RS_Image* image = nullptr;
    double scale = 1.;
    std::unique_ptr<QDoubleValidator> val;
};

#endif // QG_DLGIMAGE_H
