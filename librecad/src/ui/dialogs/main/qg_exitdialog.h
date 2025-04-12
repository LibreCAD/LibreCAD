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

#include "lc_dialog.h"
class QAbstractButton;

namespace Ui {
    class QG_ExitDialog;
}

class QG_ExitDialog : public LC_Dialog{
    Q_OBJECT
public:
    QG_ExitDialog(QWidget* parent = nullptr);
	~QG_ExitDialog() override;
	enum ExitDialogResult {
        Cancel, DontSave, DontSaveAll, Save, SaveAll
	};

public slots:
    void setText( const QString & text );
    void setTitle( const QString & text );
    void setForce( bool force );
	void setShowOptionsForAll(bool show);
    void clicked(QAbstractButton * button);
protected slots:
    void languageChange();
private:
    void init();
	std::unique_ptr<Ui::QG_ExitDialog> ui;
};

#endif // QG_EXITDIALOG_H
