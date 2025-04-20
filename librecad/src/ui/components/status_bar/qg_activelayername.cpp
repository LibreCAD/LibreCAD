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

QG_ActiveLayerName::QG_ActiveLayerName(QWidget *parent) :
    QWidget(parent){
    setupUi(this);
    lActiveLayerName->setText("");
}

void QG_ActiveLayerName::activeLayerChanged(const QString& name){
    lActiveLayerName->setText(name);
}

void QG_ActiveLayerName::setGraphicView(RS_GraphicView* gview) {
    if (gview == nullptr) {
        lActiveLayerName->setText("");
    }
    else {
        RS_Graphic* graphic = gview->getGraphic();
        if (graphic != nullptr) {
            auto activeLayer = graphic->getActiveLayer();
            QString activeLayerName = (activeLayer != nullptr) ? activeLayer->getName() : "";
            lActiveLayerName->setText(activeLayerName);
        }
    }
}
