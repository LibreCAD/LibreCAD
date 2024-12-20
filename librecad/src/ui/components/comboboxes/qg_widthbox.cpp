/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010-2011 R. van Twisk (librecad@rvt.dds.nl)
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

#include <tuple>

#include "qg_widthbox.h"

#include "rs_debug.h"

namespace {
std::tuple<QString, QString, RS2::LineWidth> g_boxItems[] = {
    {":ui/width00.png", QObject::tr("-Unchanged-"),
     RS2::WidthUnchanged /*utilitytypefornotchangedlinewidthduringediting*/},
    {":ui/width00.png", QObject::tr("ByLayer"),
     RS2::WidthByLayer /**<Linewidthdefinedbylayernotentity.*/},
    {":ui/width00.png", QObject::tr("ByBlock"),
     RS2::WidthByBlock /**<Linewidthdefinedbyblocknotentity.*/},
    {":ui/width01.png", QObject::tr("Default"),
     RS2::WidthDefault /**<Linewidthdefaultstothepredefinedlinewidth.*/},
    {":ui/width01.png", QObject::tr("0.00mm"),
     RS2::Width00 /**<Width1.(0.00mm)*/},
    {":ui/width01.png", QObject::tr("0.05mm"),
     RS2::Width01 /**<Width2.(0.05mm)*/},
    {":ui/width01.png", QObject::tr("0.09mm"),
     RS2::Width02 /**<Width3.(0.09mm)*/},
    {":ui/width01.png", QObject::tr("0.13mmISO"),
     RS2::Width03 /**<Width4.(0.13mm)*/},
    {":ui/width01.png", QObject::tr("0.15mm"),
     RS2::Width04 /**<Width5.(0.15mm)*/},
    {":ui/width01.png", QObject::tr("0.18mmISO"),
     RS2::Width05 /**<Width6.(0.18mm)*/},
    {":ui/width01.png", QObject::tr("0.20mm"),
     RS2::Width06 /**<Width7.(0.20mm)*/},
    {":ui/width01.png", QObject::tr("0.25mmISO"),
     RS2::Width07 /**<Width8.(0.25mm)*/},
    {":ui/width01.png", QObject::tr("0.30mm"),
     RS2::Width08 /**<Width9.(0.30mm)*/},
    {":ui/width03.png", QObject::tr("0.35mmISO"),
     RS2::Width09 /**<Width10.(0.35mm)*/},
    {":ui/width03.png", QObject::tr("0.40mm"),
     RS2::Width10 /**<Width11.(0.40mm)*/},
    {":ui/width04.png", QObject::tr("0.50mmISO"),
     RS2::Width11 /**<Width12.(0.50mm)*/},
    {":ui/width05.png", QObject::tr("0.53mm"),
     RS2::Width12 /**<Width13.(0.53mm)*/},
    {":ui/width05.png", QObject::tr("0.60mm"),
     RS2::Width13 /**<Width14.(0.60mm)*/},
    {":ui/width06.png", QObject::tr("0.70mmISO"),
     RS2::Width14 /**<Width15.(0.70mm)*/},
    {":ui/width07.png", QObject::tr("0.80mm"),
     RS2::Width15 /**<Width16.(0.80mm)*/},
    {":ui/width08.png", QObject::tr("0.90mm"),
     RS2::Width16 /**<Width17.(0.90mm)*/},
    {":ui/width09.png", QObject::tr("1.00mmISO"),
     RS2::Width17 /**<Width18.(1.00mm)*/},
    {":ui/width10.png", QObject::tr("1.06mm"),
     RS2::Width18 /**<Width19.(1.06mm)*/},
    {":ui/width10.png", QObject::tr("1.20mm"),
     RS2::Width19 /**<Width20.(1.20mm)*/},
    {":ui/width12.png", QObject::tr("1.40mmISO"),
     RS2::Width20 /**<Width21.(1.40mm)*/},
    {":ui/width12.png", QObject::tr("1.58mm"),
     RS2::Width21 /**<Width22.(1.58mm)*/},
    {":ui/width12.png", QObject::tr("2.00mmISO"),
     RS2::Width22 /**<Width23.(2.00mm)*/},
    {":ui/width12.png", QObject::tr("2.11mm"),
     RS2::Width23 /**<Width24.(2.11mm)*/}
};
}
/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_WidthBox::QG_WidthBox(QWidget* parent, const char* name)
	: QComboBox(parent)
	,showByLayer(false)
	,showUnchanged(false)
	,unchanged(false)
{
	setObjectName(name);
}

/**
 * Constructor that calls init and provides a fully functional 
 * combobox for choosing widths.
 *
 * @param showByLayer true: Show attributes ByLayer, ByBlock
 */
