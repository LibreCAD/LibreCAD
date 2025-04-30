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
#ifndef QG_DLGTEXT_H
#define QG_DLGTEXT_H

#include "ui_qg_dlgtext.h"
#include "lc_entitypropertiesdlg.h"

class RS_Text;

class QG_DlgText : public LC_EntityPropertiesDlg, public Ui::QG_DlgText{
    Q_OBJECT
public:
    QG_DlgText(QWidget *parent, LC_GraphicViewport *pViewport, RS_Text* text, bool forNew);
    ~QG_DlgText() override;
    int getAlignment();
public slots:
     void updateUniCharComboBox( int );
     void updateEntity() override;
     void setAlignmentTL();
     void setAlignmentTC();
     void setAlignmentTR();
     void setAlignmentML();
     void setAlignmentMC();
     void setAlignmentMR();
     void setAlignmentLL();
     void setAlignmentLC();
     void setAlignmentLR();
     void setAlignmentBL();
     void setAlignmentBC();
     void setAlignmentBR();
     void setAlignmentFit();
     void setAlignmentAlign();
     void setAlignmentMiddle();
     void setAlignment( int a );
     void setFont( const QString & f );
//     void defaultChanged( bool );
     void loadText();
     void load( const QString & fn );
     void saveText();
     void save( const QString & fn );
     void insertSymbol( int );
     void updateUniCharButton( int );
     void insertChar();
    void reject() override;
protected slots:
    void languageChange();

protected:
    bool m_isNew;
    bool m_saveSettings;
    RS_Text* entity;
    RS_Font* font;

    void setEntity(RS_Text* t, bool isNew );
    void init();
    void destroy();
};

#endif // QG_DLGTEXT_H
