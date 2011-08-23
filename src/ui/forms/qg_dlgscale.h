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
#ifndef QG_DLGSCALE_H
#define QG_DLGSCALE_H

#include "ui_qg_dlgscale.h"

class RS_ScaleData;

class QG_DlgScale : public QDialog, public Ui::QG_DlgScale
{
    Q_OBJECT

public:
    QG_DlgScale(QWidget* parent = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgScale();

public slots:
    virtual void setData( RS_ScaleData * d );
    virtual void updateData();

protected slots:
    virtual void languageChange();

private slots:

    void on_cbIsotropic_toggled(bool checked);

    void on_leFactorX_textChanged(const QString &arg1);
    void on_leFactorY_textChanged(const QString &arg1);

private:
    QString scaleFactorX;
    QString scaleFactorY;
    RS_ScaleData* data;
    QString copies;
    bool isotropic;
    int numberMode;
    bool useCurrentLayer;
    bool useCurrentAttributes;

    void init();
    void destroy();

};

#endif // QG_DLGSCALE_H
