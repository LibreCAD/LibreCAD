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
#ifndef QG_DLGFONTCONVERSION_H
#define QG_DLGFONTCONVERSION_H

#include <QDialog>
#include <QAbstractButton>
#include "rs_font.h"
#include "rs_entitycontainer.h"
#include "lc_fontcreation.h"

namespace Ui {
class QG_DlgFontConversion;
}

class QG_DlgFontConversion : public QDialog
{
    Q_OBJECT

public:
    explicit QG_DlgFontConversion(QWidget *parent = nullptr);
    ~QG_DlgFontConversion();

public slots:
	virtual void showEvent(QShowEvent * e);
	virtual void resizeEvent(QResizeEvent *);

protected slots:
	virtual void slotButtonClicked(QAbstractButton* b);
	virtual void slotFontChanged(QFont);
	virtual void slotFontFileChanged();
	virtual void slotFontFileClicked();
	virtual void slotFontSpacingChanged();

	virtual void slotSaveAsChanged();
	virtual void slotSaveAsClicked();
	virtual void slotWritingSystemChanged();

protected slots:
	virtual void languageChange();

private: //helpers
	void addFont(const QString& lff);
	void createFont(const QString& lff);
	void enableApply(bool enable);
	void updatePreview(bool resizing = false);
	void setWritingSystems();
	QString getSaveAsFileName();
	QString getTempFileName();
	QString getSampleText();

private:
    Ui::QG_DlgFontConversion *ui;
	LC_FontCreation creation;
	QString fontFolder;
	bool dirty;
	RS_EntityContainer* preview;
	RS_Font* font; // the previewed font
};

#endif // QG_DLGFONTCONVERSION_H
