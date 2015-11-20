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
#include "qg_polylineoptions.h"

#include "rs_actiondrawpolyline.h"
#include "rs_settings.h"
#include "rs_math.h"
#include "ui_qg_polylineoptions.h"
#include "rs_debug.h"

using wLists = std::initializer_list<QWidget*>;
/*
 *  Constructs a QG_PolylineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PolylineOptions::QG_PolylineOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
	, ui(new Ui::Ui_PolylineOptions{})
{
	ui->setupUi(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PolylineOptions::~QG_PolylineOptions()
{
    destroy();
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PolylineOptions::languageChange()
{
	ui->retranslateUi(this);
}

void QG_PolylineOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
	RS_SETTINGS->writeEntry("/PolylineMode", ui->cbMode->currentIndex());
	RS_SETTINGS->writeEntry("/PolylineRadius", ui->leRadius->text());
	RS_SETTINGS->writeEntry("/PolylineAngle", ui->leAngle->text());
	RS_SETTINGS->writeEntry("/PolylineReversed", (int)ui->rbNeg->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_PolylineOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a && a->rtti()==RS2::ActionDrawPolyline) {
		action = static_cast<RS_ActionDrawPolyline*>(a);

        QString sd1,sd2;
        int mode;
        bool reversed(false);;

        if (update) {
            sd1 = QString("%1").arg(action->getRadius());
            sd2 = QString("%1").arg(action->getAngle());
            mode = action->getMode();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sd1 = RS_SETTINGS->readEntry("/PolylineRadius", "1.0");
            sd2 = RS_SETTINGS->readEntry("/PolylineAngle", "180.0");
            mode = RS_SETTINGS->readNumEntry("/PolylineMode",0);
            reversed = RS_SETTINGS->readNumEntry("/PolylineReversed", 0);
            RS_SETTINGS->endGroup();
            action->setRadius(sd1.toDouble());
            action->setAngle(sd2.toDouble());
            action->setMode((RS_ActionDrawPolyline::SegmentMode)mode);
            action->setReversed(reversed);
        }
		ui->leRadius->setText(sd1);
		ui->leAngle->setText(sd2);
		ui->cbMode->setCurrentIndex(mode);
		ui->rbNeg->setChecked(reversed);
        updateMode(mode);
    } else {
		RS_DEBUG->print(RS_Debug::D_ERROR, QString("QG_PolylineOptions::setAction:"
						+ tr("wrong action type")).toStdString().c_str());
		action = nullptr;
    }
}

void QG_PolylineOptions::close() {
    if (action) {
        action->close();
    }
}

void QG_PolylineOptions::undo() {
    if (action) {
        action->undo();
    }
}

void QG_PolylineOptions::updateRadius(const QString& s) {
    if (action) {
        action->setRadius(RS_Math::eval(s));
    }
}

void QG_PolylineOptions::updateAngle(const QString& s) {
    if (action) {
        double a=RS_Math::eval(s);
//	QString sr;
        if (a>359.999) {
            a=359.999;
			ui->leAngle->setText(QString("%1").arg(a));
        }
        else if (a<0.0) {
            a=0.0;
			ui->leAngle->setText(QString("%1").arg(a));
        }
        action->setAngle(a);
    }
}

void QG_PolylineOptions::updateDirection(bool /*pos*/) {
    if (action) {
		action->setReversed(!(ui->rbPos->isChecked()));
    }
}

void QG_PolylineOptions::updateMode( int m )
{
//    enum Mode {
//        Line,Right-click on the package and choose Mark for Upgrade from the context menu, or press Ctrl + U.
//        Tangential,
//        TanRad,
////	TanAng,
////	TanRadAng,
//        Ang,
////	RadAngEndp,
////	RadAngCenp
//    };

    if (action) {
        action->setMode((RS_ActionDrawPolyline::SegmentMode) m);
    }
    switch((RS_ActionDrawPolyline::SegmentMode) m) {
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
        //        case TanAng:
    case RS_ActionDrawPolyline::Ang:
		for(QWidget* p: wLists{ui->leRadius, ui->lRadius})
            p->hide();
		for(QWidget* p: wLists{ui->leAngle, ui->lAngle, ui->buttonGroup1, ui->rbPos, ui->rbNeg})
            p->show();
        break;
        /*        case TanRadAng:
        case RadAngEndp:
        case RadAngCenp:
			ui->leRadius->setDisabled(false);
			ui->leAngle->setDisabled(false);
			ui->lRadius->setDisabled(false);
			ui->lAngle->setDisabled(false);
			ui->buttonGroup1->setDisabled(false);*/
    }
}
