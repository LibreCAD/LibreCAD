
#ifdef DEVELOPER

#include "rs_dialogs.h"
#include <QVBoxLayout>
#include <QLabel>

RS_InputDialog::RS_InputDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags ( Qt::FramelessWindowHint );
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Press a character key"));
    label->setStyleSheet("font: 18pt;");
    layout->addWidget(label);
}

void RS_InputDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->text() != "") {
        qDebug() << "RS_InputDialog::keyPressEvent pressed: (char)" << (uint_fast8_t)event->text().at(0).toLatin1();
        m_char = (char) event->text().at(0).toLatin1();
        accept();
    }
}

#endif // DEVELOPER
