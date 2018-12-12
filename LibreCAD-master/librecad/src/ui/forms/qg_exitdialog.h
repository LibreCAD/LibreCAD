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
#ifndef QG_EXITDIALOG_H
#define QG_EXITDIALOG_H

#include <memory>
#include <QDialog>

class QAbstractButton;

namespace Ui {
class QG_ExitDialog;
}

class QG_ExitDialog : public QDialog
{
    Q_OBJECT

public:
    QG_ExitDialog(QWidget* parent = 0, bool modal = false, Qt::WindowFlags fl = 0);
	~QG_ExitDialog();

public slots:
    virtual void setText( const QString & text );
    virtual void setTitle( const QString & text );
    virtual void setForce( bool force );
    virtual void slotSaveAs();
    virtual void slotSave();
    virtual void clicked(QAbstractButton * button);

protected slots:
    virtual void languageChange();

private:
    void init();
	std::unique_ptr<Ui::QG_ExitDialog> ui;
};

#endif // QG_EXITDIALOG_H
