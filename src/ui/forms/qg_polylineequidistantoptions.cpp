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
#include "qg_polylineequidistantoptions.h"

#include "rs_actionpolylineequidistant.h"
#include "rs_settings.h"

/*
 *  Constructs a QG_LineRelAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_PolylineEquidistantOptions::QG_PolylineEquidistantOptions(QWidget* parent, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_PolylineEquidistantOptions::~QG_PolylineEquidistantOptions()
{
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/PolylineEquidisantDist", leDist->text());
    RS_SETTINGS->writeEntry("/PolylineEquidisantCopies", leNumber->text());
    RS_SETTINGS->endGroup();
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_PolylineEquidistantOptions::languageChange()
{
    retranslateUi(this);
}

void QG_PolylineEquidistantOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionPolylineEquidistant) {
        action = (RS_ActionPolylineEquidistant*)a;

        QString sd;
        QString sn;

        // settings from action:
        if (update) {
            sd = QString("%1").arg(action->getDist());
            sn = QString("%1").arg(action->getNumber());
        }
        // settings from config file:
        else {
            RS_SETTINGS->beginGroup("/Draw");
            sd = RS_SETTINGS->readEntry("/PolylineEquidisantDist", "10.0");
            sn = RS_SETTINGS->readEntry("/PolylineEquidisantCopies", "1");
            RS_SETTINGS->endGroup();
        }

        leDist->setText(sd);
        leNumber->setText(sn);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_PolylineEquidistantOptions::setAction: wrong action type");
        this->action = NULL;
    }
}

void QG_PolylineEquidistantOptions::destroy() {
    if (action!=NULL) {
        RS_SETTINGS->beginGroup("/Draw");
        RS_SETTINGS->writeEntry("/PolylineEquidistantDist", action->getDist());
        RS_SETTINGS->writeEntry("/PolylineEquidistantCopies", action->getNumber());
        RS_SETTINGS->endGroup();
    }
}

void QG_PolylineEquidistantOptions::updateDist(const QString& l) {
    if (action!=NULL) {
        action->setDist(RS_Math::eval(l));
    }
}
void QG_PolylineEquidistantOptions::updateNumber(const QString& l) {
    int i=abs(l.toInt());
    if(!i) {
        i=1;
        leNumber->setText(QString::number(i));
    }
    if (action!=NULL) {
        action->setNumber(i);
    }
}
