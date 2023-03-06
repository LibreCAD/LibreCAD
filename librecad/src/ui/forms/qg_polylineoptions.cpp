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


#include <cmath>

#include "rs_math.h"
#include "rs_debug.h"
#include "rs_settings.h"
#include "rs_actiondrawpolyline.h"
#include "ui_qg_polylineoptions.h"

#include "qg_polylineoptions.h"


using wLists = std::initializer_list<QWidget*>;


/*
    Constructs a QG_PolylineOptions as a child of 'parent', 
    with the name 'name' and widget flags set to 'f'.
*/
QG_PolylineOptions::QG_PolylineOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_PolylineOptions{})
{
	ui->setupUi(this);
}


/* Destroys the object and frees any allocated resources. */
QG_PolylineOptions::~QG_PolylineOptions()
{
    destroy();
}


/* Sets the strings of the subwidgets using the current language. */
void QG_PolylineOptions::languageChange()
{
	ui->retranslateUi(this);
}


void QG_PolylineOptions::destroy()
{
	RS_SETTINGS->writeEntry("Draw/PolylineMode",     ui->cbMode->currentIndex());
	RS_SETTINGS->writeEntry("Draw/PolylineRadius",   ui->leRadius->text());
	RS_SETTINGS->writeEntry("Draw/PolylineAngle",    ui->leAngle->text());
	RS_SETTINGS->writeEntry("Draw/PolylineReversed", (int) ui->rbNeg->isChecked());
}


void QG_PolylineOptions::setAction(RS_ActionInterface* a, bool update)
{
    if ((a != nullptr) && (a->rtti() == RS2::ActionDrawPolyline))
    {
		action = static_cast<RS_ActionDrawPolyline*>(a);

        const bool wasPos = ui->rbPos->isChecked();

        QString   sd1;
        QString   sd2;
        bool    isPos;
        int      mode;

        if (update)
        {
            sd1   = QString("%1").arg(action->getRadius());
            sd2   = QString("%1").arg(action->getAngle());
            isPos = action->isReversed();
            mode  = action->getMode();
        }
        else
        {
            sd1   = RS_SETTINGS->readEntry    ("Draw/PolylineRadius",  "1.0");
            sd2   = RS_SETTINGS->readEntry    ("Draw/PolylineAngle",   "180.0");
            isPos = RS_SETTINGS->readNumEntry ("Draw/PolylineReversed", 0);
            mode  = RS_SETTINGS->readNumEntry ("Draw/PolylineMode",     0);
        }

        updateRadius(sd1);
        updateAngle(sd2);
        updateMode(mode);

        updateDirection(wasPos != isPos, update);
    }
    else
    {
		RS_DEBUG->print( RS_Debug::D_ERROR, 
                         QString("QG_PolylineOptions::setAction:" + tr("wrong action type")).toStdString().c_str());
		action = nullptr;
    }
}


void QG_PolylineOptions::close()
{
    if (action) action->close();
}


void QG_PolylineOptions::undo()
{
    if (action) action->undo();
}


void QG_PolylineOptions::updateRadius(const QString& s)
{
    if (action)
    {
        const double r = RS_Math::eval(s);

        ui->leRadius->setText(QString("%1").arg(r));

        action->setRadius(r);
    }
}


void QG_PolylineOptions::updateAngle(const QString& s)
{
    if (action)
    {
        const double a = fmod(fabs(RS_Math::eval(s)), 360.0);

        ui->leAngle->setText(QString("%1").arg(a));

        action->setAngle(a);
    }
}


void QG_PolylineOptions::updateDirection(const bool& toggled, const bool& update)
{
    if (action)
    {
        if (update)
        {
            ui->rbPos->setChecked(!action->isReversed());
            ui->rbNeg->setChecked(action->isReversed());
        }
        else
        {
            action->setReversed(ui->rbNeg->isChecked());
        }
    }
}


void QG_PolylineOptions::updateMode(const int& m)
{
    if (action) action->setMode((RS_ActionDrawPolyline::SegmentMode) m);

    ui->cbMode->setCurrentIndex(m);

    switch((RS_ActionDrawPolyline::SegmentMode) m)
    {
        case RS_ActionDrawPolyline::Line:
        case RS_ActionDrawPolyline::Tangential:
        default:
		    for(QWidget* p: wLists{ui->leRadius, ui->leAngle, ui->lRadius, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg})
                p->hide();
            break;

        case RS_ActionDrawPolyline::TanRad:
		    for(QWidget* p: wLists{ui->leAngle, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg})
                p->hide();
		    for(QWidget* p: wLists{ui->leRadius, ui->lRadius})
                p->show();
            break;

        case RS_ActionDrawPolyline::Ang:
		    for(QWidget* p: wLists{ui->leRadius, ui->lRadius})
                p->hide();
		    for(QWidget* p: wLists{ui->leAngle, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg})
                p->show();
            break;
    }
}

