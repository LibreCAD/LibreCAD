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
#ifndef QG_ARCTANGENTIALOPTIONS_H
#define QG_ARCTANGENTIALOPTIONS_H

#include<memory>
#include<QWidget>
#include "lc_actionoptionswidgetbase.h"

class RS_ActionInterface;
class RS_ActionDrawArcTangential;
namespace Ui {
class Ui_ArcTangentialOptions;
}

class QG_ArcTangentialOptions : public LC_ActionOptionsWidgetBase
{
    Q_OBJECT
public:
    QG_ArcTangentialOptions();
    ~QG_ArcTangentialOptions() override;
public slots:
    virtual void onRadiusEditingFinished();
    virtual void onAngleEditingFinished();
    virtual void onRadiusClicked(bool checked);
    virtual void onAngleClicked(bool checked);
    void languageChange() override;
public:
    void updateRadius(double d);
    void updateAngle(double a);
protected:
    RS_ActionDrawArcTangential* action = nullptr;
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void setRadiusToActionAndView(const QString& s  );
    void setAngleToActionAndView(const QString& s  );
    void setByRadiusToActionAndView(const bool byRadius);
    std::unique_ptr<Ui::Ui_ArcTangentialOptions> ui;
};

#endif // QG_ARCTANGENTIALOPTIONS_H
