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

	slotFontChanged(QFont());
}

QG_DlgFontConversion::~QG_DlgFontConversion()
{
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
		createFont(ui->txtSaveAs->text());
		emit accept();
		break;
	case QDialogButtonBox::Apply:
		createFont(ui->txtSaveAs->text());
		dirty = false;
		enableApply();
		break;
	case QDialogButtonBox::Cancel:	
	default:
		emit reject();
		break;
	}
}

void QG_DlgFontConversion::slotFontSpacingChanged()
{
	dirty = true;
	updatePreview();
}

void QG_DlgFontConversion::slotFontChanged(QFont)
{
	dirty = true;
	QString searching = tr(" - Searching");
	QApplication::setOverrideCursor(Qt::WaitCursor);
	setWindowTitle(windowTitle() + searching);
	ui->txtFontName->setText(getFontFileName(ui->cbFontFamily->currentFont().family()));
	setWindowTitle(windowTitle().chopped(windowTitle().size() - searching.size()));
	QApplication::restoreOverrideCursor();
	if (!ui->txtFontName->text().isEmpty())
		updatePreview();
	else {
		preview->clear();
		ui->gvPreview->redraw();
	}
}

void QG_DlgFontConversion::slotFontFileChanged()
{
	dirty = true;
	ui->txtSaveAs->setText(getSaveAsFileName());
}

void QG_DlgFontConversion::slotFontFileClicked()
{
	QFileDialog dlg(this);
	dlg.setFileMode(QFileDialog::ExistingFile);
	dlg.setViewMode(QFileDialog::List);
	dlg.setNameFilter(tr("TrueType Font Files(*.ttf)"));
	dlg.setDefaultSuffix("ttf");	
	dlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::FontsLocation));

	if (dlg.exec()) {
		
		int id = QFontDatabase::addApplicationFont(dlg.selectedFiles()[0]);
		QFileInfo file(QDir(fontFolder), QString("%1.%2").arg(QFontDatabase::applicationFontFamilies(id)[0], "lff"));
		QFontDatabase::removeApplicationFont(id);
		ui->txtFontName->setText(dlg.selectedFiles()[0]);
		ui->txtSaveAs->setText(file.filePath());
		updatePreview();
	}		
}

void QG_DlgFontConversion::slotSaveAsChanged()
{
	enableApply();
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

void QG_DlgFontConversion::updatePreview(bool resizing)
{
	if (preview == nullptr) {
		return;
	}
	if (!resizing)
		createFont(getTempFileName());
	preview->clear();
		
	if (font) 
		RS_FONTLIST->deleteFont(font);
	font = new RS_Font(getTempFileName(), true);
	if (font->loadFont()) {
		RS_FONTLIST->addFont(font);

		RS_Pen p = RS_Pen(RS_Color(255, 255, 255), RS2::WidthDefault, RS2::SolidLine);
		QString text = "Lorem ipsum dolor sit amet,\n consectetur adipiscing elit";
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
		mtext->calculateBorders();

		preview->setPen(mtext->getPen());
		preview->addEntity(mtext);
		preview->calculateBorders();

		ui->gvPreview->zoomAuto();
		ui->gvPreview->redraw();
	} else {
		preview->clear();
		ui->gvPreview->zoomAuto();
	}
}

bool QG_DlgFontConversion::containsFamily(const QString & fontFile, const QString & family)
{
	int id = QFontDatabase::addApplicationFont(fontFile);
	for (auto fam : QFontDatabase::applicationFontFamilies(id)) {
		if (family.compare(fam, Qt::CaseInsensitive) == 0) {
			QFontDatabase::removeApplicationFont(id);
			return true;
		}
	}
	QFontDatabase::removeApplicationFont(id);
	return false;
}

void QG_DlgFontConversion::createFont(QString lff)
{
	QProcess proc(this);
	QFileInfo info = QFileInfo(QDir(QDir::cleanPath(QCoreApplication::applicationDirPath())), "ttf2lff.exe");
	QString command = info.filePath() +
		QString(" -l %1").arg(ui->sbLetter->value()) +
		QString(" -w %1").arg(ui->sbWord->value()) +
		QString(" -f %1").arg(ui->sbLine->value());

	if (!ui->txtAuthor->text().isEmpty())
		command += QString(" -a \"%1\"").arg(ui->txtAuthor->text());
	if (!ui->txtLicense->text().isEmpty())
		command += QString(" -L \"%1\"").arg(ui->txtLicense->text());

	command +=
		QString(" \"%1\"").arg(ui->txtFontName->text()) +
		QString(" \"%1\"").arg(lff);

	if (!lff.isEmpty()) {
		QString rendering = tr(" - Rendering");
		QApplication::setOverrideCursor(Qt::WaitCursor);
		setWindowTitle(windowTitle() + rendering);
		ui->txtFontName->setText(getFontFileName(ui->cbFontFamily->currentFont().family()));
		proc.start(command);
		proc.waitForFinished();
		setWindowTitle(windowTitle().chopped(windowTitle().size() - rendering.size()));
		QApplication::restoreOverrideCursor();
	}
}

void QG_DlgFontConversion::enableApply()
{
	QFileInfo file(ui->txtSaveAs->text());
	bool fileOk = file.dir().exists() && !file.baseName().isEmpty();
	ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(dirty && fileOk);	
}

QString QG_DlgFontConversion::getFontFileName(const QString & family, const QString & fontDir, QHash<QString, int>* checked)
{
	QString result = "";

	if (fontDir.isEmpty()) {
		QHash<QString, int> map;
		for (auto dirName : QStandardPaths::standardLocations(QStandardPaths::FontsLocation)) {
			result = getFontFileName(family, dirName, &map);
			if (!result.isEmpty())
				return result;
		}
	} else {
		QDir dir(fontDir);
		QString part = family.chopped(family.size()-3);
		checked->insert(fontDir, 1);
		// make a couple of guesses before scanning every font in the folder tree
		for (auto dirInfo : dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::System)) {
			QString path = dirInfo.filePath();
			if (path.contains(part, Qt::CaseInsensitive)) {
				checked->insert(path, 1);
				result = getFontFileName(family, path, checked);
				if (!result.isEmpty())
					return result;
			}
		}
		for (auto fileInfo : dir.entryInfoList(QDir::Files)) {
			QString filePath = fileInfo.filePath();
			if (fileInfo.baseName().contains(part, Qt::CaseInsensitive)) {
				checked->insert(filePath, 1);
				if (containsFamily(filePath, family))
					return filePath;
			}
		}

		// brute force
		for (auto fileInfo : dir.entryInfoList(QDir::Files)) {
			QString filePath = fileInfo.filePath();
			if (!checked->contains(filePath)) {
				if (containsFamily(filePath, family))
					return filePath;
			}
		}
		for (auto dirInfo : dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::System)) {
			QString path = dirInfo.filePath();
			if (!checked->contains(path)) {
				result = getFontFileName(family, path, checked);
				if (!result.isEmpty())
					return result;
			}
		}
	}

	return result;
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
	QDir dir(QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)));
	QFileInfo result(dir, "preview.lff");
	QString res = result.filePath();
	return result.filePath();
}
