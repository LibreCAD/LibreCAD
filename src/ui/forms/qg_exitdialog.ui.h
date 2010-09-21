/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

#include <qmessagebox.h>
#include <q3accel.h>
//Added by qt3to4:
#include <q3mimefactory.h>

//sets aditional accel, eg. Key_A for ALT+Key_A

/*
static void makeLetterAccell(QButton *btn)
{
    if (btn->accel().count()<1)
        return;
    Q3Accel *a = new Q3Accel( btn );
    a->connectItem( a->insertItem( btn->accel() & ~(Qt::MODIFIER_MASK|Qt::UNICODE_ACCEL) ),
                    btn, SLOT(animateClick()) );
} */

void QG_ExitDialog::init()
{
    //set dlg icon
    QMessageBox mb("","",QMessageBox::Warning, QMessageBox::Ok, Qt::NoButton, Qt::NoButton);
    // RVT_PORT l_icon->setPixmap( *mb.iconPixmap() );
    bLeave->setIconSet(qPixmapFromMimeSource("fileclose.png"));
    // RVT_PORT makeLetterAccell( bLeave );
    bSave->setIconSet(qPixmapFromMimeSource("filesave2.png"));
     // RVT_PORT makeLetterAccell( bSave );
    bSaveAs->setIconSet(qPixmapFromMimeSource("filesaveas.png"));
     // RVT_PORT makeLetterAccell( bSaveAs );
    // RVT_PORT  makeLetterAccell( bCancel );
}

void QG_ExitDialog::setText(const QString& text) {
    lQuestion->setText(text);
/* RVT_PORT
    resize(lQuestion->sizeHint().width()+32,
           lQuestion->sizeHint().height() + layButtons->sizeHint().height()+32);
           */
}

void QG_ExitDialog::setTitle(const QString& text) {
    setCaption(text);
}

void QG_ExitDialog::setForce(bool force) {
     bCancel->setDisabled(force);
}

void QG_ExitDialog::slotSaveAs() {
    done(3);
}

void QG_ExitDialog::slotSave() {
    done(2);
}

 
