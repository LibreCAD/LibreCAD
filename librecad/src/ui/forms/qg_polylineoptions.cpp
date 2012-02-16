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

/*
 *  Constructs a QG_PolylineOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PolylineOptions::QG_PolylineOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PolylineOptions::~QG_PolylineOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PolylineOptions::languageChange()
{
    retranslateUi(this);
}

void QG_PolylineOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/PolylineMode",cbMode->currentIndex());
    RS_SETTINGS->writeEntry("/PolylineRadius", leRadius->text());
    RS_SETTINGS->writeEntry("/PolylineAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/PolylineReversed", (int)rbNeg->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_PolylineOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawPolyline) {
        action = (RS_ActionDrawPolyline*)a;

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
            action->setMode(mode);
            action->setReversed(reversed);
        }
        leRadius->setText(sd1);
        leAngle->setText(sd2);
        cbMode->setCurrentIndex(mode);
        rbNeg->setChecked(reversed);
        updateMode(mode);
    } else {
        std::cerr << "QG_PolylineOptions::setAction: wrong action type\n";
        action = NULL;
    }
}

void QG_PolylineOptions::close() {
    if (action!=NULL) {
        action->close();
    }
}

void QG_PolylineOptions::undo() {
    if (action!=NULL) {
        action->undo();
    }
}

void QG_PolylineOptions::updateRadius(const QString& s) {
    if (action!=NULL) {
        action->setRadius(RS_Math::eval(s));
    }
}

void QG_PolylineOptions::updateAngle(const QString& s) {
    if (action!=NULL) {
        double a=RS_Math::eval(s);
//	QString sr;
        if (a>359.999) {
            a=359.999;
            leAngle->setText(QString("%1").arg(a));
        }
        else if (a<0.0) {
            a=0.0;
            leAngle->setText(QString("%1").arg(a));
        }
        action->setAngle(a);
    }
}

void QG_PolylineOptions::updateDirection(bool /*pos*/) {
    if (action!=NULL) {
        action->setReversed(!(rbPos->isChecked()));
    }
}

void QG_PolylineOptions::updateMode( int m )
{
    enum Mode {
        Line,
        Tangential,
        TanRad,
//	TanAng,
//	TanRadAng,
        Ang,
//	RadAngEndp,
//	RadAngCenp
    };

    if (action!=NULL) {
        action->setMode(m);
    }
    switch(m) {
        case Line:
        case Tangential:
            leRadius->setDisabled(true);
            leAngle->setDisabled(true);
            lRadius->setDisabled(true);
            lAngle->setDisabled(true);
            buttonGroup1->setDisabled(true);
            break;
        case TanRad:
            leRadius->setDisabled(false);
            leAngle->setDisabled(true);
            lRadius->setDisabled(false);
            lAngle->setDisabled(true);
            buttonGroup1->setDisabled(true);
            break;
//        case TanAng:
        case Ang:
            leRadius->setDisabled(true);
            leAngle->setDisabled(false);
            lRadius->setDisabled(true);
            lAngle->setDisabled(false);
            buttonGroup1->setDisabled(false);
            break;
/*        case TanRadAng:
        case RadAngEndp:
        case RadAngCenp:
            leRadius->setDisabled(false);
            leAngle->setDisabled(false);
            lRadius->setDisabled(false);
            lAngle->setDisabled(false);
            buttonGroup1->setDisabled(false);*/
    }
}
