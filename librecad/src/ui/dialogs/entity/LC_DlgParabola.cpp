/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2014 Pavel Krejcir (pavel@pamsoft.cz)

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
**********************************************************************/

#include <cmath>
#include <QStandardItemModel>

#include "LC_DlgParabola.h"
#include "lc_parabola.h"
#include "rs_dialogfactory.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "ui_LC_DlgParabola.h"

LC_DlgParabola::LC_DlgParabola(QWidget* parent)
	: LC_Dialog(parent, "ParabolaProperties")
    , ui(std::make_unique<Ui::DlgParabola>()){
//	setModal(modal);
	ui->setupUi(this);
}

LC_DlgParabola::~LC_DlgParabola() = default;

void LC_DlgParabola::languageChange()
{
	ui->retranslateUi(this);
}

void LC_DlgParabola::setParabola(LC_Parabola& b)
{
    parabola = &b;
	//pen = spline->getPen();
	ui->wPen->setPen(b.getPen(false), true, false, "Pen");
	RS_Graphic* graphic = b.getGraphic();
	if (graphic) {
		ui->cbLayer->init(*(graphic->getLayerList()), false, false);
	}
	RS_Layer* lay = b.getLayer(false);
	if (lay) {
		ui->cbLayer->setLayer(*lay);
	}

	//number of control points
	updatePoints();
}

void LC_DlgParabola::updatePoints(){
    auto const& bData = parabola->getData();
    auto const& pts = bData.controlPoints;
    auto model = new QStandardItemModel(pts.size(), 2, this);
    model->setHorizontalHeaderLabels({"x", "y"});

    //set control data
    for (size_t row = 0; row < pts.size(); ++row) {
        auto const& vp = pts.at(row);
        auto* x = new QStandardItem(QString::number(vp.x));
        model->setItem(row, 0, x);
        auto* y = new QStandardItem(QString::number(vp.y));
        model->setItem(row, 1, y);
    }
    model->setRowCount(pts.size());
    ui->tvPoints->setModel(model);
    //connect(model, itemChanged(QStandardItem), this, &LC_DlgParabola::updateParabola);
}

void LC_DlgParabola::updateParabola(){
    if (parabola == nullptr)
        return;

//update pen
    parabola->setPen(ui->wPen->getPen());
//update layer
    parabola->setLayer(ui->cbLayer->currentText());
//update Spline Points
    auto model = static_cast<QStandardItemModel*>(ui->tvPoints->model());
    model->setRowCount(3);

//update points
    std::array<RS_Vector, 3> vps;
//update points
    for (size_t i = 0; i < 3; ++i) {
        auto& vp = vps.at(i);
        auto const& vpx = model->item(i, 0)->text();
        vp.x = RS_Math::eval(vpx, vp.x);
        auto const& vpy = model->item(i, 1)->text();
        vp.y = RS_Math::eval(vpy, vp.y);
    }
    if (std::abs(std::remainder(vps.front().angleTo(vps[1]) - vps.back().angleTo(vps[1]), M_PI)) < RS_TOLERANCE_ANGLE)
    {
        RS_DIALOGFACTORY->commandMessage(tr("Parabola control points cannot be collinear"));
        return;
    }
    auto& d = parabola->getData();
    d.controlPoints = vps;

    parabola->update();
}
