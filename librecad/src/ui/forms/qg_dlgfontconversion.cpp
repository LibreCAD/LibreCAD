/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)
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
#include <QPushButton>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFontDatabase>
#include <QProcess>
#include <QMessageBox>
#include "qg_dlgfontconversion.h"
#include "ui_qg_dlgfontconversion.h"
#include "rs_settings.h"
#include "rs_mtext.h"
#include "rs_fontlist.h"
#include "rs_system.h"
#include <cstdlib>
#include <math.h>
#include "qc_applicationwindow.h"
#include "rs_document.h"
#include "lc_telemetry.h"
#include "lc_helpbrowser.h"

QG_DlgFontConversion::QG_DlgFontConversion(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QG_DlgFontConversion)
{
    ui->setupUi(this);

	RS_SETTINGS->beginGroup("/Paths");	
	fontFolder = RS_SETTINGS->readEntry("/Fonts", "").trimmed();
	RS_SETTINGS->endGroup();

	dirty = false;	
	font = nullptr;
	preview = new RS_EntityContainer();
	ui->gvPreview->setContainer(preview);

	ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Create"));

	RS_Document* doc = QC_ApplicationWindow::getAppWindow()->getDocument();
	if (doc)
		doc->getTelemetryData().fontConversionClicks++;

	installEventFilter(LC_HELP);

	LC_HELP->associateTopic(this, "topic_guide_convertFonts");

	slotFontChanged(QFont());
}

QG_DlgFontConversion::~QG_DlgFontConversion()
{
	if (font)
		RS_FONTLIST->deleteFont(font);
	delete preview;
    delete ui;
}

void QG_DlgFontConversion::languageChange()
{
	ui->retranslateUi(this);
}

void QG_DlgFontConversion::resizeEvent(QResizeEvent *)
{
	updatePreview(true);
}

void QG_DlgFontConversion::showEvent(QShowEvent * e)
{
	QDialog::showEvent(e);
	ui->gvPreview->zoomAuto();
}

void QG_DlgFontConversion::slotButtonClicked(QAbstractButton* b)
{
	QDialogButtonBox::StandardButton bt = ui->buttonBox->standardButton(b);
	switch (bt) {
	case QDialogButtonBox::Ok:
		if (dirty) {
			createFont(ui->txtSaveAs->text());
			addFont(ui->txtSaveAs->text());
		}
		emit accept();
		break;
	case QDialogButtonBox::Apply:
		createFont(ui->txtSaveAs->text());
		addFont(ui->txtSaveAs->text());
		enableApply(false);
		break;
	case QDialogButtonBox::Cancel:	
	default:
		emit reject();
		break;
	}
}

void QG_DlgFontConversion::slotFontSpacingChanged()
{
	updatePreview();
	enableApply(true);
}

void QG_DlgFontConversion::slotFontChanged(QFont)
{
	QString searching = tr(" - Searching");
	QApplication::setOverrideCursor(Qt::WaitCursor);
	setWindowTitle(windowTitle() + searching);

	ui->txtFontName->setText(creation.getFontFileName(ui->cbFontFamily->currentFont().family()));
	ui->txtSaveAs->setText(getSaveAsFileName());
	setWritingSystems();

	setWindowTitle(windowTitle().chopped(windowTitle().size() - searching.size()));
	QApplication::restoreOverrideCursor();
	if (!ui->txtFontName->text().isEmpty())
		updatePreview();
	else {
		preview->clear();
		ui->gvPreview->redraw();
	}
	enableApply(true);
}

void QG_DlgFontConversion::slotFontFileChanged()
{
	//ui->txtSaveAs->setText(getSaveAsFileName());
	enableApply(true);
}

void QG_DlgFontConversion::slotFontFileClicked()
{
	QFileDialog dlg(this);
	QStringList filters;
	QFileInfo file;
	dlg.setFileMode(QFileDialog::ExistingFile);
	dlg.setViewMode(QFileDialog::List);
	dlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::FontsLocation));	
	filters << "All Font Files(*.ttf, *.shp)"
		<< "TrueType Font Files(*.ttf)"
		<< "AutoCAD Font Files(*.shp)";
	dlg.setNameFilters(filters);

	if (dlg.exec()) {
		file.setFile(dlg.selectedFiles()[0]);
		QFileInfo lff(QDir(fontFolder), QString("%1.lff").arg(file.baseName()));
		if (!(file.suffix().toLower() == "shp")) {
			int id = QFontDatabase::addApplicationFont(dlg.selectedFiles()[0]);
			lff.setFile(QDir(fontFolder), QString("%1.%2").arg(QFontDatabase::applicationFontFamilies(id)[0], "lff"));
			QFontDatabase::removeApplicationFont(id);
		}

		ui->txtFontName->setText(dlg.selectedFiles()[0]);
		ui->txtSaveAs->setText(lff.filePath());
		setWritingSystems();
		updatePreview();
	}		
}

