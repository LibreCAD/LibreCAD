#include "lc_rectangle3pointsoptions.h"
#include "ui_lc_rectangle3pointsoptions.h"

LC_Rectangle3PointsOptions::LC_Rectangle3PointsOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LC_Rectangle3PointsOptions)
{
    ui->setupUi(this);
}

LC_Rectangle3PointsOptions::~LC_Rectangle3PointsOptions()
{
    delete ui;
}
