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
#include "rs_actiondrawtext.h"
#include "ui_qg_textoptions.h"

/*
 *  Constructs a QG_TextOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_TextOptions::QG_TextOptions()
    :LC_ActionOptionsWidgetBase(RS2::ActionDrawText, "", ""), ui(new Ui::Ui_TextOptions{}) {
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::textChanged, this, &QG_TextOptions::onAngleChanged);
    connect(ui->leText, &QLineEdit::textChanged, this, &QG_TextOptions::onTextChanged);
    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_TextOptions::~QG_TextOptions() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_TextOptions::languageChange() {
    ui->retranslateUi(this);
}

void QG_TextOptions::doSetAction(RS_ActionInterface *a, bool update) {
    m_action = dynamic_cast<RS_ActionDrawText *>(a);
    QString text;
    QString angle;
    if (update) {
        text = m_action->getText();
        angle = fromDouble(m_action->getUcsAngleDegrees());
    } else {
        text = "";
        angle = "0.0";
    }

//    LC_ERR << " Options: --- " << text << " Angle: " << angle;
/*#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        QTextCodec *codec = QTextCodec::codecForName(iso);
        if (codec) {
            st = codec->toUnicode(RS_FilterDXF::toNativeString(action->getText().local8Bit()));
        } else {
            st = RS_FilterDXF::toNativeString(action->getText().local8Bit());
        }
//#else*/
    ui->leText->setText(text);
//#endif
    ui->leAngle->setText(angle);
}

void QG_TextOptions::onTextChanged() {
    if (m_action) {
/*#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        action->setText(
            RS_FilterDXF::toNativeString( 
			 QString::fromLocal8Bit( QTextCodec::codecForName( iso )->fromUnicode( ui->teText->text() ) )
            )
        );
//#else*/
        const QString &plainText = ui->leText->text();
        m_action->setText(plainText);
//#endif
    }
}

void QG_TextOptions::onAngleChanged() {
    double angle;
    QString val = ui->leAngle->text();
    if (toDoubleAngleDegrees(val, angle, 0.0, false)) {
        m_action->setUcsAngleDegrees(angle);
    }
}

void QG_TextOptions::doSaveSettings() {
}
