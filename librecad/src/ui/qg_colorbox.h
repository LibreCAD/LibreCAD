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

#ifndef QG_COLORBOX_H
#define QG_COLORBOX_H

#include <QComboBox>
#include <memory>

class RS_Color;

/**
 * A combobox for choosing a color.
 */
class QG_ColorBox: public QComboBox {
    Q_OBJECT

public:
    QG_ColorBox(QWidget* parent=nullptr, const char* name=nullptr);
    QG_ColorBox(bool showByLayer, bool showUnchanged,
                QWidget* parent=nullptr, const char* name=nullptr);
    virtual ~QG_ColorBox();

    RS_Color getColor() const;

    void addColor(Qt::GlobalColor color, QString text);
    void setColor(const RS_Color& color);
    void setLayerColor(const RS_Color& color);

    void init(bool showByLayer, bool showUnchanged);

    bool isUnchanged() const;

private slots:
    void slotColorChanged(int index);

signals:
    void colorChanged(const RS_Color& color);

private:
    std::unique_ptr<RS_Color> currentColor;
    int colorIndexStart;
    bool showByLayer;
	bool showUnchanged;
	bool unchanged;
};

#endif

