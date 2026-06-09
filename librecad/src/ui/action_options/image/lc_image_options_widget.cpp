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
#include "lc_image_options_widget.h"

#include "lc_action_draw_image.h"
#include "lc_guarded_signals_blocker.h"
#include "ui_lc_image_options_widget.h"

/*
 *  Constructs a QG_ImageOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
LC_ImageOptionsWidget::LC_ImageOptionsWidget()
    :ui(new Ui::LC_ImageOptionsWidget) {
    ui->setupUi(this);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_ImageOptionsWidget::onAngleEditingFinished);
    connect(ui->leFactor, &QLineEdit::editingFinished, this, &LC_ImageOptionsWidget::onFactorEditingFinished);
    connect(ui->leDPI, &QLineEdit::editingFinished, this, &LC_ImageOptionsWidget::onDpiEditingFinished);

    pickAngleSetup("angle", ui->tbPickAngle, ui->leAngle);
}

/*
 *  Destroys the object and frees any allocated resources
 */
LC_ImageOptionsWidget::~LC_ImageOptionsWidget() = default;

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LC_ImageOptionsWidget::languageChange() {
    ui->retranslateUi(this);
}

void LC_ImageOptionsWidget::doUpdateByAction(RS_ActionInterface* a) {
    m_action = static_cast<LC_ActionDrawImage*>(a);

    const QString sAngle = fromDouble(m_action->getUcsAngleDegrees());
    const QString sFactor = fromDouble(m_action->getFactor());

    LC_GuardedSignalsBlocker({ui->leAngle, ui->leFactor, ui->leDPI});

    ui->leAngle->setText(sAngle);

    const double dpi = m_action->scaleToDpi(m_action->getFactor());
    ui->leDPI->setText(QString::number(dpi));
    ui->leFactor->setText(sFactor);
}

void LC_ImageOptionsWidget::onAngleEditingFinished() {
    const auto val = ui->leAngle->text();
    double angleDegree = 0.;
    if (toDoubleAngleDegrees(val, angleDegree, 0.0, false)) {
        m_action->setUcsAngleDegrees(angleDegree);
    }
    m_action->updateOptions();
}

void LC_ImageOptionsWidget::onDpiEditingFinished() {
    const auto val = ui->leDPI->text();
    double dpi;
    const bool ok = toDouble(val, dpi, 72, true);
    if (ok) {
        const double factor = m_action->dpiToScale(dpi);
        m_action->setFactor(factor);
    }
    m_action->updateOptions();
}

void LC_ImageOptionsWidget::onFactorEditingFinished() {
    const auto val = ui->leFactor->text();
    double factor;
    const bool ok = toDouble(val, factor, 1.0, true);
    if (ok) {
        m_action->setFactor(factor);
    }
    m_action->updateOptions();
}
