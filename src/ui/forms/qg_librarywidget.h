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
#ifndef QG_LIBRARYWIDGET_H
#define QG_LIBRARYWIDGET_H

#include "ui_qg_librarywidget.h"

class QG_LibraryWidget : public QWidget, public Ui::QG_LibraryWidget
{
    Q_OBJECT

public:
    QG_LibraryWidget(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LibraryWidget();

    virtual QString getItemDir( Q3ListViewItem * item );
    virtual QString getItemPath( Q3IconViewItem * item );
    virtual QPixmap getPixmap( const QString & dir, const QString & dxfFile, const QString & dxfPath );
    virtual QString getPathToPixmap( const QString & dir, const QString & dxfFile, const QString & dxfPath );

public slots:
    virtual void setActionHandler( QG_ActionHandler * ah );
    virtual void keyPressEvent( QKeyEvent * e );
    virtual void insert();
    virtual void appendTree( QG_ListViewItem * item, QString directory );
    virtual void updatePreview( Q3ListViewItem * item );

signals:
    void escape();

protected slots:
    virtual void languageChange();

private:
    QG_ActionHandler* actionHandler;

    void init();

};

#endif // QG_LIBRARYWIDGET_H
