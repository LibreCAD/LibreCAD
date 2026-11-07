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

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_layerlist.h"

QG_ActiveLayerName::QG_ActiveLayerName(QWidget *parent) :
    QWidget(parent){
    setupUi(this);
    lActiveLayerName->setText("");

    // the name, then an icon for each state the Layer List and the Layer Tree show with an icon: hidden, locked
    m_hiddenIcon = new QLabel(this);
    m_lockedIcon = new QLabel(this);
    auto* row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);
    verticalLayout->removeWidget(lActiveLayerName);
    row->addWidget(lActiveLayerName);
    row->addWidget(m_hiddenIcon);
    row->addWidget(m_lockedIcon);
    row->addStretch(1);
    verticalLayout->addLayout(row);
    m_hiddenIcon->hide();
    m_lockedIcon->hide();
}

QG_ActiveLayerName::~QG_ActiveLayerName() = default;

/**
 * Shows or hides one of the state icons, with the icon of the docks and a tool tip that says what it means.
 */
void QG_ActiveLayerName::showState(QLabel* label, const char* iconResource, const QString& toolTip, const bool shown) const {
    label->setVisible(shown);
    if (!shown) {
        return;
    }
    const int side = qBound(14, fontMetrics().height(), 24);
    label->setPixmap(QIcon(QString::fromLatin1(iconResource)).pixmap(side, side));
    label->setToolTip(toolTip);
    label->setAccessibleName(toolTip);
}

/**
 * Shows the current layer of the layer list, and, with the icons the docks use, whether it is hidden or
 * locked: what is drawn goes to it all the same, and is not shown on a hidden layer (#3229).
 */
void QG_ActiveLayerName::updateActiveLayer() const {
    const RS_Layer* layer = (m_layerList != nullptr) ? m_layerList->getActive() : nullptr;
    lActiveLayerName->setToolTip(tr("Name of Current Active Layer"));
    if (layer == nullptr) {
        lActiveLayerName->setText("");
        showState(m_hiddenIcon, ":/icons/not_visible.lci", {}, false);
        showState(m_lockedIcon, ":/icons/locked.lci", {}, false);
        return;
    }
    lActiveLayerName->setText(layer->getName());
    showState(m_hiddenIcon, ":/icons/not_visible.lci",
              tr("The current layer is hidden: what is drawn on it is not shown until the layer is shown."),
              layer->isFrozen());
    showState(m_lockedIcon, ":/icons/locked.lci",
              tr("The current layer is locked, as no other layer is unlocked: it cannot be selected or modified."),
              layer->isLocked());
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
    RS_Graphic* graphic = (gview != nullptr) ? gview->getGraphic(true) : nullptr;
    setLayerList((graphic != nullptr) ? graphic->getLayerList() : nullptr);
}
