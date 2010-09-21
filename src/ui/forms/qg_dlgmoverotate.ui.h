/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgMoveRotate::init() {
    RS_SETTINGS->beginGroup("/Modify");
    copies = RS_SETTINGS->readEntry("/MoveRotateCopies", "10");
    numberMode = RS_SETTINGS->readNumEntry("/MoveRotateMode", 0);
    useCurrentLayer =
        (bool)RS_SETTINGS->readNumEntry("/MoveRotateUseCurrentLayer", 0);
    useCurrentAttributes =
        (bool)RS_SETTINGS->readNumEntry("/MoveRotateUseCurrentAttributes", 0);
    angle = RS_SETTINGS->readEntry("/MoveRotateAngle", "0.0");
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
    leAngle->setText(angle);
    cbCurrentAttributes->setChecked(useCurrentAttributes);
    cbCurrentLayer->setChecked(useCurrentLayer);
}

void QG_DlgMoveRotate::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    RS_SETTINGS->writeEntry("/MoveRotateCopies", leNumber->text());
    if (rbMove->isChecked()) {
        numberMode = 0;
    } else if (rbCopy->isChecked()) {
        numberMode = 1;
    } else {
        numberMode = 2;
    }
    RS_SETTINGS->writeEntry("/MoveRotateMode", numberMode);
    RS_SETTINGS->writeEntry("/MoveRotateAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/MoveRotateUseCurrentLayer",
                            (int)cbCurrentLayer->isChecked());
    RS_SETTINGS->writeEntry("/MoveRotateUseCurrentAttributes",
                            (int)cbCurrentAttributes->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_DlgMoveRotate::setData(RS_MoveRotateData* d) {
    data = d;

    leAngle->setText(QString("%1").arg(RS_Math::rad2deg(data->angle)));
}

void QG_DlgMoveRotate::updateData() {
    if (rbMove->isChecked()) {
        data->number = 0;
    } else if (rbCopy->isChecked()) {
        data->number = 1;
    } else {
        data->number = leNumber->text().toInt();
    }
    data->angle = RS_Math::deg2rad(RS_Math::eval(leAngle->text()));
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
}

