#include "lc_offsetdlg.h"
#include <QIntValidator>

LC_OffsetDlg::LC_OffsetDlg(QWidget* parent)
    :QDialog(parent)
{
    this->setWindowTitle(tr("offset plugin"));
    Layout();

    connect(button,SIGNAL(clicked(bool)),this,SLOT(setData()));
    connect(edit,SIGNAL(editingFinished()),this,SLOT(setData()));
}

LC_OffsetDlg::~LC_OffsetDlg()
{

}


void LC_OffsetDlg::Layout()
{
    edit = new QLineEdit;
    edit->setText("0.0");
    edit->setValidator(new QIntValidator(-100,100,this));

    button = new QPushButton;
    button->setText(tr("OK"));

    layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("offset value: ")),0,0);
    layout->addWidget(edit,0,1);

    layout->addWidget(button,1,1,Qt::AlignRight);

    this->setLayout(layout);
}

void LC_OffsetDlg::setData()
{
    offset_val = edit->text().toDouble();

    this->accept();
}

double LC_OffsetDlg::getData()
{
    return offset_val;
}



