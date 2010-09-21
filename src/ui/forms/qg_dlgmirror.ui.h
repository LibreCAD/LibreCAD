/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgMirror::init() {
    RS_SETTINGS->beginGroup("/Modify");
    numberMode = RS_SETTINGS->readNumEntry("/MirrorMode", 0);
    useCurrentLayer =
        (bool)RS_SETTINGS->readNumEntry("/MirrorUseCurrentLayer", 0);
    useCurrentAttributes =
        (bool)RS_SETTINGS->readNumEntry("/MirrorUseCurrentAttributes", 0);
    RS_SETTINGS->endGroup();

    switch (numberMode) {
    case 0:
        rbMove->setChecked(true);
        break;
    case 1:
        rbCopy->setChecked(true);
        break;
    default:
        break;
    }
    cbCurrentAttributes->setChecked(useCurrentAttributes);
    cbCurrentLayer->setChecked(useCurrentLayer);
}

void QG_DlgMirror::destroy() {
    RS_SETTINGS->beginGroup("/Modify");
    if (rbMove->isChecked()) {
        numberMode = 0;
    } else if (rbCopy->isChecked()) {
        numberMode = 1;
    } else {
        numberMode = 2;
    }
    RS_SETTINGS->writeEntry("/MirrorMode", numberMode);
    RS_SETTINGS->writeEntry("/MirrorUseCurrentLayer",
                            (int)cbCurrentLayer->isChecked());
    RS_SETTINGS->writeEntry("/MirrorUseCurrentAttributes",
                            (int)cbCurrentAttributes->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_DlgMirror::setData(RS_MirrorData* d) {
    data = d;
}

void QG_DlgMirror::updateData() {
    if (rbMove->isChecked()) {
        data->copy = false;
    } else if (rbCopy->isChecked()) {
        data->copy = true;
    }
    data->useCurrentAttributes = cbCurrentAttributes->isChecked();
    data->useCurrentLayer = cbCurrentLayer->isChecked();
}

