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

#ifndef QG_LINETYPEBOX_H
#define QG_LINETYPEBOX_H

#include <QComboBox>

#include "rs.h"

/**
 * A combobox for choosing a line type.
 */
class QG_LineTypeBox: public QComboBox {
    Q_OBJECT

public:
    QG_LineTypeBox(QWidget* parent=0);
    QG_LineTypeBox(bool showByLayer, bool showUnchanged, 
		QWidget* parent=0, const char* name=0);
    virtual ~QG_LineTypeBox();

    RS2::LineType getLineType() {
        return currentLineType;
    }
    void setLineType(RS2::LineType w);
    void setLayerLineType(RS2::LineType w);

    void init(bool showByLayer, bool showUnchanged);
	
	bool isUnchanged() {
		return unchanged;
	}

private slots:
    void slotLineTypeChanged(int index);

signals:
    void lineTypeChanged(RS2::LineType);

private:
    RS2::LineType currentLineType;
    bool showByLayer;
	bool showUnchanged;
	bool unchanged;
};

#endif

