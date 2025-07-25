#include "lc_dimstyleedit.h"
#include "ui_lc_dimstyleedit.h"

LC_DimStyleEdit::LC_DimStyleEdit(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LC_DimStyleEdit)
{
    ui->setupUi(this);
}

LC_DimStyleEdit::~LC_DimStyleEdit()
{
    delete ui;
}
