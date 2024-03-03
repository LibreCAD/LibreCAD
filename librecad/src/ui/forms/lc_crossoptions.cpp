#include "lc_crossoptions.h"
#include "ui_lc_crossoptions.h"
#include "rs_settings.h"
#include "rs_debug.h"
#include "rs_math.h"

LC_CrossOptions::LC_CrossOptions(QWidget *parent) :
    LC_ActionOptionsWidget(parent),
    ui(new Ui::LC_CrossOptions)
{
    ui->setupUi(this);

    connect(ui->leX, &QLineEdit::editingFinished, this, &LC_CrossOptions::onXEditingFinished);
    connect(ui->leY, &QLineEdit::editingFinished, this, &LC_CrossOptions::onYEditingFinished);
    connect(ui->leAngle, &QLineEdit::editingFinished, this, &LC_CrossOptions::onAngleEditingFinished);
    connect(ui->cbMode, SIGNAL(currentIndexChanged(int)), SLOT(onModeIndexChanged(int)));
}

LC_CrossOptions::~LC_CrossOptions(){
    delete ui;
}

void LC_CrossOptions::clearAction(){
    action = nullptr;
}

bool LC_CrossOptions::checkActionRttiValid(RS2::ActionType actionType){
    return actionType ==RS2::ActionDrawCross;
}

void LC_CrossOptions::doSetAction(RS_ActionInterface *a, bool update){
    action = static_cast<LC_ActionDrawCross *>(a);

    QString x;
    QString y;
    QString angle;
    int mode;
    if (update){
        x = QString::number(action->getLenX(), 'g', 6);
        y = QString::number(action->getLenY(), 'g', 6);
        angle = QString::number(action->getCrossAngle(), 'g', 6);
        mode = action->getCrossMode();
    } else {
        RS_SETTINGS->beginGroup("/Draw");
        x = RS_SETTINGS->readEntry("/CrossX", "1.0");
        y = RS_SETTINGS->readEntry("/CrossY", "1.0");
        angle = RS_SETTINGS->readEntry("/CrossAngle", "0.0");
        mode = RS_SETTINGS->readNumEntry("/CrossMode", 1);
        RS_SETTINGS->endGroup();
    }
    setXToActionAndView(x);
    setYToActionAndView(y);
    setAngleToActionAndView(angle);
    setModeToActionAndView(mode);
}


void LC_CrossOptions::saveSettings(){
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/CrossX", ui->leX->text());
    RS_SETTINGS->writeEntry("/CrossY", ui->leY->text());
    RS_SETTINGS->writeEntry("/CrossAngle", ui->leAngle->text());
    RS_SETTINGS->writeEntry("/CrossMode", ui->cbMode->currentIndex());
    RS_SETTINGS->endGroup();
}

void LC_CrossOptions::onXEditingFinished(){
    const QString &expr = ui->leX->text();
    setXToActionAndView(expr);
}

void LC_CrossOptions::onYEditingFinished(){
    const QString &expr = ui->leY->text();
    setYToActionAndView(expr);
}

void LC_CrossOptions::onAngleEditingFinished(){
    const QString &expr = ui->leAngle->text();
    setAngleToActionAndView(expr);
}

void LC_CrossOptions::onModeIndexChanged(int index){
    setModeToActionAndView(index);
}

void LC_CrossOptions::setXToActionAndView(const QString &strValue){
    bool ok = false;
    double x =std::abs(RS_Math::eval(strValue, &ok));
    if(!ok) return;
    if (x<RS_TOLERANCE) x=1.0;
    action->setXLength(x);
    ui->leX->setText(QString::number(x, 'g', 6));
}

void LC_CrossOptions::setYToActionAndView(const QString &strValue){
    bool ok = false;
    double y =std::abs(RS_Math::eval(strValue, &ok));
    if(!ok) return;
    if (y<RS_TOLERANCE) y=1.0;
    action->setYLength(y);
    ui->leY->setText(QString::number(y, 'g', 6));
}

void LC_CrossOptions::setAngleToActionAndView(const QString &expr){
    bool ok = false;
    double angle =std::abs(RS_Math::eval(expr, &ok));
    if(!ok) return;
    if (angle < RS_TOLERANCE_ANGLE) angle=0.0;
    action->setCrossAngle(angle);
    ui->leAngle->setText(QString::number(angle, 'g', 6));
}

void LC_CrossOptions::setModeToActionAndView(int mode){
    action->setCrossMode(mode);
    ui->cbMode->setCurrentIndex(mode);
}

void LC_CrossOptions::languageChange(){
    ui->retranslateUi(this);
}
