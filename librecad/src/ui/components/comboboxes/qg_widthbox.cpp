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
    {":linetypes/width00.lci", QObject::tr("-Unchanged-"),
     RS2::WidthUnchanged /*utilitytypefornotchangedlinewidthduringediting*/},
    {":linetypes/width00.lci", QObject::tr("By Layer"),
     RS2::WidthByLayer /**<Linewidthdefinedbylayernotentity.*/},
    {":linetypes/width00.lci", QObject::tr("By Block"),
     RS2::WidthByBlock /**<Linewidthdefinedbyblocknotentity.*/},
    {":linetypes/width01.lci", QObject::tr("Default"),
     RS2::WidthDefault /**<Linewidthdefaultstothepredefinedlinewidth.*/},
    {":linetypes/width01.lci", QObject::tr("0.00mm"),
     RS2::Width00 /**<Width1.(0.00mm)*/},
    {":linetypes/width01.lci", QObject::tr("0.05mm"),
     RS2::Width01 /**<Width2.(0.05mm)*/},
    {":linetypes/width01.lci", QObject::tr("0.09mm"),
     RS2::Width02 /**<Width3.(0.09mm)*/},
    {":linetypes/width01.lci", QObject::tr("0.13mmISO"),
     RS2::Width03 /**<Width4.(0.13mm)*/},
    {":linetypes/width01.lci", QObject::tr("0.15mm"),
     RS2::Width04 /**<Width5.(0.15mm)*/},
    {":linetypes/width01.lci", QObject::tr("0.18mmISO"),
     RS2::Width05 /**<Width6.(0.18mm)*/},
    {":linetypes/width01.lci", QObject::tr("0.20mm"),
     RS2::Width06 /**<Width7.(0.20mm)*/},
    {":linetypes/width01.lci", QObject::tr("0.25mmISO"),
     RS2::Width07 /**<Width8.(0.25mm)*/},
    {":linetypes/width01.lci", QObject::tr("0.30mm"),
     RS2::Width08 /**<Width9.(0.30mm)*/},
    {":linetypes/width03.lci", QObject::tr("0.35mmISO"),
     RS2::Width09 /**<Width10.(0.35mm)*/},
    {":linetypes/width03.lci", QObject::tr("0.40mm"),
     RS2::Width10 /**<Width11.(0.40mm)*/},
    {":linetypes/width04.lci", QObject::tr("0.50mmISO"),
     RS2::Width11 /**<Width12.(0.50mm)*/},
    {":linetypes/width05.lci", QObject::tr("0.53mm"),
     RS2::Width12 /**<Width13.(0.53mm)*/},
    {":linetypes/width05.lci", QObject::tr("0.60mm"),
     RS2::Width13 /**<Width14.(0.60mm)*/},
    {":linetypes/width06.lci", QObject::tr("0.70mmISO"),
     RS2::Width14 /**<Width15.(0.70mm)*/},
    {":linetypes/width07.lci", QObject::tr("0.80mm"),
     RS2::Width15 /**<Width16.(0.80mm)*/},
    {":linetypes/width08.lci", QObject::tr("0.90mm"),
     RS2::Width16 /**<Width17.(0.90mm)*/},
    {":linetypes/width09.lci", QObject::tr("1.00mmISO"),
     RS2::Width17 /**<Width18.(1.00mm)*/},
    {":linetypes/width10.lci", QObject::tr("1.06mm"),
     RS2::Width18 /**<Width19.(1.06mm)*/},
    {":linetypes/width10.lci", QObject::tr("1.20mm"),
     RS2::Width19 /**<Width20.(1.20mm)*/},
    {":linetypes/width12.lci", QObject::tr("1.40mmISO"),
     RS2::Width20 /**<Width21.(1.40mm)*/},
    {":linetypes/width12.lci", QObject::tr("1.58mm"),
     RS2::Width21 /**<Width22.(1.58mm)*/},
    {":linetypes/width12.lci", QObject::tr("2.00mmISO"),
     RS2::Width22 /**<Width23.(2.00mm)*/},
    {":linetypes/width12.lci", QObject::tr("2.11mm"),
     RS2::Width23 /**<Width24.(2.11mm)*/}
};
}
/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_WidthBox::QG_WidthBox(QWidget* parent, const char* name)
	: QComboBox(parent)
	,m_showByLayer(false)
	,m_showUnchanged(false)
	,m_unchanged(false)
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
    return m_currentWidth;
}

