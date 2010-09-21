/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

void QG_SelectionWidget::init() {
    lEntities->setText("0");
    
    int fsize;
#ifdef __APPLE__
    fsize = 9;
#else
    fsize = 7;
#endif
    
    RS_SETTINGS->beginGroup("/Appearance");
    fsize = RS_SETTINGS->readNumEntry("/StatusBarFontSize", fsize);
    RS_SETTINGS->endGroup();
    
    lEntities->setFont(QFont("Helvetica", fsize));
    lLabel->setFont(QFont("Helvetica", fsize));
}

void QG_SelectionWidget::setNumber(int n) {
    QString str;
    str.setNum(n);
    lEntities->setText(str);
}
