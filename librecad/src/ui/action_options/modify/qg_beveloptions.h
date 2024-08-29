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
#ifndef QG_BEVELOPTIONS_H
#define QG_BEVELOPTIONS_H

#include<memory>
#include "lc_actionoptionswidgetbase.h"

class RS_ActionModifyBevel;
class RS_ActionInterface;
namespace Ui {
class Ui_BevelOptions;
}

class QG_BevelOptions:public LC_ActionOptionsWidgetBase {
Q_OBJECT

public:
    QG_BevelOptions();
    ~QG_BevelOptions() override;
public slots:
    void languageChange() override;
    void onTrimToggled(bool checked);
    void onLength1EditingFinished();
    void onLength2EditingFinished();
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    RS_ActionModifyBevel *action = nullptr;
    std::unique_ptr<Ui::Ui_BevelOptions> ui;
    void setLength1ToActionAndView(QString val);
    void setLength2ToActionAndView(QString val);
    void setTrimToActionAndView(bool val);
};

#endif // QG_BEVELOPTIONS_H