bool QG_WidthBox::isUnchanged() const{
    return m_unchanged;
}

/**
 * Initialisation (called from constructor or manually but only
 * once).
 *
 * @param showByLayer true: Show attributes ByLayer, ByBlock
 */
void QG_WidthBox::init(bool showByLayer, bool showUnchanged) {
    m_showByLayer = showByLayer;
	m_showUnchanged = showUnchanged;
    for(const auto& [icon, text, lineWidth]: g_boxItems) {
        switch (lineWidth) {
            case RS2::WidthUnchanged:
                if (!showUnchanged) {
                    continue;
                }
                break;
            case RS2::WidthByLayer:
            case RS2::WidthByBlock:
                if (!showByLayer) {
                    continue;
                }
                break;
            default:
                break;
        }
        addItem(QIcon(icon), text, lineWidth);
        m_width2Index.emplace(lineWidth, count() - 1);
    }

    connect(this, &QG_WidthBox::activated, this, &QG_WidthBox::slotWidthChanged);
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

    if (it->second == currentIndex()) {
        return;
    }
    setCurrentIndex(it->second);
    slotWidthChanged(currentIndex());
}

/**
 * Sets the pixmap showing the width of the "By Layer" item.
 */
void QG_WidthBox::setLayerWidth(RS2::LineWidth w) {
    if (m_showByLayer) {
        QIcon pixmap;
        switch(w) {
        default:
        case RS2::Width00:
            pixmap = QPixmap(":linetypes/width00.lci");
            break;
        case RS2::Width01:
        case RS2::Width02:
            pixmap = QPixmap(":linetypes/width01.lci");
            break;
        case RS2::Width03:
        case RS2::Width04:
            pixmap = QPixmap(":linetypes/width02.lci");
            break;
        case RS2::Width05:
        case RS2::Width06:
            pixmap = QPixmap(":linetypes/width03.lci");
            break;
        case RS2::Width07:
        case RS2::Width08:
            pixmap = QPixmap(":linetypes/width04.lci");
            break;
        case RS2::Width09:
        case RS2::Width10:
            pixmap = QPixmap(":linetypes/width05.lci");
            break;
        case RS2::Width11:
        case RS2::Width12:
            pixmap = QPixmap(":linetypes/width06.lci");
            break;
        case RS2::Width13:
        case RS2::Width14:
            pixmap = QPixmap(":linetypes/width07.lci");
            break;
        case RS2::Width15:
        case RS2::Width16:
            pixmap = QPixmap(":linetypes/width08.lci");
            break;
        case RS2::Width17:
        case RS2::Width18:
            pixmap = QPixmap(":linetypes/width09.lci");
            break;
        case RS2::Width19:
        case RS2::Width20:
            pixmap = QPixmap(":linetypes/width10.lci");
            break;
        case RS2::Width21:
        case RS2::Width22:
            pixmap = QPixmap(":linetypes/width11.lci");
            break;
        case RS2::Width23:
            //case RS2::Width24:
            pixmap = QPixmap(":linetypes/width12.lci");
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
    if (m_showUnchanged && index == 0) {
        m_unchanged = true;
    } else {
        m_unchanged = false;
        m_currentWidth = static_cast<RS2::LineWidth>(itemData(index).toInt());
    }

    RS_DEBUG->print("Current width is (%d): %d\n",
                    index, ((int)m_currentWidth));

    emit widthChanged(m_currentWidth);
}
