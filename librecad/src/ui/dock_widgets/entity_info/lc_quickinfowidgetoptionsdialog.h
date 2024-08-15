/****************************************************************************
*
* Options Dialog for QuickInfo widget related functions

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

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
**********************************************************************/
#ifndef LC_QUICKINFOWIDGETOPTIONSDIALOG_H
#define LC_QUICKINFOWIDGETOPTIONSDIALOG_H

#include <QDialog>
#include "lc_quickinfowidgetoptions.h"

namespace Ui {
class LC_QuickInfoWidgetOptionsDialog;
}

class LC_QuickInfoWidgetOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LC_QuickInfoWidgetOptionsDialog(QWidget *parent, LC_QuickInfoOptions *pOptions);
    ~LC_QuickInfoWidgetOptionsDialog() override;

public slots:
    void validate();
    void onDefaultActionAutoClicked();

protected slots:
    virtual void languageChange();

private:
    Ui::LC_QuickInfoWidgetOptionsDialog *ui;
    LC_QuickInfoOptions *options;
};

#endif // LC_QUICKINFOWIDGETOPTIONSDIALOG_H
