/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2025 librecad.org
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

#include "qg_activelayername.h"

#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

QG_ActiveLayerName::QG_ActiveLayerName(QWidget *parent) :
    QWidget(parent){
    setupUi(this);
    lActiveLayerName->setText("");
}

QG_ActiveLayerName::~QG_ActiveLayerName() = default;

/**
 * Shows the current layer of the layer list, and whether it is hidden or locked: what is drawn goes to
 * it all the same, and is not shown on a hidden layer (#3229).
 */
void QG_ActiveLayerName::updateActiveLayer() const {
    const RS_Layer* layer = (m_layerList != nullptr) ? m_layerList->getActive() : nullptr;
    if (layer == nullptr) {
        lActiveLayerName->setText("");
        lActiveLayerName->setToolTip(tr("Name of Current Active Layer"));
        return;
    }
    QString text = layer->getName();
    QString toolTip = tr("Name of Current Active Layer");
    if (layer->isFrozen() && layer->isLocked()) {
        text = tr("%1 (hidden, locked)").arg(text);
    }
    else if (layer->isFrozen()) {
        text = tr("%1 (hidden)").arg(text);
    }
    else if (layer->isLocked()) {
        text = tr("%1 (locked)").arg(text);
    }
    if (layer->isFrozen()) {
        toolTip += "\n" + tr("The current layer is hidden: what is drawn on it is not shown until the layer is shown.");
    }
    if (layer->isLocked()) {
        toolTip += "\n" + tr("The current layer is locked: what is drawn goes to it, but cannot be selected or modified.");
    }
    lActiveLayerName->setText(text);
    lActiveLayerName->setToolTip(toolTip);
}

void QG_ActiveLayerName::layerListModified(bool) {
    updateActiveLayer();
}

/**
 * The layer list is being destroyed with its drawing, which was not detached from this widget first:
 * the list has already dropped this listener, so forget it.
 */
void QG_ActiveLayerName::layerListDestroyed() {
    m_layerList = nullptr;
    updateActiveLayer();
}

void QG_ActiveLayerName::setLayerList(RS_LayerList* layerList) {
    if (m_layerList != nullptr) {
        m_layerList->removeListener(this);
    }
    m_layerList = layerList;
    if (m_layerList != nullptr) {
        m_layerList->addListener(this);
    }
    updateActiveLayer();
}

void QG_ActiveLayerName::setGraphicView(RS_GraphicView* gview) {
    RS_Graphic* graphic = (gview != nullptr) ? gview->getGraphic() : nullptr;
    setLayerList((graphic != nullptr) ? graphic->getLayerList() : nullptr);
}
