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
#include "qg_textoptions.h"

#include <qvariant.h>
#include <qtextcodec.h>
#include "rs_system.h"
#include "rs_filterdxf.h"

/*
 *  Constructs a QG_TextOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_TextOptions::QG_TextOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_TextOptions::~QG_TextOptions()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_TextOptions::languageChange()
{
    retranslateUi(this);
}

void QG_TextOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawText) {
        action = (RS_ActionDrawText*)a;

        QString st;
        QString sa;
        if (update) {
            st = action->getText();
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
        } else {
            st = "";
            sa = "0.0";
        }

/*#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        QTextCodec *codec = QTextCodec::codecForName(iso);
        if (codec!=NULL) {
            st = codec->toUnicode(RS_FilterDXF::toNativeString(action->getText().local8Bit()));
        } else {
            st = RS_FilterDXF::toNativeString(action->getText().local8Bit());
        }
//#else*/
        teText->setText(st);
//#endif
        leAngle->setText(sa);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_TextOptions::setAction: wrong action type");
        action = NULL;
    }

}

void QG_TextOptions::updateText() {
    if (action!=NULL) {
/*#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        action->setText(
            RS_FilterDXF::toNativeString( 
             QString::fromLocal8Bit( QTextCodec::codecForName( iso )->fromUnicode( teText->text() ) )
            )
        );
//#else*/
       action->setText(teText->text());
//#endif
    }
}

void QG_TextOptions::updateAngle() {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
    }
}
