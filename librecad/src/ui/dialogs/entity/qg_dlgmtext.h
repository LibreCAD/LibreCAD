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
#ifndef QG_DLGMTEXT_H
#define QG_DLGMTEXT_H

#include "lc_entitypropertiesdlg.h"
#include "rs_mtext.h"
#include "ui_qg_dlgmtext.h"

class QG_DlgMText : public LC_EntityPropertiesDlg, public Ui::QG_DlgMText{
    Q_OBJECT
public:
    QG_DlgMText(QWidget *parent, LC_GraphicViewport *pViewport, RS_MText* mtext, bool forNew);
    ~QG_DlgMText() override;
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
     void setAlignmentBL();
     void setAlignmentBC();
     void setAlignmentBR();
     void setFont( const QString & f );
     void defaultChanged( bool );
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
    void setAlignment(QToolButton& button);
    void layoutDirectionChanged();
    bool m_isNew = false;
    bool m_saveSettings = true;
    RS_MText* m_entity = nullptr;
    RS_Font* m_font = nullptr;
    std::vector<QToolButton*> m_alignmentButtons;

    void init();
    void destroy();
    bool eventFilter(QObject *obj, QEvent *event) override;
    size_t alignmentButtonIdex(QToolButton* button) const;
    void setEntity(RS_MText *t, bool isNew );
};

#endif // QG_DLGMTEXT_H