QG_WidthBox::QG_WidthBox(bool showByLayer, bool showUnchanged,
                         QWidget* parent, const char* name)
        : QComboBox(parent) {
    setObjectName(name);
    init(showByLayer, showUnchanged);
}

RS2::LineWidth QG_WidthBox::getWidth() const{
    return currentWidth;
}

bool QG_WidthBox::isUnchanged() const{
    return unchanged;
}

/**
 * Initialisation (called from constructor or manually but only
 * once).
 *
 * @param showByLayer true: Show attributes ByLayer, ByBlock
 */
void QG_WidthBox::init(bool showByLayer, bool showUnchanged) {
    this->showByLayer = showByLayer;
	this->showUnchanged = showUnchanged;

    for(const auto& [icon, text, lineWidth]: g_boxItems) {
        switch (lineWidth) {
        case RS2::WidthUnchanged:
            if (!showUnchanged)
                continue;
            break;
        case RS2::WidthByLayer:
        case RS2::WidthByBlock:
            if (!showByLayer)
                continue;
            break;
        default:
            break;
        }
        addItem(QIcon(icon), text, lineWidth);
        m_width2Index.emplace(lineWidth, count() - 1);
    }

    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotWidthChanged(int)));

    setCurrentIndex(0);
    slotWidthChanged(currentIndex());
}

/**
 * Sets the currently selected width item to the given width.
 */
void QG_WidthBox::setWidth(RS2::LineWidth w) {

    RS_DEBUG->print("QG_WidthBox::setWidth %d\n", (int)w);

    auto it = m_width2Index.find(w);
    if (it == m_width2Index.end()) {
        LC_ERR<<"QG_WidthBox::"<<__func__<<"(): error: unknown LineWidth="<<w<<" : ignored";
        return;
    }

    if (it->second == currentIndex())
        return;

    setCurrentIndex(it->second);

    slotWidthChanged(currentIndex());
}



/**
 * Sets the pixmap showing the width of the "By Layer" item.
 */
void QG_WidthBox::setLayerWidth(RS2::LineWidth w) {
    if (showByLayer) {
        QIcon pixmap;
        switch(w) {
        default:
        case RS2::Width00:
            pixmap = QPixmap(":ui/width00.png");
            break;
        case RS2::Width01:
        case RS2::Width02:
            pixmap = QPixmap(":ui/width01.png");
            break;
        case RS2::Width03:
        case RS2::Width04:
            pixmap = QPixmap(":ui/width02.png");
            break;
        case RS2::Width05:
        case RS2::Width06:
            pixmap = QPixmap(":ui/width03.png");
            break;
        case RS2::Width07:
        case RS2::Width08:
            pixmap = QPixmap(":ui/width04.png");
            break;
        case RS2::Width09:
        case RS2::Width10:
            pixmap = QPixmap(":ui/width05.png");
            break;
        case RS2::Width11:
        case RS2::Width12:
            pixmap = QPixmap(":ui/width06.png");
            break;
        case RS2::Width13:
        case RS2::Width14:
            pixmap = QPixmap(":ui/width07.png");
            break;
        case RS2::Width15:
        case RS2::Width16:
            pixmap = QPixmap(":ui/width08.png");
            break;
        case RS2::Width17:
        case RS2::Width18:
            pixmap = QPixmap(":ui/width09.png");
            break;
        case RS2::Width19:
        case RS2::Width20:
            pixmap = QPixmap(":ui/width10.png");
            break;
        case RS2::Width21:
        case RS2::Width22:
            pixmap = QPixmap(":ui/width11.png");
            break;
        case RS2::Width23:
            //case RS2::Width24:
            pixmap = QPixmap(":ui/width12.png");
            break;
        }

        setItemIcon(0, pixmap);
        setItemText(0, tr("By Layer"));

        // needed for the first time a layer is added:
        slotWidthChanged(currentIndex());
    }
}

/**
 * Called when the width has changed. This method 
 * sets the current width to the value chosen or even
 * offers a dialog to the user that allows him/ her to
 * choose an individual width.
 */
void QG_WidthBox::slotWidthChanged(int index) {

    RS_DEBUG->print("QG_WidthBox::slotWidthChanged %d\n", index);


    if (showUnchanged && index == 0) {
        unchanged = true;
    } else {
        unchanged = false;
        currentWidth = static_cast<RS2::LineWidth>(itemData(index).toInt());
    }

    RS_DEBUG->print("Current width is (%d): %d\n",
                    index, ((int)currentWidth));

    emit widthChanged(currentWidth);
}