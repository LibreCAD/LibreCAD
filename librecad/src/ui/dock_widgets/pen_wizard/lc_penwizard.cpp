/*
**********************************************************************************
**
** This file was created for LibreCAD (https://github.com/LibreCAD/LibreCAD).
**
** Copyright (C) 2016 ravas (github.com/r-a-v-a-s)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
** http://www.gnu.org/licenses/gpl-2.0.html
**
**********************************************************************************
 */

#include "lc_penwizard.h"

#include <QVBoxLayout>
#include <qnetworkreply.h>

#include "colorwizard.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_color.h"
#include "rs_selection.h"
#include "rs_settings.h"

LC_PenWizard::LC_PenWizard(QWidget* parent)
    : LC_GraphicViewAwareWidget(parent)
      , m_colorWizard(new ColorWizard(this)) {
    // auto frame = new QFrame(this);
    const auto layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(1,0, 1, 2);
    layout->setSpacing(1);
    layout->addWidget(m_colorWizard);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum));
    connect(m_colorWizard, &ColorWizard::requestingColorChange, this, &LC_PenWizard::setColorForSelected);
    connect(m_colorWizard, &ColorWizard::requestingSelection, this, &LC_PenWizard::selectByColor);
    connect(m_colorWizard, &ColorWizard::colorDoubleClicked, this, &LC_PenWizard::setActivePenColor);
    updateWidgetSettings();
}

void LC_PenWizard::setColorForSelected(const QColor color) const {
    const auto graphic = m_graphicView->getGraphic();
    auto pen = graphic->getActivePen();
    pen.setColor(RS_Color(color));

    QList<RS_Entity*> selection;
    if (graphic->collectSelected(selection)) {
        graphic->undoableModify(m_graphicView->getViewPort(), [selection, pen](LC_DocumentModificationBatch& ctx)-> bool {
                                    for (const auto e : selection) {
                                        RS_Entity* clone = e->clone();
                                        clone->setPen(pen);
                                        ctx += clone;
                                        ctx -= e;
                                    }
                                    return true;
                                }, [](const LC_DocumentModificationBatch& ctx, RS_Document* doc)-> void {
                                    doc->select(ctx.entitiesToAdd);
                                });
    }
}

void LC_PenWizard::selectByColor(const QColor color) const {
    const auto graphic = m_graphicView->getGraphic();
    QString colorName = color.name();
    const RS_Selection sel(m_graphicView);
    sel.selectIfMatched(graphic->getEntityList(), true, [colorName](const RS_Entity* e)->bool {
        return e->getPen().getColor().name() == colorName;
    });
}

void LC_PenWizard::setActivePenColor(const QColor color) const {
    const auto graphic = m_graphicView->getGraphic();
    auto pen = graphic->getActivePen();
    pen.setColor(RS_Color(color));
    graphic->setActivePen(pen);
}

QLayout* LC_PenWizard::getTopLevelLayout() const {
    return layout();
}

void LC_PenWizard::setGraphicView(RS_GraphicView* gview) {
    m_graphicView = gview;
}
