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

#include "qg_linetypebox.h"

#include "rs_debug.h"

/**
 * Default Constructor. You must call init manually if you choose
 * to use this constructor.
 */
QG_LineTypeBox::QG_LineTypeBox(QWidget* parent)
        : QComboBox(parent) {
    m_showByLayer = false;
    m_showUnchanged = false;
    m_unchanged = false;
}

/**
 * Constructor that calls init and provides a fully functional 
 * combobox for choosing linetypes.
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 */
QG_LineTypeBox::QG_LineTypeBox(bool showByLayer, bool showUnchanged, 
		QWidget* parent, const char* name)
        : QComboBox(parent) {
    setObjectName(name);
    m_unchanged = false;
    init(showByLayer, showUnchanged);
}


/**
 * Destructor
 */
QG_LineTypeBox::~QG_LineTypeBox() = default;


/**
 * Initialisation (called from constructor or manually but only
 * once).
 *
 * @param showByLayer true: Show attribute ByLayer, ByBlock.
 */
void QG_LineTypeBox::init(bool showByLayer, bool showUnchanged, bool showNoPen) {
    LC_LOG<<__func__<<"(): QG_LineTypeBox, begin";
    this->m_showByLayer = showByLayer;
    this->m_showUnchanged = showUnchanged;

    if (showUnchanged) {
        addItem(QIcon(":linetypes/linetype00.lci"), tr("- Unchanged -"), RS2::LineTypeUnchanged);
    }

    if (showByLayer) {
        addItem(QIcon(":linetypes/linetype00.lci"), tr("By Layer"), RS2::LineByLayer);
        addItem(QIcon(":linetypes/linetype00.lci"), tr("By Block"), RS2::LineByBlock);
    }
    if (showNoPen) {
        addItem(QIcon(":linetypes/linetype00.lci"), tr("No Pen"), RS2::NoPen);
    }
    addItem(QIcon(":linetypes/linetype01.lci"), tr("Continuous"), RS2::SolidLine);
    addItem(QIcon(":linetypes/linetype02.lci"), tr("Dot"),RS2::DotLine);
    addItem(QIcon(":linetypes/linetype02.lci"), tr("Dot (tiny)"),RS2::DotLineTiny);
    addItem(QIcon(":linetypes/linetype02.lci"), tr("Dot (small)"), RS2::DotLine2);
    addItem(QIcon(":linetypes/linetype02.lci"), tr("Dot (large)"), RS2::DotLineX2);
    addItem(QIcon(":linetypes/linetype03.lci"), tr("Dash"),RS2::DashLine);
    addItem(QIcon(":linetypes/linetype03.lci"), tr("Dash (tiny)"),RS2::DashLineTiny);
    addItem(QIcon(":linetypes/linetype03.lci"), tr("Dash (small)"),RS2::DashLine2);
    addItem(QIcon(":linetypes/linetype03.lci"), tr("Dash (large)"),RS2::DashLineX2);
    addItem(QIcon(":linetypes/linetype04.lci"), tr("Dash Dot"),RS2::DashDotLine);
    addItem(QIcon(":linetypes/linetype04.lci"), tr("Dash Dot (tiny)"),RS2::DashDotLineTiny);
    addItem(QIcon(":linetypes/linetype04.lci"), tr("Dash Dot (small)"),RS2::DashDotLine2);
    addItem(QIcon(":linetypes/linetype04.lci"), tr("Dash Dot (large)"),RS2::DashDotLineX2);
    addItem(QIcon(":linetypes/linetype05.lci"), tr("Divide"),RS2::DivideLine);
    addItem(QIcon(":linetypes/linetype05.lci"), tr("Divide (tiny)"),RS2::DivideLineTiny);
    addItem(QIcon(":linetypes/linetype05.lci"), tr("Divide (small)"),RS2::DivideLine2);
    addItem(QIcon(":linetypes/linetype05.lci"), tr("Divide (large)"),RS2::DivideLineX2);
    addItem(QIcon(":linetypes/linetype06.lci"), tr("Center"),RS2::CenterLine);
    addItem(QIcon(":linetypes/linetype06.lci"), tr("Center (tiny)"),RS2::CenterLineTiny);
    addItem(QIcon(":linetypes/linetype06.lci"), tr("Center (small)"),RS2::CenterLine2);
    addItem(QIcon(":linetypes/linetype06.lci"), tr("Center (large)"),RS2::CenterLineX2);
    addItem(QIcon(":linetypes/linetype07.lci"), tr("Border"),RS2::BorderLine);
    addItem(QIcon(":linetypes/linetype07.lci"), tr("Border (tiny)"),RS2::BorderLineTiny);
    addItem(QIcon(":linetypes/linetype07.lci"), tr("Border (small)"),RS2::BorderLine2);
    addItem(QIcon(":linetypes/linetype07.lci"), tr("Border (large)"),RS2::BorderLineX2);

    connect(this, &QG_LineTypeBox::activated, this, &QG_LineTypeBox::slotLineTypeChanged);

    setCurrentIndex(0);
    slotLineTypeChanged(currentIndex());
    LC_LOG<<__func__<<"(): QG_LineTypeBox, done";
}

