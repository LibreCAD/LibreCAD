#ifndef RS_DIALOGS_H
#define RS_DIALOGS_H

#include <QDialog>
#include <QKeyEvent>

class RS_InputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RS_InputDialog(QWidget *parent = nullptr);
    ~RS_InputDialog() {}
    char getChar() { return m_char; }
    static char readChar()
    {
        RS_InputDialog dlg;
        if (dlg.exec() == QDialog::Accepted)
        {
            return dlg.getChar();
        }
        else
        {
            return '0';
        }
    }
protected:
    void keyPressEvent(QKeyEvent *event);
private:
    char m_char;
};

#endif // RS_DIALOGS_H
