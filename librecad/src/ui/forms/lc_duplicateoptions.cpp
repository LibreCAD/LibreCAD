#include "lc_duplicateoptions.h"
#include "ui_lc_duplicateoptions.h"
#include "rs_math.h"
#include "rs_settings.h"

LC_DuplicateOptions::LC_DuplicateOptions(QWidget *parent):
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_DuplicateOptions){
    ui->setupUi(this);
    connect(ui->leOffsetX, &QLineEdit::editingFinished, this, &LC_DuplicateOptions::onOffsetXEditingFinished);
    connect(ui->leOffsetY, &QLineEdit::editingFinished, this, &LC_DuplicateOptions::onOffsetYEditingFinished);
}

LC_DuplicateOptions::~LC_DuplicateOptions(){
    saveSettings();
    delete ui;
}

void LC_DuplicateOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/DuplicateOffsetX", ui->leOffsetX->text());
    RS_SETTINGS->writeEntry("/DuplicateOffsetY", ui->leOffsetY->text());
    RS_SETTINGS->endGroup();
}

void LC_DuplicateOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = dynamic_cast<LC_ActionModifyDuplicate *>(a);
    QString ofX;
    QString ofY;
    if (update){
        ofX = QString::number(action->getOffsetX(), 'g', 6);
        ofY = QString::number(action->getOffsetY(), 'g', 6);
    }
    else{
        RS_SETTINGS->beginGroup("/Modify");
        ofX = RS_SETTINGS->readEntry("/DuplicateOffsetX", "0");
        ofY = RS_SETTINGS->readEntry("/DuplicateOffsetY", "0");
        RS_SETTINGS->endGroup();
    }
    setOffsetXToActionAndView(ofX);
    setOffsetYToActionAndView(ofY);
}

bool LC_DuplicateOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType == RS2::ActionModifyDuplicate;
}

void LC_DuplicateOptions::onOffsetXEditingFinished(){
    const QString &expr = ui->leOffsetX->text();
    setOffsetXToActionAndView(expr);
}

void LC_DuplicateOptions::onOffsetYEditingFinished(){
    const QString &expr = ui->leOffsetY->text();
    setOffsetYToActionAndView(expr);
}

void LC_DuplicateOptions::clearAction(){
    action = nullptr;
}

void LC_DuplicateOptions::setOffsetXToActionAndView(const QString &val){
    bool ok = false;
    double value = std::abs(RS_Math::eval(val, &ok));
    if (!ok) return;
    if (value < RS_TOLERANCE) value = 0;
    action->setOffsetX(value);
    ui->leOffsetX->setText(QString::number(value, 'g', 6));
}

void LC_DuplicateOptions::setOffsetYToActionAndView(const QString &val){
    bool ok = false;
    double value = std::abs(RS_Math::eval(val, &ok));
    if (!ok) return;
    if (value < RS_TOLERANCE) value = 0;
    action->setOffsetY(value);
    ui->leOffsetY->setText(QString::number(value, 'g', 6));
}
