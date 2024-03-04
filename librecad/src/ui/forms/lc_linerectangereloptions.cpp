#include "lc_linerectangereloptions.h"
#include "ui_lc_linerectangereloptions.h"

LC_LineRectangeRelOptions::LC_LineRectangeRelOptions(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LC_LineRectangeRelOptions)
{
    ui->setupUi(this);
}

LC_LineRectangeRelOptions::~LC_LineRectangeRelOptions()
{
    delete ui;
}
