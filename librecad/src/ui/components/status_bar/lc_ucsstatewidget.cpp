#include "lc_ucsstatewidget.h"
#include "ui_lc_ucsstatewidget.h"

LC_UCSStateWidget::LC_UCSStateWidget(QWidget* parent, const char* name)
    : QWidget(parent)
    , ui(new Ui::LC_UCSStateWidget){
    setObjectName(name);
    ui->setupUi(this);
}

LC_UCSStateWidget::~LC_UCSStateWidget(){
    delete ui;
}

void LC_UCSStateWidget::update(QIcon icon, QString ucsName, QString ucsInfo) {
    ui->lblName->setText(ucsName);
    ui->lblInfo->setText(ucsInfo);
    ui->lblType->setPixmap(icon.pixmap(iconSize));
    savedIcon = icon;
}

void LC_UCSStateWidget::onIconsRefreshed(){
    ui->lblType->setPixmap(savedIcon.pixmap(iconSize));
}
