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
#include "qg_dlghatch.h"

#include "rs_settings.h"
#include "rs_line.h"
#include "rs_hatch.h"
#include "rs_patternlist.h"
#include "rs_pattern.h"
#include "rs_math.h"
#include "rs_math.h"

/*
 *  Constructs a QG_DlgHatch as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgHatch::QG_DlgHatch(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    init();
}


/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgHatch::languageChange()
{
    retranslateUi(this);
}

void QG_DlgHatch::init() {
	pattern=nullptr;
	hatch = nullptr;
    isNew = false;

    preview = new RS_EntityContainer();
    gvPreview->setContainer(preview);
    gvPreview->setBorders(15,15,15,15);
    gvPreview->addScrollbars();

    cbPattern->init();

}

void QG_DlgHatch::polish() {
    QDialog::ensurePolished();
    gvPreview->zoomAuto();
}

void QG_DlgHatch::showEvent ( QShowEvent * e) {
    QDialog::showEvent(e);
    gvPreview->zoomAuto();
}

QG_DlgHatch::~QG_DlgHatch()
{
    delete preview;
}

void QG_DlgHatch::setHatch(RS_Hatch& h, bool isNew) {
    hatch = &h;
    this->isNew = isNew;

    RS_SETTINGS->beginGroup("/Draw");
    QString enablePrev = RS_SETTINGS->readEntry("/HatchPreview", "0");
    RS_SETTINGS->endGroup();

    cbEnablePreview->setChecked(enablePrev=="1");

    // read defaults from config file:
    if (isNew) {
        RS_SETTINGS->beginGroup("/Draw");
        QString solid = RS_SETTINGS->readEntry("/HatchSolid", "0");
        QString pat = RS_SETTINGS->readEntry("/HatchPattern", "ANSI31");
        QString scale = RS_SETTINGS->readEntry("/HatchScale", "1.0");
        QString angle = RS_SETTINGS->readEntry("/HatchAngle", "0.0");
        RS_SETTINGS->endGroup();

        cbSolid->setChecked(solid=="1");
        setPattern(pat);
        leScale->setText(scale);
        leAngle->setText(angle);
    }
    // initialize dialog based on given hatch:
    else {
        cbSolid->setChecked(hatch->isSolid());
        setPattern(hatch->getPattern());
        QString s;
        s.setNum(hatch->getScale());
        leScale->setText(s);
        s.setNum(RS_Math::rad2deg(hatch->getAngle()));
        leAngle->setText(s);
    }
}

void QG_DlgHatch::updateHatch() {
    if (hatch) {
        hatch->setSolid(cbSolid->isChecked());
        hatch->setPattern(cbPattern->currentText());
        hatch->setScale(RS_Math::eval(leScale->text()));
        hatch->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
    }
}

void QG_DlgHatch::setPattern(const QString& p) {
    if (!RS_PATTERNLIST->contains(p)) {
        cbPattern->addItem(p);
    }
    cbPattern->setCurrentIndex( cbPattern->findText(p) );
    pattern = cbPattern->getPattern();
}

void QG_DlgHatch::resizeEvent ( QResizeEvent * ) {
	updatePreview();
}

void QG_DlgHatch::updatePreview() {
	if (preview==nullptr) {
        return;
    }
	if (hatch==nullptr || !cbEnablePreview->isChecked()) {
        preview->clear();
        gvPreview->zoomAuto();
        return;
    }
	pattern = cbPattern->getPattern();
	if (pattern->countDeep()==0)
		return;

    QString patName = cbPattern->currentText();
    bool isSolid = cbSolid->isChecked();
    double scale = RS_Math::eval(leScale->text(), 1.0);
    double angle = RS_Math::deg2rad(RS_Math::eval(leAngle->text(), 0.0));
	double prevSize = 100.0;
    if (pattern) {
        pattern->calculateBorders();
		prevSize = std::max(prevSize, pattern->getSize().magnitude());
	}

    preview->clear();

    RS_Hatch* prevHatch = new RS_Hatch(preview,
                                       RS_HatchData(isSolid, scale, angle, patName));
    prevHatch->setPen(hatch->getPen());

    RS_EntityContainer* loop = new RS_EntityContainer(prevHatch);
    loop->setPen(RS_Pen(RS2::FlagInvalid));
	loop->addRectangle({0., 0.}, {prevSize,prevSize});
    prevHatch->addEntity(loop);
    preview->addEntity(prevHatch);
    if (!isSolid) {
        prevHatch->update();
    }

    gvPreview->zoomAuto();
}

void QG_DlgHatch::saveSettings()
{
    if (isNew)
    {
        RS_SETTINGS->beginGroup("/Draw");
        RS_SETTINGS->writeEntry("/HatchSolid", cbSolid->isChecked());
        RS_SETTINGS->writeEntry("/HatchPattern", cbPattern->currentText());
        RS_SETTINGS->writeEntry("/HatchScale", leScale->text());
        RS_SETTINGS->writeEntry("/HatchAngle", leAngle->text());
        RS_SETTINGS->writeEntry("/HatchPreview", cbEnablePreview->isChecked());
        RS_SETTINGS->endGroup();
    }
}
