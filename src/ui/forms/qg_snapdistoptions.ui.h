/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_SnapDistOptions::destroy() {
    RS_SETTINGS->beginGroup("/Snap");
    RS_SETTINGS->writeEntry("/Distance", leDist->text());
    RS_SETTINGS->endGroup();
}

void QG_SnapDistOptions::setDist(double* d) {
    dist = d;

    RS_SETTINGS->beginGroup("/Snap");
    QString r = RS_SETTINGS->readEntry("/Distance", "1.0");
    RS_SETTINGS->endGroup();

    leDist->setText(r);
}

void QG_SnapDistOptions::updateDist(const QString& d) {
    if (dist!=NULL) {
        *dist = RS_Math::eval(d, 1.0);
    }
}
