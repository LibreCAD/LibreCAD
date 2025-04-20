/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "qg_pentoolbar.h"

#include "qg_colorbox.h"
#include "qg_widthbox.h"
#include "qg_linetypebox.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_pen.h"

/**
 * Constructor.
 */
QG_PenToolBar::QG_PenToolBar( const QString & title, QWidget * parent )
		: QToolBar(title, parent)
		, m_currentPen(new RS_Pen{})
		, m_colorBox(new QG_ColorBox{true, false, this, "colorbox"})
		, m_widthBox(new QG_WidthBox{true, false, this, "widthbox"})
		, m_lineTypeBox(new QG_LineTypeBox{true, false, this, "lineTypebox"})
{
    m_colorBox->setToolTip(tr("Line color"));
    connect(m_colorBox.get(), &QG_ColorBox::colorChanged, this, &QG_PenToolBar::slotColorChanged);

    m_widthBox->setToolTip(tr("Line width"));
    connect(m_widthBox.get(), &QG_WidthBox::widthChanged,this, &QG_PenToolBar::slotWidthChanged);

    m_lineTypeBox->setToolTip(tr("Line type"));
    connect(m_lineTypeBox.get(), &QG_LineTypeBox::lineTypeChanged,this,&QG_PenToolBar::slotLineTypeChanged);

    m_currentPen->setColor(m_colorBox->getColor());
    m_currentPen->setWidth(m_widthBox->getWidth());
    m_currentPen->setLineType(m_lineTypeBox->getLineType());

    addWidget(m_colorBox.get());
    addWidget(m_widthBox.get());
    addWidget(m_lineTypeBox.get());
}

/**
 * Destructor
 */
QG_PenToolBar::~QG_PenToolBar() = default;

void QG_PenToolBar::updateByLayer(RS_Layer* l) {
    if (l==nullptr) return;
    m_colorBox->setLayerColor(l->getPen().getColor());
    m_widthBox->setLayerWidth(l->getPen().getWidth());
    m_lineTypeBox->setLayerLineType(l->getPen().getLineType());
}

/**
 * Called by the layer list if this object was added as a listener
 * to a layer list.
 */
void QG_PenToolBar::layerActivated(RS_Layer* l) {
    updateByLayer(l);
}

void QG_PenToolBar::setLayerColor(RS_Color color, bool updateSelection){
    m_colorBox->setLayerColor(color);
    if (updateSelection){
        m_colorBox->setCurrentIndex(0);
    }
    m_currentPen->setColor(color);
    emitPenChanged();
}

void QG_PenToolBar::setColor(RS_Color color){
    m_colorBox->setColor(color);
}

void QG_PenToolBar::setLayerLineType(RS2::LineType lineType, bool updateSelection){
    m_lineTypeBox->setLayerLineType(lineType);
    if (updateSelection){
        m_lineTypeBox->setCurrentIndex(0);
    }
    m_currentPen->setLineType(lineType);
    emitPenChanged();
}

void QG_PenToolBar::setLineType(RS2::LineType lineType){
    m_lineTypeBox->setLineType(lineType);
}

void QG_PenToolBar::setLayerWidth(RS2::LineWidth width, bool updateSelection){
    m_widthBox->setLayerWidth(width);
    if (updateSelection){
        m_widthBox->setCurrentIndex(0);
    }
    m_currentPen->setWidth(width);
    emitPenChanged();
}

void QG_PenToolBar::setWidth(RS2::LineWidth width){
    m_widthBox->setWidth(width);
}

void QG_PenToolBar::emitPenChanged(){
    emit penChanged(*m_currentPen);
}

void QG_PenToolBar::setLayerList(RS_LayerList* ll) {
    if (m_layerList != nullptr) {
        m_layerList->removeListener(this);
    }
    m_layerList = ll;
    if (ll != nullptr) {
        m_layerList->addListener(this);
        auto activeLayer = m_layerList->getActive();
        if (activeLayer != nullptr) {
            updateByLayer(activeLayer);
        }
    }
}

void QG_PenToolBar::setGraphicView(RS_GraphicView* gv) {
    if (gv == nullptr) {
        setLayerList(nullptr);
    }
    else {
        auto graphic = gv->getGraphic();
        if (graphic == nullptr) {
            setLayerList(nullptr);
        }
        else {
            auto layerList = graphic->getLayerList();
            setLayerList(layerList);
        }
    }
}

RS_Pen QG_PenToolBar::getPen() const {
    return *m_currentPen;
}

/**
 * Called by the layer list (if this object was previously
 * added as a listener to a layer list).
 */
void QG_PenToolBar::layerEdited(RS_Layer* l) { updateByLayer(l); }

/**
 * Called when the color was changed by the user.
 */
void QG_PenToolBar::slotColorChanged(const RS_Color& color) {
    m_currentPen->setColor(color);
    //printf("  color changed\n");

    emit penChanged(*m_currentPen);
}

/**
 * Called when the width was changed by the user.
 */
void QG_PenToolBar::slotWidthChanged(RS2::LineWidth w) {
    m_currentPen->setWidth(w);
    //printf("  width changed\n");

    emit penChanged(*m_currentPen);
}

/**
 * Called when the linetype was changed by the user.
 */
void QG_PenToolBar::slotLineTypeChanged(RS2::LineType w) {
    m_currentPen->setLineType(w);
    //printf("  line type changed\n");

    emit penChanged(*m_currentPen);
}