void QG_DlgFontConversion::slotSaveAsChanged()
{
	enableApply(true);
}

void QG_DlgFontConversion::slotSaveAsClicked()
{
	QFileDialog dlg(this);
	dlg.setAcceptMode(QFileDialog::AcceptSave);
	dlg.setNameFilter(tr("LibreCAD Font Files(*.lff)"));
	dlg.setDefaultSuffix("lff");
	dlg.setDirectory(fontFolder);

	if (dlg.exec())
		ui->txtSaveAs->setText(dlg.selectedFiles()[0]);
}

void QG_DlgFontConversion::slotWritingSystemChanged()
{
	updatePreview(true);
}

void QG_DlgFontConversion::updatePreview(bool resizing)
{
	if (preview == nullptr) {
		return;
	}
	if (!resizing)
		createFont(getTempFileName());
	if (font) 
		RS_FONTLIST->deleteFont(font);
	font = new RS_Font(getTempFileName(), true);
	if (font->loadFont()) {
		RS_FONTLIST->addFont(font);

		RS_Pen p = RS_Pen(RS_Color(255, 255, 255), RS2::WidthDefault, RS2::SolidLine);
		QString text = getSampleText();
		RS_MTextData d(RS_Vector(0, 0, 0), 1.0, 1.0,
			RS_MTextData::VABottom,
			RS_MTextData::HALeft,
			RS_MTextData::ByStyle,
			RS_MTextData::Exact,
			ui->sbLine->value(),
			text, QString("preview"), 0.0,
			RS2::NoUpdate);
		RS_MText* mtext = new RS_MText(preview, d);
		mtext->setLayer("0");
		mtext->setPen(p);
		mtext->update();

		preview->clear();
		preview->setPen(mtext->getPen());
		preview->addEntity(mtext);
		preview->calculateBorders();
		ui->gvPreview->zoomAuto();
	} else {
		preview->clear();
	}
}

void QG_DlgFontConversion::setWritingSystems()
{
	QFontDatabase db;
	ui->cbWritingSystem->clear();
	int id = db.addApplicationFont(ui->txtFontName->text());
	for (auto system : db.writingSystems(ui->cbFontFamily->currentFont().family())) {
		ui->cbWritingSystem->addItem(db.writingSystemName(system), system);
	}
	db.removeApplicationFont(id);
}

void QG_DlgFontConversion::addFont(const QString & lff)
{
	if (!lff.isEmpty()) {
		QFileInfo file(lff);
		RS_Font* f = RS_FONTLIST->requestFont(file.baseName());
		if (f != nullptr)
			RS_FONTLIST->deleteFont(f);
		RS_FONTLIST->addFont(new RS_Font(lff, true));
	}	
}

void QG_DlgFontConversion::createFont(const QString & lff)
{
	if (!lff.isEmpty()) {
		QApplication::setOverrideCursor(Qt::WaitCursor);
		QString suffix = tr(" - Searching");		
		setWindowTitle(windowTitle() + suffix);
		//ui->txtFontName->setText(creation.getFontFileName(ui->cbFontFamily->currentFont().family()));
		setWindowTitle(windowTitle().chopped(windowTitle().size() - suffix.size()));

		suffix = tr(" - Rendering");
		setWindowTitle(windowTitle() + suffix);
		creation.createFont(
			ui->txtFontName->text(),
			lff,
			ui->txtAuthor->text(),
			ui->txtLicense->text(),
			ui->sbLetter->value(),
			ui->sbWord->value(),
			ui->sbLine->value()
		);
		setWindowTitle(windowTitle().chopped(windowTitle().size() - suffix.size()));
		QApplication::restoreOverrideCursor();
	}
}

void QG_DlgFontConversion::enableApply(bool enable)
{
	QFileInfo file(ui->txtSaveAs->text());
	bool fileOk = file.dir().exists() && !file.baseName().isEmpty();
	ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(enable && fileOk);	
	dirty = enable;
}

QString QG_DlgFontConversion::getSaveAsFileName()
{
	if (ui->txtFontName->text().isEmpty() || ui->cbFontFamily->currentText().isEmpty())
		return "";
	QFileInfo result(QDir(fontFolder), QString("%1.%2").arg(ui->cbFontFamily->currentText(), "lff"));
	return result.filePath();
}

QString QG_DlgFontConversion::getTempFileName()
{
	QDir dir(QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::TempLocation)));
	QFileInfo result(dir, "preview.lff");
	QString res = result.filePath();
	return result.filePath();
}

QString QG_DlgFontConversion::getSampleText()
{
	QFontDatabase::WritingSystem ws = static_cast<QFontDatabase::WritingSystem>(ui->cbWritingSystem->currentData().toInt());
	if (ws != QFontDatabase::Latin)
		return QFontDatabase::writingSystemSample(ws);
	return QString("Lorem ipsum dolor sit amet,\n consectetur adipiscing elit");
}
