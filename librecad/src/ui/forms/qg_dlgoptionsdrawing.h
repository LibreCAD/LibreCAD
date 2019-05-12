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

#include<memory>
#include <QGraphicsScene>
#include "ui_qg_dlgoptionsdrawing.h"

class RS_Graphic;
class RS_Vector;

class QG_DlgOptionsDrawing : public QDialog, public Ui::QG_DlgOptionsDrawing
{
    Q_OBJECT

public:
    QG_DlgOptionsDrawing(QWidget* parent = 0, bool modal = false, Qt::WindowFlags fl = 0);
	~QG_DlgOptionsDrawing();

public slots:
    virtual void setGraphic( RS_Graphic * g );
    virtual void validate();
    virtual void updateLengthPrecision();
    virtual void updateAnglePrecision();
    virtual void updatePreview();
    virtual void updatePaperSize();
    virtual void updateUnitLabels();
    virtual void updateDimLengthPrecision();
    virtual void updateDimAnglePrecision();
    virtual void updatePaperPreview();

protected slots:
    virtual void languageChange();

private slots:
    void on_rbIsometricGrid_clicked();

    void on_rbCrosshairLeft_toggled(bool checked);

    void on_rbCrosshairTop_toggled(bool checked);

    void on_rbCrosshairRight_toggled(bool checked);

    void on_rbOrthogonalGrid_clicked();

    void on_cbGridOn_toggled(bool checked);

	void on_rbLandscape_toggled(bool checked);

    void on_cbDimFxLon_toggled(bool checked);

    void on_tabWidget_currentChanged(int index);

private:
    void updateCBLengthPrecision(QComboBox* u, QComboBox* l);
    void updateCBAnglePrecision(QComboBox* u, QComboBox* p);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    QStringList listPrec1;
    RS_Graphic* graphic;
    QGraphicsScene* paperScene;
	std::unique_ptr<RS_Vector> spacing;
    void init();
};

#endif // QG_DLGOPTIONSDRAWING_H
