/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Dongxu Li ( dongxuli2011@gmail.com )
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#ifndef QG_MODIFYOFFSETOPTIONS_H
#define QG_MODIFYOFFSETOPTIONS_H

#include<memory>
#include<QWidget>

class RS_ActionInterface;
class RS_ActionDrawModifyOffset;
namespace Ui {
class Ui_ModifyOffsetOptions;
}

class QG_ModifyOffsetOptions : public QWidget
{
    Q_OBJECT

public:
    QG_ModifyOffsetOptions(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~QG_ModifyOffsetOptions();

public slots:
//    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateDist( const QString & d );
    virtual void setDist( double& d , bool initial=true);

protected:
//    RS_ActionModifyOffset* action;
    double* dist;

protected slots:
    virtual void languageChange();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_ModifyOffsetOptions> ui;
};

#endif
//EOF
