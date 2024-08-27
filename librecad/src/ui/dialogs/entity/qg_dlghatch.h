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
#ifndef QG_DLGHATCH_H
#define QG_DLGHATCH_H

#include <memory>

#include "ui_qg_dlghatch.h"
#include "lc_dialog.h"

class RS_Hatch;

class QG_DlgHatch : public LC_Dialog, public Ui::QG_DlgHatch{
    Q_OBJECT

public:
    QG_DlgHatch(QWidget* parent = nullptr);
    ~QG_DlgHatch() override;
    void saveSettings();

public slots:
    virtual void polish();
    void showEvent( QShowEvent * e ) override;
    virtual void setHatch( RS_Hatch & h, bool isNew );
    virtual void updateHatch();
    virtual void setPattern( const QString & p );
    void resizeEvent( QResizeEvent * ) override;
	virtual void updatePreview();

protected slots:
    virtual void languageChange();

private:
    void init();
    void showArea();
    std::unique_ptr<RS_EntityContainer> preview;
    std::shared_ptr<RS_Pattern> pattern;
    RS_Hatch* hatch = nullptr;
    bool isNew = false;


};

#endif // QG_DLGHATCH_H
