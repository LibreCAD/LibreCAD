#include <QMessageBox>
#include "lc_dlgucsproperties.h"
#include "ui_lc_dlgucsproperties.h"
#include "lc_ucslistmodel.h"
#include "rs_math.h"
#include "rs_units.h"

LC_DlgUCSProperties::LC_DlgUCSProperties(QWidget *parent)
    : LC_Dialog(parent, "UCSEdit")
    , ui(new Ui::LC_DlgUCSProperties)
{
    ui->setupUi(this);
}

LC_DlgUCSProperties::~LC_DlgUCSProperties(){
    delete ui;
}

void LC_DlgUCSProperties::languageChange() {
    ui->retranslateUi(this);
}

void LC_DlgUCSProperties::updateUCS() {
    QString name = ui->leName->text().trimmed();
    if (name.isEmpty()){
        QMessageBox::warning(this, tr("UCS Edit"),
                             tr("Please specify name of UCS"),
                             QMessageBox::Close,
                             QMessageBox::Close);
    }
    else if (!LC_UCS::isValidName(name)){
        QMessageBox::warning(this, tr("UCS Edit"),
                             tr("Name contains not allowed characters"),
                             QMessageBox::Close,
                             QMessageBox::Close);
    }
    else{
        LC_UCS* existingUCS = ucsList->find(name);
        if (existingUCS == nullptr){
            ucs->setName(name);
            ucsList->setModified(true);
        }
        else{
            if (existingUCS == ucs){
                // actually, do nothing... no name change
            }
            else{
                if (applyDuplicateSilently){

                }
            }
        }
    }
}

void LC_DlgUCSProperties::setUCS(LC_UCSList *ulist, bool applyDuplicates, [[maybe_unused]]LC_UCS* u, RS2::Unit unit, RS2::LinearFormat linearFormat, int linearPrec, RS2::AngleFormat angleFormat, int anglePrec) {

    double angleValue = RS_Math::correctAnglePlusMinusPi(ucs->getXAxis().angle());
    QString originX = RS_Units::formatLinear(ucs->getOrigin().x, unit, linearFormat, linearPrec);
    QString originY = RS_Units::formatLinear(ucs->getOrigin().y, unit, linearFormat, linearPrec);
    QString angle = RS_Units::formatAngle(angleValue, angleFormat, anglePrec);

    QString origin;
    origin.append(originX).append(" , "). append(originY);

    ui->lblOrigin->setText(origin);
    ui->lblAxis->setText(angle);

    QString xEnd = RS_Units::formatLinear(ucs->getXAxis().x, unit, linearFormat, linearPrec);
    QString yxEnd = RS_Units::formatLinear(ucs->getXAxis().y, unit, linearFormat, linearPrec);
    QString xAxis;
    xAxis.append(originX).append(" , "). append(originY);

    ui->lblAxisEndtpoint->setText(xAxis);

    QString gridType;
    switch (ucs->getOrthoType()) {
        case LC_UCS::FRONT:
        case LC_UCS::BACK:
            gridType = tr("Ortho");
            break;
        case LC_UCS::LEFT: {
            gridType = tr("Left");
            break;
        }
        case LC_UCS::RIGHT: {
            gridType = tr("Right");
            break;
        }
        case LC_UCS::TOP:
        case LC_UCS::BOTTOM: {
            gridType = tr("Top");
            break;
        }
        default:
            gridType = tr("Ortho");
    }

    ui->lblGridType->setText(gridType);

    ucsList = ulist;
    applyDuplicateSilently = applyDuplicates;

}
