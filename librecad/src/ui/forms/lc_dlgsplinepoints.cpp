#include <QStandardItemModel>
#include <QDebug>
#include "lc_dlgsplinepoints.h"
#include "lc_splinepoints.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "ui_lc_dlgsplinepoints.h"

LC_DlgSplinePoints::LC_DlgSplinePoints(QWidget* parent, bool modal, Qt::WindowFlags fl)
	: QDialog(parent, fl)
	, ui(new Ui::DlgSplinePoints{})
{
	setModal(modal);
	ui->setupUi(this);
}

LC_DlgSplinePoints::~LC_DlgSplinePoints() = default;

void LC_DlgSplinePoints::languageChange()
{
	ui->retranslateUi(this);
}

void LC_DlgSplinePoints::setSpline(LC_SplinePoints& b)
{
	bezier = &b;
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

	ui->cbClosed->setChecked(b.isClosed());

	//number of control points
	auto const& bData = b.getData();
	auto const n = bData.splinePoints.size();
	auto model = new QStandardItemModel(n, 2, this);
	model->setHorizontalHeaderLabels({"x", "y"});
	//set spline data
	//TODO fix data editing for saved drawings
	//currently, there's no spline points after reading the spline from a saved
	//drawing, i.e., n = 0
	for (size_t row = 0; row < n; ++row) {
		auto const& vp = bData.splinePoints.at(row);
		qDebug()<<"Appending point: ("<<vp.x<<" , "<<vp.y<<")";
		QStandardItem* x = new QStandardItem(QString::number(vp.x));
		model->setItem(row, 0, x);
		QStandardItem* y = new QStandardItem(QString::number(vp.y));
		model->setItem(row, 1, y);
	}
	ui->tvPoints->setModel(model);
}

void LC_DlgSplinePoints::updateSpline()
{
	if (!bezier) return;
	//update closed
	bezier->setClosed(ui->cbClosed->isChecked());
	//update pen
	bezier->setPen(ui->wPen->getPen());
	//update layer
	bezier->setLayer(ui->cbLayer->currentText());
	//update Spline Points
	auto model = static_cast<QStandardItemModel*>(ui->tvPoints->model());
	size_t const n = model->rowCount();
	auto& d = bezier->getData();
	auto& vps = d.splinePoints;
	size_t const n0 = vps.size();
	//update points
	for (size_t i = 0; i < n; ++i) {
		auto& vp = vps.at(i<n0?i:n0-1);
		auto const& vpx = model->item(i, 0)->text();
		vp.x = RS_Math::eval(vpx, vp.x);
		auto const& vpy = model->item(i, 1)->text();
		vp.y = RS_Math::eval(vpy, vp.y);
	}
	bezier->update();
}


