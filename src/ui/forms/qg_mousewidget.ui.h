/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/


void QG_MouseWidget::init() {
    lLeftButton->setText("");
    lRightButton->setText("");
    
    int fsize;
#ifdef __APPLE__
    fsize = 9;
#else
    fsize = 7;
#endif
    
    RS_SETTINGS->beginGroup("/Appearance");
    fsize = RS_SETTINGS->readNumEntry("/StatusBarFontSize", fsize);
    RS_SETTINGS->endGroup();
    
    lLeftButton->setFont(QFont("Helvetica", fsize));
    lRightButton->setFont(QFont("Helvetica", fsize));
}

void QG_MouseWidget::setHelp(const QString& left, const QString& right) {
    lLeftButton->setText(left);
    lRightButton->setText(right);
}
