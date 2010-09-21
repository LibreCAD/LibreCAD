/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_TextOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawText) {
        action = (RS_ActionDrawText*)a;

        QString st;
        QString sa;
        if (update) {
            st = action->getText();
            sa = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
        } else {
            st = "";
            sa = "0.0";
        }

/*#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        QTextCodec *codec = QTextCodec::codecForName(iso);
        if (codec!=NULL) {
            st = codec->toUnicode(RS_FilterDXF::toNativeString(action->getText().local8Bit()));
        } else {
            st = RS_FilterDXF::toNativeString(action->getText().local8Bit());
        }
//#else*/
        teText->setText(st);
//#endif
        leAngle->setText(sa);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_TextOptions::setAction: wrong action type");
        action = NULL;
    }

}

void QG_TextOptions::updateText() {
    if (action!=NULL) {
/*#if defined(OOPL_VERSION) && defined(Q_WS_WIN)
        QCString iso = RS_System::localeToISO( QTextCodec::locale() );
        action->setText(
            RS_FilterDXF::toNativeString( 
             QString::fromLocal8Bit( QTextCodec::codecForName( iso )->fromUnicode( teText->text() ) )
            )
        );
//#else*/
       action->setText(teText->text());
//#endif
    }
}

void QG_TextOptions::updateAngle() {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
    }
}
