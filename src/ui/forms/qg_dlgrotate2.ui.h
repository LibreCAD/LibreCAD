/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgRotate2::init() {
    RS_SETTINGS->beginGroup("/Modify");
    copies = RS_SETTINGS->readEntry("/Rotate2Copies", "10");
    numberMode = RS_SETTINGS->readNumEntry("/Rotate2Mode", 0);
    useCurrentLayer =
        (bool)RS_SETTINGS->readNumEntry("/Rotate2UseCurrentLayer", 0);
    useCurrentAttributes =
        (bool)RS_SETTINGS->readNumEntry("/MoveRotate2UseCurrentAttributes", 0);
    angle1 = RS_SETTINGS->readEntry("/Rotate2Angle1", "30.0");
    angle2 = RS_SETTINGS->readEntry("/Rotate2Angle2", "-30.0");
    RS_SETTINGS->endGroup();

    switch (numberMode) {
    case 0:
        rbMove->setChecked(true);
        break;
    case 1:
        rbCopy->setChecked(true);
        break;
    case 2:
        rbMultiCopy->setChecked(true);
        break;
    default:
        break;
    }
    leNumber->setText(copies);
    leAngle1->setText(angle1);
    leAngle2->setText(angle2);
    cbCurrentAttributes->setChecked(useCurrentAttributes);
    cbCurrentLayer->setChecked(useCurrentLayer);
}

void QG_DlgRotate2::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/Rotate2Copies", leNumber->text());
    if (rbMove->isChecked()) {
        numberMode = 0;
    } else if (rbCopy->isChecked()) {
        numberMode = 1;
    } else {
        numberMode = 2;
    }
    RS_SETTINGS->writeEntry("/Rotate2Mode", numberMode);
    RS_SETTINGS->writeEntry("/Rotate2Angle1", leAngle1->text());
    RS_SETTINGS->writeEntry("/Rotate2Angle2", leAngle2->text());
    RS_SETTINGS->writeEntry("/Rotate2UseCurrentLayer",
                            (int)cbCurrentLayer->isChecked());
    RS_SETTINGS->writeEntry("/Rotate2UseCurrentAttributes",
                            (int)cbCurrentAttributes->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_DlgRotate2::setData(RS_Rotate2Data* d) {
    data = d;

    //leAngle1->setText(QString("%1").arg(RS_Math::rad2deg(data->angle1)));
    //leAngle2->setText(QString("%1").arg(RS_Math::rad2deg(data->angle2)));
}

void QG_DlgRotate2::updateData() {
    if (rbMove->isChecked()) {
        data->number = 0;
    } else if (rbCopy->isChecked()) {
        data->number = 1;
    } else {
        data->number = leNumber->text().toInt();
    }
    data->angle1 = RS_Math::deg2rad(RS_Math::eval(leAngle1->text()));
    data->angle2 = RS_Math::deg2rad(RS_Math::eval(leAngle2->text()));
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
}