/**
 * Sets the currently selected linetype item to the given linetype.
 */
void QG_LineTypeBox::setLineType(RS2::LineType t) {
    RS_DEBUG->print("QG_LineTypeBox::setLineType %d\n", (int)t);
    switch (t) {
        case RS2::LineByLayer: {
            if (m_showByLayer) {
                setCurrentIndex(0 + (int)m_showUnchanged);
            } else {
                RS_DEBUG->print(RS_Debug::D_WARNING,
                                "QG_LineTypeBox::setLineType: "
                                "Combobox doesn't support linetype BYLAYER");
            }
            break;
        }
        case RS2::LineByBlock: {
            if (m_showByLayer) {
                setCurrentIndex(1 + (int)m_showUnchanged);
            } else {
                RS_DEBUG->print(RS_Debug::D_WARNING,
                                "QG_LineTypeBox::setLineType: "
                                "Combobox doesn't support linetype BYBLOCK");
            }
            break;
        }
        default:{
            int index=findData(t);
            if(t>=0){
                setCurrentIndex(index);
            }else{
                RS_DEBUG->print(RS_Debug::D_WARNING,
                                "QG_LineTypeBox::setLineType: "
                                "Combobox doesn't support linetype %d",(int) t);
            }
        }
    }
    slotLineTypeChanged(currentIndex());
}

/**
 * Sets the pixmap showing the linetype of the "By Layer" item.
 *
 * @todo needs an update, but not used currently
 */
void QG_LineTypeBox::setLayerLineType(RS2::LineType t) {
    if (m_showByLayer) {
        QPixmap pixmap;
        switch(t) {
            case RS2::NoPen:
                pixmap = QPixmap(":linetypes/linetype00.lci");
                break;
            default:
            case RS2::SolidLine:
                pixmap = QPixmap(":linetypes/linetype01.lci");
                break;
            case RS2::DashLine:
                pixmap = QPixmap(":linetypes/linetype02.lci");
                break;
            case RS2::DotLine:
                pixmap = QPixmap(":linetypes/linetype03.lci");
                break;
            case RS2::DashDotLine:
                pixmap = QPixmap(":linetypes/linetype04.lci");
                break;
            case RS2::DivideLine:
                pixmap = QPixmap(":linetypes/linetype05.lci");
                break;
        }

        setItemIcon(0, QIcon(pixmap));
        setItemText(0, tr("By Layer"));

        // needed for the first time a layer is added:
        slotLineTypeChanged(currentIndex());
    }
}

/**
 * Called when the linetype has changed. This method 
 * sets the current linetype to the value chosen or even
 * offers a dialog to the user that allows him/ her to
 * choose an individual linetype.
 */
void QG_LineTypeBox::slotLineTypeChanged(int index) {
    RS_DEBUG->print("QG_LineTypeBox::slotLineTypeChanged %d\n", index);
    m_unchanged = false;
    if (m_showByLayer) {
        switch (index) {
            case 0:
                if (m_showUnchanged) {
                    m_unchanged = true;
                }
                else {
                    m_currentLineType = RS2::LineByLayer;
                }
                break;

            case 1:
                if (m_showUnchanged) {
                    m_currentLineType = RS2::LineByLayer;
                }
                else {
                    m_currentLineType = RS2::LineByBlock;
                }
                break;

            default:
                m_currentLineType = (RS2::LineType) itemData(index).toInt();
        }
    } else {
        m_currentLineType = (RS2::LineType) itemData(index).toInt();
    }
    //    RS_DEBUG->print(RS_Debug::D_ERROR, "Current linetype is (%d): %d\n", index, currentLineType);
    RS_DEBUG->print("QG_LineTypeBox::slotLineTypeChanged %d: done\n", index);

    emit lineTypeChanged(m_currentLineType);
}
