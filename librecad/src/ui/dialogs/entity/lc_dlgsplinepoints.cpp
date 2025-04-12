
#include "lc_dlgsplinepoints.h"

#include <QStandardItemModel>

#include "lc_splinepoints.h"
#include "rs_graphic.h"
#include "ui_lc_dlgsplinepoints.h"

class RS_Graphic;

LC_DlgSplinePoints::LC_DlgSplinePoints(QWidget* parent, LC_GraphicViewport* vp,LC_SplinePoints * splinePoints)
    : LC_EntityPropertiesDlg(parent,"SplinePointProperties",vp)
    , ui(new Ui::DlgSplinePoints{}){

    ui->setupUi(this);
    connect(ui->rbSplinePoints, &QRadioButton::toggled,this, &LC_DlgSplinePoints::updatePoints);
    setEntity(splinePoints);
}

LC_DlgSplinePoints::~LC_DlgSplinePoints() = default;

void LC_DlgSplinePoints::languageChange(){
    ui->retranslateUi(this);
}

void LC_DlgSplinePoints::setEntity(LC_SplinePoints* b){
    m_entity = b;

    RS_Graphic* graphic = b->getGraphic();
    if (graphic) {
        ui->cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = b->getLayer(false);
    if (lay != nullptr) {
        ui->cbLayer->setLayer(*lay);
    }

    ui->wPen->setPen(b,lay, tr("Pen"));

    ui->cbClosed->setChecked(b->isClosed());

//number of control points
    auto const& bData = b->getData();
    auto const n = bData.splinePoints.size();
    if (n <= 2) {
        ui->rbControlPoints->setChecked(true);
        ui->rbSplinePoints->setEnabled(false);
    } else {
        ui->rbSplinePoints->setChecked(true);
    }
    updatePoints();
}

void LC_DlgSplinePoints::updatePoints() {
    bool const useSpline = ui->rbSplinePoints->isChecked();

    auto const &bData = m_entity->getData();
    auto const &pts = useSpline ? bData.splinePoints : bData.controlPoints;
    auto model = new QStandardItemModel(pts.size(), 2, this);
    model->setHorizontalHeaderLabels({"x", "y"});

    //set spline data
    for (size_t row = 0; row < pts.size(); ++row) {
        auto const &vp = pts.at(row);
        QPair<QString, QString> pair = toUIStr(vp);

        auto *x = new QStandardItem(pair.first);
        auto *y = new QStandardItem(pair.second);

        model->setItem(row, 0, x);
        model->setItem(row, 1, y);
    }
    ui->tvPoints->setModel(model);
}

void LC_DlgSplinePoints::updateEntity() {
    if (!m_entity) return;

    m_entity->setClosed(ui->cbClosed->isChecked());
    m_entity->setPen(ui->wPen->getPen());
    m_entity->setLayer(ui->cbLayer->getLayer());

 //update Spline Points
    auto model = static_cast<QStandardItemModel *>(ui->tvPoints->model());
    size_t const n = model->rowCount();
    auto &d = m_entity->getData();

//update points
    bool const useSpline = ui->rbSplinePoints->isChecked();
    auto &vps = useSpline ? d.splinePoints : d.controlPoints;
    size_t const n0 = vps.size();
//update points
    for (size_t i = 0; i < n; ++i) {
        auto &vp = vps.at(i < n0 ? i : n0 - 1);
        auto const &vpx = model->item(i, 0)->text();
        auto const &vpy = model->item(i, 1)->text();

        RS_Vector wcsPoint = toWCSVector(vpx, vpy, vp);

        vp.x = wcsPoint.x;
        vp.y = wcsPoint.y;
    }
    m_entity->update();
}
