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
#ifndef QG_DLGOPTIONSDRAWING_H
#define QG_DLGOPTIONSDRAWING_H

#include "ui_qg_dlgoptionsdrawing.h"
#include "lc_dialog.h"

class RS_Graphic;
class RS_Vector;

class QG_DlgOptionsDrawing : public LC_Dialog, public Ui::QG_DlgOptionsDrawing{
    Q_OBJECT
public:
    QG_DlgOptionsDrawing(QWidget* parent = nullptr);
	~QG_DlgOptionsDrawing() override;
    void showInitialTab(int tabIndex);
    void setGraphic( RS_Graphic * g );
protected slots:
    void languageChange();
    void validate();
    void updateLengthPrecision();
    void updateAnglePrecision();
    void updatePreview();
    void updatePaperSize();
    void updateUnitLabels();
    void updateDimLengthPrecision();
    void updateDimAnglePrecision();
    void updatePaperPreview();
    void on_cbGridOn_toggled(bool checked);
	void onLandscapeToggled(bool checked);
    void onDimFxLonToggled(bool checked);
    void onRelSizeToggled(bool checked);
    void disableXSpacing(bool checked);
    void enableXSpacing(bool checked);
protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void updateCBLengthPrecision(QComboBox* u, QComboBox* l);
    void updateCBAnglePrecision(QComboBox* u, QComboBox* p);
    void updateLPtSzUnits();
private:
    std::unique_ptr<QStringList> m_listPrec1;
    RS_Graphic* m_graphic;
    QGraphicsScene* m_paperScene;
    std::unique_ptr<RS_Vector> m_spacing;
    void init();
    void initVariables();
};

#endif // QG_DLGOPTIONSDRAWING_H
